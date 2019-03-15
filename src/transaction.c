/*******************************************************************************
*   Ledger App - Bitcoin Wallet
*   (c) 2016-2019 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "internal.h"
#include "apdu_constants.h"
#include "blake256.h"

#define DEBUG_LONG "%ld"


void check_transaction_available(unsigned char x)
{
    if (context_D.transactionDataRemaining < x)
    {
        PRINTF("Check transaction available failed: %d < %d\n",
                     context_D.transactionDataRemaining, x);
        THROW(0x01);
    }
}

#define OP_HASH160 0xA9
#define OP_EQUAL 0x87
#define OP_CHECKMULTISIG 0xAE

unsigned char transaction_amount_add_be(unsigned char *target,
                                        unsigned char WIDE *a,
                                        unsigned char WIDE *b)
{
    unsigned char carry = 0;
    unsigned char i;
    for (i = 0; i < 8; i++)
    {
        unsigned short val = a[8 - 1 - i] + b[8 - 1 - i] + (carry ? 1 : 0);
        carry = (val > 255);
        target[8 - 1 - i] = (val & 255);
    }
    return carry;
}

unsigned char transaction_amount_sub_be(unsigned char *target,
                                        unsigned char WIDE *a,
                                        unsigned char WIDE *b)
{
    unsigned char borrow = 0;
    unsigned char i;
    for (i = 0; i < 8; i++)
    {
        unsigned short tmpA = a[8 - 1 - i];
        unsigned short tmpB = b[8 - 1 - i];
        if (borrow)
        {
            if (tmpA <= tmpB)
            {
                tmpA += (255 + 1) - 1;
            }
            else
            {
                borrow = 0;
                tmpA--;
            }
        }
        if (tmpA < tmpB)
        {
            borrow = 1;
            tmpA += 255 + 1;
        }
        target[8 - 1 - i] = (unsigned char)(tmpA - tmpB);
    }
    return borrow;
}

void transaction_offset(unsigned char value)
{
    if ((context_D.transactionHashOption & TRANSACTION_HASH_FULL) != 0)
    {
        PRINTF("Add to prefix hash: ", value ,context_D.transactionBufferPointer);
        blake256_Update(&context_D.transactionHashPrefix, context_D.transactionBufferPointer, value);
        blake256_Update(&context_D.transactionHashAuthorization, context_D.transactionBufferPointer, value);
    }
    if ((context_D.transactionHashOption & TRANSACTION_HASH_WITNESS) != 0)
    {
        PRINTF("Add to witness hash: ", value, context_D.transactionBufferPointer);
        blake256_Update(&context_D.transactionHashWitness, context_D.transactionBufferPointer, value);
    }
}

void transaction_offset_increase(unsigned char value)
{
    transaction_offset(value);
    context_D.transactionBufferPointer += value;
    context_D.transactionDataRemaining -= value;
}

unsigned long int transaction_get_varint(void)
{
    unsigned char firstByte;
    check_transaction_available(1);
    firstByte = *context_D.transactionBufferPointer;
    if (firstByte < 0xFD)
    {
        transaction_offset_increase(1);
        return firstByte;
    }
    else if (firstByte == 0xFD)
    {
        unsigned long int result;
        transaction_offset_increase(1);
        check_transaction_available(2);
        result =
            (unsigned long int)(*context_D.transactionBufferPointer) |
            ((unsigned long int)(*(context_D.transactionBufferPointer +
                                   1))
             << 8);
        transaction_offset_increase(2);
        return result;
    }
    else if (firstByte == 0xFE)
    {
        unsigned long int result;
        transaction_offset_increase(1);
        check_transaction_available(4);
        result =
            read_u32(context_D.transactionBufferPointer, 0, 0);
        transaction_offset_increase(4);
        return result;
    }
    else
    {
        PRINTF("Varint parsing failed\n");
        THROW(INVALID_PARAMETER);
        return 0;
    }
}

void transaction_parse(unsigned char parseMode)
{
    unsigned char optionP2SHSkip2FA =
        ((N_btchip.bkp.config.options & OPTION_SKIP_2FA_P2SH) != 0);
    set_check_internal_structure_integrity(0);
    BEGIN_TRY
    {
        TRY
        {
            for (;;)
            {
                PRINTF("TX parse, state=%d\n", context_D.transactionContext.transactionState);
                switch (context_D.transactionContext.transactionState)
                {
                case TRANSACTION_NONE:
                {
                    PRINTF("\nInit transaction parser\n");
                    // Reset transaction state
                    context_D.transactionContext
                        .transactionRemainingInputsOutputs = 0;
                    context_D.transactionContext
                        .transactionCurrentInputOutput = 0;
                    context_D.transactionContext.scriptRemaining = 0;
                    os_memset(
                        context_D.transactionContext.transactionAmount,
                        0, sizeof(context_D.transactionContext.transactionAmount));
                    // TODO : transactionControlFid
                    // Reset hashes
                    blake256_Init(&context_D.transactionHashPrefix);
                    blake256_Init(&context_D.transactionHashWitness);
                    blake256_Init(&context_D.transactionHashAuthorization);
                    //cx_blake2b_init(&context_D.transactionHashPrefix, 256);
                    //cx_blake2b_init(&context_D.transactionHashWitness, 256);


                    // Parse the beginning of the transaction
                    // Version
                    check_transaction_available(4);
                    os_memmove(context_D.transactionVersion,
                               context_D.transactionBufferPointer, 4);
                    // decred "no witness" serialization type ORing
                    context_D.transactionBufferPointer[2] |= 1;
                    context_D.transactionHashOption = TRANSACTION_HASH_FULL; // prefix only
                    transaction_offset_increase(4);

                    context_D.transactionBufferPointer -= 4;
                    context_D.transactionDataRemaining += 4;
                    context_D.transactionBufferPointer[2] |= 3;
                    context_D.transactionHashOption = TRANSACTION_HASH_WITNESS; // witness only
                    transaction_offset_increase(4);

                    context_D.transactionHashOption = 0x05; // both prefix and witness hash


                    // Number of inputs
                    context_D.transactionContext
                        .transactionRemainingInputsOutputs =
                        transaction_get_varint();
                    context_D.transactionHashOption = TRANSACTION_HASH_FULL;

                    // Ready to proceed
                    context_D.transactionContext.transactionState =
                        TRANSACTION_DEFINED_WAIT_INPUT;

                    // no break is intentional
                }

                case TRANSACTION_DEFINED_WAIT_INPUT:
                {
                    unsigned char trustedInputFlag = 1;
                    if (context_D.transactionContext
                            .transactionRemainingInputsOutputs == 0)
                    {
                        // No more inputs to hash, move forward
                        context_D.transactionContext.transactionState =
                            TRANSACTION_INPUT_HASHING_DONE;
                        continue;
                    }
                    if (context_D.transactionDataRemaining < 1)
                    {
                        // No more data to read, ok
                        PRINTF("Waiting for more data...\n");
                        goto ok;
                    }
                    // Proceed with the next input
                    if (parseMode == PARSE_MODE_TRUSTED_INPUT)
                    {
                        PRINTF("PARSE_MODE_TRUSTED_INPUT\n");
                        check_transaction_available(
                            37); // prevout : 32 hash + 4 index + 1 tree
                        transaction_offset_increase(37);
                    }
                    if (parseMode == PARSE_MODE_SIGNATURE)
                    {
                        PRINTF("PARSE_MODE_SIGNATURE\n");
                        unsigned char trustedInputLength;
                        unsigned char trustedInput[0x38];
                        unsigned char amount[8];
                        unsigned char *savePointer;

                        // Expect the trusted input flag and trusted input
                        // length
                        check_transaction_available(2);
                        switch (*context_D.transactionBufferPointer)
                        {
                        case 0:
                            trustedInputFlag = 0;
                            break;
                        case 1:
                            trustedInputFlag = 1;
                            break;

                        default:
                            PRINTF("Invalid trusted input flag\n");
                            goto fail;
                        }
                        /*
                        trustedInputLength =
                        *(context_D.transactionBufferPointer + 1);
                        if (trustedInputLength > sizeof(trustedInput)) {
                          PRINTF("Trusted input too long\n");
                          goto fail;
                        }
                        */

                        if (!trustedInputFlag)
                        {

                            if (!optionP2SHSkip2FA)
                            {
                                PRINTF("Untrusted input not authorized\n");
                                goto fail;
                            }

                            context_D.transactionBufferPointer++;
                            context_D.transactionDataRemaining--;
                            check_transaction_available(
                                36); // prevout : 32 hash + 4 index
                            transaction_offset_increase(36);
                            PRINTF("Marking relaxed input\n");
                            context_D.transactionContext.relaxed = 1;
                            /*
                            PRINTF("Clearing P2SH consumption\n");
                            context_D.transactionContext.consumeP2SH = 0;
                            */
                        }
                        else
                        {
                            trustedInputLength = *(
                                context_D.transactionBufferPointer + 1);
                            if ((trustedInputLength > sizeof(trustedInput)) ||
                                (trustedInputLength < 8))
                            {
                                PRINTF("Invalid trusted input size\n");
                                goto fail;
                            }

                            check_transaction_available(2 + trustedInputLength);
                            cx_hmac_sha256(
                                N_btchip.bkp.trustedinput_key,
                                sizeof(N_btchip.bkp.trustedinput_key),
                                context_D.transactionBufferPointer + 2,
                                trustedInputLength - 8, trustedInput);
                            if (secure_memcmp(
                                    trustedInput,
                                    context_D.transactionBufferPointer +
                                        2 + trustedInputLength - 8,
                                    8) != 0)
                            {
                                PRINTF("Invalid signature\n");
                                goto fail;
                            }
                            os_memmove(
                                trustedInput,
                                context_D.transactionBufferPointer + 2,
                                trustedInputLength - 8);
                            if (trustedInput[0] != MAGIC_TRUSTED_INPUT)
                            {
                                PRINTF(("Failed to verify trusted input "
                                             "signature\n"));
                                goto fail;
                            }
                            else
                            {
                                PRINTF("Good Signature\n");
                            }

                            // Update the hash with prevout data
                            savePointer =
                                context_D.transactionBufferPointer; //trusted input 01
                            /*
                            // Check if a P2SH script is used
                            if ((trustedInput[1] & FLAG_TRUSTED_INPUT_P2SH) ==
                            0) {
                              PRINTF("Clearing P2SH consumption\n");
                              context_D.transactionContext.consumeP2SH =
                            0;
                            }
                            */
                            context_D.transactionBufferPointer =
                                trustedInput + 4;
                            PRINTF("Trusted input txid & input index\n", 36,
                                context_D.transactionBufferPointer);
                            transaction_offset(36);

                            context_D.transactionBufferPointer =
                                savePointer + (2 + trustedInputLength);
                            context_D.transactionDataRemaining -=
                                (2 + trustedInputLength);

                            // add Decred tree
                            check_transaction_available(1);
                            transaction_offset_increase(1);

                            // Update the amount

                            swap_bytes(amount, trustedInput + 40, 8);
                            if (transaction_amount_add_be(
                                    context_D.transactionContext
                                        .transactionAmount,
                                    context_D.transactionContext
                                        .transactionAmount,
                                    amount))
                            {
                                PRINTF("Overflow\n");
                                goto fail;
                            }

                            PRINTF("Adding amount\n", 8, (trustedInput + 40));
                            PRINTF("New amount\n", 8, context_D.transactionContext.transactionAmount);
                        }


                    }
                    // DIRTY: include utxo script len and script in witness hash
                    context_D.transactionHashOption = TRANSACTION_HASH_WITNESS;

                    // Read the script length
                    context_D.transactionContext.scriptRemaining =
                        transaction_get_varint();
                    PRINTF("Script to read %d \n",
                         context_D.transactionContext.scriptRemaining);

                    if ((parseMode == PARSE_MODE_SIGNATURE) &&
                        !trustedInputFlag)
                    {
                        // Only proceeds if this is not to be signed - so length
                        // should be null
                        if (context_D.transactionContext
                                .scriptRemaining != 0)
                        {
                            PRINTF("Request to sign relaxed input\n");
                            if (!optionP2SHSkip2FA)
                            {
                                goto fail;
                            }
                        }
                    }
                    PRINTF("NEXT_STATE\n");
                    // Move on
                    context_D.transactionContext.transactionState =
                        TRANSACTION_INPUT_HASHING_IN_PROGRESS_INPUT_SCRIPT;

                    // no break is intentional
                }
                case TRANSACTION_INPUT_HASHING_IN_PROGRESS_INPUT_SCRIPT:
                {
                    unsigned char dataAvailable;
                    PRINTF("Process input script, remaining %d, buffer %d\n",
                         context_D.transactionContext.scriptRemaining, context_D.transactionDataRemaining);
                    if (context_D.transactionDataRemaining < 1)
                    {
                        // No more data to read, ok
                        goto ok;
                    }
                    // Scan for P2SH consumption - huge shortcut, but fine
                    // enough
                    // Also usable in SegWit mode
                    if (context_D.transactionContext.scriptRemaining ==
                        1)
                    {
                        if (*context_D.transactionBufferPointer ==
                            OP_CHECKMULTISIG)
                        {
                            if (optionP2SHSkip2FA)
                            {
                                PRINTF("Marking P2SH consumption\n");
                                context_D.transactionContext
                                    .consumeP2SH = 1;
                            }
                        }
                        else
                        {
                            // When using the P2SH shortcut, all inputs must use
                            // P2SH
                            PRINTF("Disabling P2SH consumption\n");
                            context_D.transactionContext.consumeP2SH = 0;
                        }
                        transaction_offset_increase(1);
                        context_D.transactionContext.scriptRemaining--;

                    }

                    if (context_D.transactionContext.scriptRemaining ==
                        0)
                    {
                        // restore full hash
                        context_D.transactionHashOption = 0x01;

                        if (parseMode == PARSE_MODE_SIGNATURE)
                        {
                            context_D.transactionHashOption = TRANSACTION_HASH_BOTH;
                        }
                        // Sequence
                        check_transaction_available(4);
                        transaction_offset_increase(4);
                        // Move to next input
                        context_D.transactionContext
                            .transactionRemainingInputsOutputs--;
                        context_D.transactionContext
                            .transactionCurrentInputOutput++;
                        context_D.transactionContext.transactionState =
                            TRANSACTION_DEFINED_WAIT_INPUT;
                        continue;
                    }
                    // Save the last script byte for the P2SH check
                    dataAvailable =
                        (context_D.transactionDataRemaining > context_D.transactionContext.scriptRemaining - 1
                             ? context_D.transactionContext.scriptRemaining - 1
                             : context_D.transactionDataRemaining);
                    if (dataAvailable == 0)
                    {
                        goto ok;
                    }
                    transaction_offset_increase(dataAvailable);
                    context_D.transactionContext.scriptRemaining -=
                        dataAvailable;
                    break;
                }
                case TRANSACTION_INPUT_HASHING_DONE:
                {
                    PRINTF("Input hashing done\n");
                    if (parseMode == PARSE_MODE_SIGNATURE)
                    {

                        context_D.transactionContext
                            .transactionState =
                            TRANSACTION_PRESIGN_READY;
                        continue;
                    }
                    if (context_D.transactionDataRemaining < 1)
                    {
                        // No more data to read, ok
                        goto ok;
                    }
                    // Number of outputs
                    context_D.transactionContext
                        .transactionRemainingInputsOutputs =
                        transaction_get_varint();
                    context_D.transactionContext
                        .transactionCurrentInputOutput = 0;
                    /*MEPRINTF(("Number of outputs : " DEBUG_LONG "\n",
                                 context_D.transactionContext
                                     .transactionRemainingInputsOutputs));*/
                    // Ready to proceed
                    context_D.transactionContext.transactionState =
                        TRANSACTION_DEFINED_WAIT_OUTPUT;

                    // no break is intentional
                }
                case TRANSACTION_DEFINED_WAIT_OUTPUT:
                {
                    if (context_D.transactionContext
                            .transactionRemainingInputsOutputs == 0)
                    {
                        // No more outputs to hash, move forward
                        context_D.transactionContext.transactionState =
                            TRANSACTION_OUTPUT_HASHING_DONE;
                        continue;
                    }
                    if (context_D.transactionDataRemaining < 1)
                    {
                        // No more data to read, ok
                        goto ok;
                    }
                    // Amount
                    check_transaction_available(8);
                    if ((parseMode == PARSE_MODE_TRUSTED_INPUT) &&
                        (context_D.transactionContext
                             .transactionCurrentInputOutput ==
                         context_D.transactionTargetInput))
                    {
                        // Save the amount
                        os_memmove(context_D.transactionContext
                                       .transactionAmount,
                                   context_D.transactionBufferPointer,
                                   8);
                        context_D.trustedInputProcessed = 1;
                        PRINTF("Input processed\n");
                    }
                    transaction_offset_increase(8);

                    // script version
                    check_transaction_available(2);
                    transaction_offset_increase(2);

                    // Read the script length
                    context_D.transactionContext.scriptRemaining =
                        transaction_get_varint();

                    /*MEPRINTF(
                        ("Script to read " DEBUG_LONG "\n",
                         context_D.transactionContext.scriptRemaining));*/
                    // Move on
                    context_D.transactionContext.transactionState =
                        TRANSACTION_OUTPUT_HASHING_IN_PROGRESS_OUTPUT_SCRIPT;

                    // no break is intentional
                }
                case TRANSACTION_OUTPUT_HASHING_IN_PROGRESS_OUTPUT_SCRIPT:
                {
                    unsigned char dataAvailable;
                    /*MEPRINTF(
                        ("Process output script, remaining " DEBUG_LONG "\n",
                         context_D.transactionContext.scriptRemaining));*/
                    /*
                    // Special check if consuming a P2SH script
                    if (parseMode == PARSE_MODE_TRUSTED_INPUT) {
                      // Assume the full input script is sent in a single APDU,
                    then do the ghetto validation
                      if ((context_D.transactionBufferPointer[0] ==
                    OP_HASH160) &&
                          (context_D.transactionBufferPointer[context_D.transactionDataRemaining
                    - 1] == OP_EQUAL)) {
                        PRINTF("Marking P2SH output\n");
                        context_D.transactionContext.consumeP2SH = 1;
                      }
                    }
                    */
                    if (context_D.transactionDataRemaining < 1)
                    {
                        // No more data to read, ok
                        goto ok;
                    }
                    if (context_D.transactionContext.scriptRemaining ==
                        0)
                    {
                        // Move to next output
                        context_D.transactionContext
                            .transactionRemainingInputsOutputs--;
                        context_D.transactionContext
                            .transactionCurrentInputOutput++;
                        context_D.transactionContext.transactionState =
                            TRANSACTION_DEFINED_WAIT_OUTPUT;
                        continue;
                    }
                    dataAvailable =
                        (context_D.transactionDataRemaining >
                                 context_D.transactionContext
                                     .scriptRemaining
                             ? context_D.transactionContext
                                   .scriptRemaining
                             : context_D.transactionDataRemaining);
                    if (dataAvailable == 0)
                    {
                        goto ok;
                    }
                    transaction_offset_increase(dataAvailable);
                    context_D.transactionContext.scriptRemaining -=
                        dataAvailable;
                    break;
                }
                case TRANSACTION_OUTPUT_HASHING_DONE:
                {
                    PRINTF("Output hashing done\n");
                    if (context_D.transactionDataRemaining < 1)
                    {
                        // No more data to read, ok
                        goto ok;
                    }
                    // Locktime + Expiration
                    check_transaction_available(4+4);
                    transaction_offset_increase(4+4);

                    if (context_D.transactionDataRemaining == 0)
                    {
                        context_D.transactionContext.transactionState =
                            TRANSACTION_PARSED;
                        continue;
                    }
                    else
                    {
                        context_D.transactionHashOption = 0;
                        context_D.transactionContext.scriptRemaining =
                            transaction_get_varint();
                        context_D.transactionHashOption =
                            TRANSACTION_HASH_FULL;
                        context_D.transactionContext.transactionState =
                            TRANSACTION_PROCESS_EXTRA;
                        continue;
                    }
                }

                case TRANSACTION_PROCESS_EXTRA:
                {
                    unsigned char dataAvailable;

                    if (context_D.transactionContext.scriptRemaining ==
                        0)
                    {
                        context_D.transactionContext.transactionState =
                            TRANSACTION_PARSED;
                        continue;
                    }

                    if (context_D.transactionDataRemaining < 1)
                    {
                        // No more data to read, ok
                        goto ok;
                    }

                    dataAvailable =
                        (context_D.transactionDataRemaining >
                                 context_D.transactionContext
                                     .scriptRemaining
                             ? context_D.transactionContext
                                   .scriptRemaining
                             : context_D.transactionDataRemaining);
                    if (dataAvailable == 0)
                    {
                        goto ok;
                    }
                    transaction_offset_increase(dataAvailable);
                    context_D.transactionContext.scriptRemaining -=
                        dataAvailable;
                    break;
                }

                case TRANSACTION_PARSED:
                {
                    PRINTF("Transaction parsed\n");
                    goto ok;
                }

                case TRANSACTION_PRESIGN_READY:
                {
                    PRINTF("Presign ready\n");
                    goto ok;
                }

                case TRANSACTION_SIGN_READY:
                {
                    PRINTF("Sign ready\n");
                    goto ok;
                }
                }
            }

        fail:
            PRINTF("Transaction parse - fail\n");
            THROW(0x03);
        ok:
        {
        }
        }
        CATCH_OTHER(e)
        {
            PRINTF("Transaction parse - surprise fail\n");
            context_D.transactionContext.transactionState =
                TRANSACTION_NONE;
            set_check_internal_structure_integrity(1);
            THROW(e);
        }
        // before the finally to restore the surrounding context if an exception
        // is raised during finally
        FINALLY
        {
            set_check_internal_structure_integrity(1);
        }
    }
    END_TRY;
}
