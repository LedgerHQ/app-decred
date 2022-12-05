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

#include "btchip_internal.h"
#include "btchip_apdu_constants.h"
#include "blake256.h"

// #define DEBUG_LONG "%ld"

#define DEBUG_LONG "%d"

void check_transaction_available(unsigned char x) {
    if (btchip_context_D.transactionDataRemaining < x) {
        PRINTF("Check transaction available failed %d < %d\n", btchip_context_D.transactionDataRemaining, x);
        THROW(EXCEPTION);
    }
}

#define OP_HASH160 0xA9
#define OP_EQUAL 0x87
#define OP_CHECKMULTISIG 0xAE

unsigned char transaction_amount_add_be(unsigned char *target,
                                        unsigned char *a,
                                        unsigned char *b) {
    unsigned char carry = 0;
    unsigned char i;
    for (i = 0; i < 8; i++) {
        unsigned short val = a[8 - 1 - i] + b[8 - 1 - i] + (carry ? 1 : 0);
        carry = (val > 255);
        target[8 - 1 - i] = (val & 255);
    }
    return carry;
}

unsigned char transaction_amount_sub_be(unsigned char *target,
                                        unsigned char *a,
                                        unsigned char *b) {
    unsigned char borrow = 0;
    unsigned char i;
    for (i = 0; i < 8; i++) {
        unsigned short tmpA = a[8 - 1 - i];
        unsigned short tmpB = b[8 - 1 - i];
        if (borrow) {
            if (tmpA <= tmpB) {
                tmpA += (255 + 1) - 1;
            } else {
                borrow = 0;
                tmpA--;
            }
        }
        if (tmpA < tmpB) {
            borrow = 1;
            tmpA += 255 + 1;
        }
        target[8 - 1 - i] = (unsigned char)(tmpA - tmpB);
    }

    return borrow;
}

void transaction_offset(unsigned char value) {
    if ((btchip_context_D.transactionHashOption & TRANSACTION_HASH_FULL) != 0) {
        PRINTF("Add to prefix hash: ", value ,btchip_context_D.transactionBufferPointer);
        blake256_Update(&btchip_context_D.transactionHashPrefix, btchip_context_D.transactionBufferPointer, value);
        blake256_Update(&btchip_context_D.transactionHashAuthorization, btchip_context_D.transactionBufferPointer, value);
    }
    if ((btchip_context_D.transactionHashOption & TRANSACTION_HASH_WITNESS) != 0) {
        PRINTF("Add to witness hash: ", value, btchip_context_D.transactionBufferPointer);
        blake256_Update(&btchip_context_D.transactionHashWitness, btchip_context_D.transactionBufferPointer, value);
    }
}

void transaction_offset_increase(unsigned char value) {
    transaction_offset(value);
    btchip_context_D.transactionBufferPointer += value;
    btchip_context_D.transactionDataRemaining -= value;
}

unsigned long int transaction_get_varint(void) {
    unsigned char firstByte;
    check_transaction_available(1);
    firstByte = *btchip_context_D.transactionBufferPointer;
    if (firstByte < 0xFD) {
        transaction_offset_increase(1);
        return firstByte;
    } else if (firstByte == 0xFD) {
        unsigned long int result;
        transaction_offset_increase(1);
        check_transaction_available(2);
        result =
            (unsigned long int)(*btchip_context_D.transactionBufferPointer) |
            ((unsigned long int)(*(btchip_context_D.transactionBufferPointer +
                                   1))
             << 8);
        transaction_offset_increase(2);
        return result;
    } else if (firstByte == 0xFE) {
        unsigned long int result;
        transaction_offset_increase(1);
        check_transaction_available(4);
        result =
            btchip_read_u32(btchip_context_D.transactionBufferPointer, 0, 0);
        transaction_offset_increase(4);
        return result;
    } else {
        PRINTF("Varint parsing failed\n");
        THROW(INVALID_PARAMETER);
        return 0;
    }
}

void transaction_parse(unsigned char parseMode) {
    unsigned char optionP2SHSkip2FA =
        ((N_btchip.bkp.config.options & BTCHIP_OPTION_SKIP_2FA_P2SH) != 0);
    btchip_set_check_internal_structure_integrity(0);
    BEGIN_TRY {
        TRY {
            for (;;) {
                PRINTF("TX parse, state=%d\n", btchip_context_D.transactionContext.transactionState);
                switch (btchip_context_D.transactionContext.transactionState) {
                case BTCHIP_TRANSACTION_NONE: {
                    PRINTF("Init transaction parser\n");
                    // Reset transaction state
                    btchip_context_D.transactionContext
                        .transactionRemainingInputsOutputs = 0;
                    btchip_context_D.transactionContext
                        .transactionCurrentInputOutput = 0;
                    btchip_context_D.transactionContext.scriptRemaining = 0;
                    explicit_bzero(
                        btchip_context_D.transactionContext.transactionAmount,
                        sizeof(btchip_context_D.transactionContext
                                      .transactionAmount));
                    // TODO : transactionControlFid
                    // Reset hashes
                    blake256_Init(&btchip_context_D.transactionHashPrefix);
                    blake256_Init(&btchip_context_D.transactionHashWitness);
                    blake256_Init(&btchip_context_D.transactionHashAuthorization);
                    //cx_blake2b_init(&btchip_context_D.transactionHashPrefix, 256);
                    //cx_blake2b_init(&btchip_context_D.transactionHashWitness, 256);


                    // Parse the beginning of the transaction
                    // Version
                    check_transaction_available(4);
                    memmove(btchip_context_D.transactionVersion,
                               btchip_context_D.transactionBufferPointer, 4);
                    // decred "no witness" serialization type ORing
                    btchip_context_D.transactionBufferPointer[2] |= 1;
                    btchip_context_D.transactionHashOption = TRANSACTION_HASH_FULL; // prefix only
                    transaction_offset_increase(4);

                    btchip_context_D.transactionBufferPointer -= 4;
                    btchip_context_D.transactionDataRemaining += 4;
                    btchip_context_D.transactionBufferPointer[2] |= 3;
                    btchip_context_D.transactionHashOption = TRANSACTION_HASH_WITNESS; // witness only
                    transaction_offset_increase(4);

                    btchip_context_D.transactionHashOption = 0x05; // both prefix and witness hash


                    // Number of inputs
                    btchip_context_D.transactionContext
                        .transactionRemainingInputsOutputs =
                        transaction_get_varint();
                    btchip_context_D.transactionHashOption = TRANSACTION_HASH_FULL;

                    // Ready to proceed
                    btchip_context_D.transactionContext.transactionState =
                        BTCHIP_TRANSACTION_DEFINED_WAIT_INPUT;

                    // no break is intentional
                }

                case BTCHIP_TRANSACTION_DEFINED_WAIT_INPUT: {
                    unsigned char trustedInputFlag = 1;
                    PRINTF("Process input\n");
                    if (btchip_context_D.transactionContext
                            .transactionRemainingInputsOutputs == 0) {
                        // No more inputs to hash, move forward
                        btchip_context_D.transactionContext.transactionState =
                            BTCHIP_TRANSACTION_INPUT_HASHING_DONE;
                        continue;
                    }
                    if (btchip_context_D.transactionDataRemaining < 1) {
                        // No more data to read, ok
                        PRINTF("Waiting for more data...\n");
                        goto ok;
                    }
                    // Proceed with the next input
                    if (parseMode == PARSE_MODE_TRUSTED_INPUT) {
                        PRINTF("PARSE_MODE_TRUSTED_INPUT\n");
                        check_transaction_available(
                            37); // prevout : 32 hash + 4 index + 1 tree
                        transaction_offset_increase(37);
                    }
                    if (parseMode == PARSE_MODE_SIGNATURE) {
                        PRINTF("PARSE_MODE_SIGNATURE\n");
                        unsigned char trustedInputLength;
                        unsigned char trustedInput[0x38];
                        unsigned char amount[8];
                        unsigned char *savePointer;

                        // Expect the trusted input flag and trusted input
                        // length
                        check_transaction_available(2);
                        switch (*btchip_context_D.transactionBufferPointer) {
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
                        *(btchip_context_D.transactionBufferPointer + 1);
                        if (trustedInputLength > sizeof(trustedInput)) {
                          PRINTF("Trusted input too long\n");
                          goto fail;
                        }
                        */

                        if (!trustedInputFlag) {

                            if (!optionP2SHSkip2FA) {
                                PRINTF("Untrusted input not authorized\n");
                                goto fail;
                            }

                            btchip_context_D.transactionBufferPointer++;
                            btchip_context_D.transactionDataRemaining--;
                            check_transaction_available(
                                36); // prevout : 32 hash + 4 index
                            transaction_offset_increase(36);
                            PRINTF("Marking relaxed input\n");
                            btchip_context_D.transactionContext.relaxed = 1;
                            /*
                            PRINTF("Clearing P2SH consumption\n");
                            btchip_context_D.transactionContext.consumeP2SH = 0;
                            */
                        } else {
                            trustedInputLength = *(
                                btchip_context_D.transactionBufferPointer + 1);
                            if ((trustedInputLength > sizeof(trustedInput)) ||
                                (trustedInputLength < 8)) {
                                PRINTF("Invalid trusted input size\n");
                                goto fail;
                            }

                            check_transaction_available(2 + trustedInputLength);
                            cx_hmac_sha256(
                                (const uint8_t *) N_btchip.bkp.trustedinput_key,
                                sizeof(N_btchip.bkp.trustedinput_key),
                                btchip_context_D.transactionBufferPointer + 2,
                                trustedInputLength - 8, trustedInput, trustedInputLength);
                            if (btchip_secure_memcmp(
                                    trustedInput,
                                    btchip_context_D.transactionBufferPointer +
                                        2 + trustedInputLength - 8,
                                    8) != 0) {
                                PRINTF("Invalid signature\n");
                                goto fail;
                            }
                            memmove(
                                trustedInput,
                                btchip_context_D.transactionBufferPointer + 2,
                                trustedInputLength - 8);
                            if (trustedInput[0] != MAGIC_TRUSTED_INPUT) {
                                PRINTF("Failed to verify trusted input signature\n");
                                goto fail;
                            }
                            else {
                                PRINTF("Good Signature\n");
                            }

                            // Update the hash with prevout data
                            savePointer =
                                btchip_context_D.transactionBufferPointer; //trusted input 01
                            /*
                            // Check if a P2SH script is used
                            if ((trustedInput[1] & FLAG_TRUSTED_INPUT_P2SH) ==
                            0) {
                              PRINTF("Clearing P2SH consumption\n");
                              btchip_context_D.transactionContext.consumeP2SH =
                            0;
                            }
                            */
                            btchip_context_D.transactionBufferPointer =
                                trustedInput + 4;
                            PRINTF("Trusted input hash\n%.*H\n",36,btchip_context_D.transactionBufferPointer);
                            transaction_offset(36);

                            btchip_context_D.transactionBufferPointer =
                                savePointer + (2 + trustedInputLength);
                            btchip_context_D.transactionDataRemaining -=
                                (2 + trustedInputLength);

                            // add Decred tree
                            check_transaction_available(1);
                            transaction_offset_increase(1);

                            // Update the amount

                            btchip_swap_bytes(amount, trustedInput + 40, 8);
                            if (transaction_amount_add_be(
                                    btchip_context_D.transactionContext
                                        .transactionAmount,
                                    btchip_context_D.transactionContext
                                        .transactionAmount,
                                    amount)) {
                                PRINTF("Overflow\n");
                                goto fail;
                            }

                            PRINTF("Adding amount\n%.*H\n",8,(trustedInput + 40));
                            PRINTF("New amount\n%.*H\n",8,btchip_context_D.transactionContext.transactionAmount);
                        }


                    }
                    // DIRTY: include utxo script len and script in witness hash
                    btchip_context_D.transactionHashOption = TRANSACTION_HASH_WITNESS;

                    // Read the script length
                    btchip_context_D.transactionContext.scriptRemaining =
                        transaction_get_varint();
                    PRINTF("Script to read " DEBUG_LONG "\n",btchip_context_D.transactionContext.scriptRemaining);

                    if ((parseMode == PARSE_MODE_SIGNATURE) &&
                        !trustedInputFlag) {
                        // Only proceeds if this is not to be signed - so length
                        // should be null
                        if (btchip_context_D.transactionContext
                                .scriptRemaining != 0) {
                            PRINTF("Request to sign relaxed input\n");
                            if (!optionP2SHSkip2FA) {
                                goto fail;
                            }
                        }
                    }
                    PRINTF("NEXT_STATE\n");
                    // Move on
                    btchip_context_D.transactionContext.transactionState =
                        BTCHIP_TRANSACTION_INPUT_HASHING_IN_PROGRESS_INPUT_SCRIPT;

                    // no break is intentional
                }
                case BTCHIP_TRANSACTION_INPUT_HASHING_IN_PROGRESS_INPUT_SCRIPT: {
                    unsigned char dataAvailable;
                    PRINTF("Process input script, remaining " DEBUG_LONG "\n",btchip_context_D.transactionContext.scriptRemaining);
                    if (btchip_context_D.transactionDataRemaining < 1) {
                        // No more data to read, ok
                        goto ok;
                    }
                    // Scan for P2SH consumption - huge shortcut, but fine
                    // enough
                    // Also usable in SegWit mode
                    if (btchip_context_D.transactionContext.scriptRemaining ==
                        1) {
                        if (*btchip_context_D.transactionBufferPointer ==
                            OP_CHECKMULTISIG) {
                            if (optionP2SHSkip2FA) {
                                PRINTF("Marking P2SH consumption\n");
                                btchip_context_D.transactionContext
                                    .consumeP2SH = 1;
                            }
                        } else {
                            // When using the P2SH shortcut, all inputs must use
                            // P2SH
                            PRINTF("Disabling P2SH consumption\n");
                            btchip_context_D.transactionContext.consumeP2SH = 0;
                        }
                        transaction_offset_increase(1);
                        btchip_context_D.transactionContext.scriptRemaining--;
                    }

                    if (btchip_context_D.transactionContext.scriptRemaining ==
                        0) {
                        // restore full hash
                        btchip_context_D.transactionHashOption = 0x01;

                        if (parseMode == PARSE_MODE_SIGNATURE) {
                            btchip_context_D.transactionHashOption = TRANSACTION_HASH_BOTH;
                        }
                        // Sequence
                        check_transaction_available(4);
                        transaction_offset_increase(4);
                        // Move to next input
                        btchip_context_D.transactionContext
                            .transactionRemainingInputsOutputs--;
                        btchip_context_D.transactionContext
                            .transactionCurrentInputOutput++;
                        btchip_context_D.transactionContext.transactionState =
                            BTCHIP_TRANSACTION_DEFINED_WAIT_INPUT;
                        continue;
                    }
                    // Save the last script byte for the P2SH check
                    dataAvailable =
                        (btchip_context_D.transactionDataRemaining >
                                 btchip_context_D.transactionContext
                                         .scriptRemaining -
                                     1
                             ? btchip_context_D.transactionContext
                                       .scriptRemaining -
                                   1
                             : btchip_context_D.transactionDataRemaining);
                    if (dataAvailable == 0) {
                        goto ok;
                    }
                    transaction_offset_increase(dataAvailable);
                    btchip_context_D.transactionContext.scriptRemaining -=
                        dataAvailable;
                    break;
                }
                case BTCHIP_TRANSACTION_INPUT_HASHING_DONE: {
                    PRINTF("Input hashing done\n");
                    if (parseMode == PARSE_MODE_SIGNATURE) {

                        btchip_context_D.transactionContext
                            .transactionState =
                            BTCHIP_TRANSACTION_PRESIGN_READY;
                        continue;
                    }
                    if (btchip_context_D.transactionDataRemaining < 1) {
                        // No more data to read, ok
                        goto ok;
                    }
                    // Number of outputs
                    btchip_context_D.transactionContext
                        .transactionRemainingInputsOutputs =
                        transaction_get_varint();
                    btchip_context_D.transactionContext
                        .transactionCurrentInputOutput = 0;
                    PRINTF("Number of outputs : " DEBUG_LONG "\n", btchip_context_D.transactionContext                                     .transactionRemainingInputsOutputs);
                    // Ready to proceed
                    btchip_context_D.transactionContext.transactionState =
                        BTCHIP_TRANSACTION_DEFINED_WAIT_OUTPUT;

                    // no break is intentional
                }
                case BTCHIP_TRANSACTION_DEFINED_WAIT_OUTPUT: {
                    if (btchip_context_D.transactionContext
                            .transactionRemainingInputsOutputs == 0) {
                        // No more outputs to hash, move forward
                        btchip_context_D.transactionContext.transactionState =
                            BTCHIP_TRANSACTION_OUTPUT_HASHING_DONE;
                        continue;
                    }
                    if (btchip_context_D.transactionDataRemaining < 1) {
                        // No more data to read, ok
                        goto ok;
                    }
                    // Amount
                    check_transaction_available(8);
                    if ((parseMode == PARSE_MODE_TRUSTED_INPUT) &&
                        (btchip_context_D.transactionContext
                             .transactionCurrentInputOutput ==
                         btchip_context_D.transactionTargetInput)) {
                        // Save the amount
                        memmove(btchip_context_D.transactionContext
                                       .transactionAmount,
                                   btchip_context_D.transactionBufferPointer,
                                   8);
                        btchip_context_D.trustedInputProcessed = 1;
                        PRINTF("Input processed\n");
                    }
                    transaction_offset_increase(8);

                    // script version
                    check_transaction_available(2);
                    transaction_offset_increase(2);

                    // Read the script length
                    btchip_context_D.transactionContext.scriptRemaining =
                        transaction_get_varint();

                    PRINTF("Script to read " DEBUG_LONG "\n",btchip_context_D.transactionContext.scriptRemaining);
                    // Move on
                    btchip_context_D.transactionContext.transactionState =
                        BTCHIP_TRANSACTION_OUTPUT_HASHING_IN_PROGRESS_OUTPUT_SCRIPT;

                    // no break is intentional
                }
                case BTCHIP_TRANSACTION_OUTPUT_HASHING_IN_PROGRESS_OUTPUT_SCRIPT: {
                    unsigned char dataAvailable;
                    PRINTF("Process output script, remaining " DEBUG_LONG "\n",btchip_context_D.transactionContext.scriptRemaining);
                    /*
                    // Special check if consuming a P2SH script
                    if (parseMode == PARSE_MODE_TRUSTED_INPUT) {
                      // Assume the full input script is sent in a single APDU,
                    then do the ghetto validation
                      if ((btchip_context_D.transactionBufferPointer[0] ==
                    OP_HASH160) &&
                          (btchip_context_D.transactionBufferPointer[btchip_context_D.transactionDataRemaining
                    - 1] == OP_EQUAL)) {
                        PRINTF("Marking P2SH output\n");
                        btchip_context_D.transactionContext.consumeP2SH = 1;
                      }
                    }
                    */
                    if (btchip_context_D.transactionDataRemaining < 1) {
                        // No more data to read, ok
                        goto ok;
                    }
                    if (btchip_context_D.transactionContext.scriptRemaining ==
                        0) {
                        // Move to next output
                        btchip_context_D.transactionContext
                            .transactionRemainingInputsOutputs--;
                        btchip_context_D.transactionContext
                            .transactionCurrentInputOutput++;
                        btchip_context_D.transactionContext.transactionState =
                            BTCHIP_TRANSACTION_DEFINED_WAIT_OUTPUT;
                        continue;
                    }
                    dataAvailable =
                        (btchip_context_D.transactionDataRemaining >
                                 btchip_context_D.transactionContext
                                     .scriptRemaining
                             ? btchip_context_D.transactionContext
                                   .scriptRemaining
                             : btchip_context_D.transactionDataRemaining);
                    if (dataAvailable == 0) {
                        goto ok;
                    }
                    transaction_offset_increase(dataAvailable);
                    btchip_context_D.transactionContext.scriptRemaining -=
                        dataAvailable;
                    break;
                }
                case BTCHIP_TRANSACTION_OUTPUT_HASHING_DONE: {
                    PRINTF("Output hashing done\n");
                    if (btchip_context_D.transactionDataRemaining < 1) {
                        // No more data to read, ok
                        goto ok;
                    }
                    // Locktime + Expiration
                    check_transaction_available(4+4);
                    transaction_offset_increase(4+4);

                    if (btchip_context_D.transactionDataRemaining == 0) {
                        btchip_context_D.transactionContext.transactionState =
                            BTCHIP_TRANSACTION_PARSED;
                        continue;
                    } else {
                        btchip_context_D.transactionHashOption = 0;
                        btchip_context_D.transactionContext.scriptRemaining =
                            transaction_get_varint();
                        btchip_context_D.transactionHashOption =
                            TRANSACTION_HASH_FULL;
                        btchip_context_D.transactionContext.transactionState =
                            BTCHIP_TRANSACTION_PROCESS_EXTRA;
                        continue;
                    }
                }

                case BTCHIP_TRANSACTION_PROCESS_EXTRA: {
                    unsigned char dataAvailable;

                    if (btchip_context_D.transactionContext.scriptRemaining ==
                        0) {
                        btchip_context_D.transactionContext.transactionState =
                            BTCHIP_TRANSACTION_PARSED;
                        continue;
                    }

                    if (btchip_context_D.transactionDataRemaining < 1) {
                        // No more data to read, ok
                        goto ok;
                    }

                    dataAvailable =
                        (btchip_context_D.transactionDataRemaining >
                                 btchip_context_D.transactionContext
                                     .scriptRemaining
                             ? btchip_context_D.transactionContext
                                   .scriptRemaining
                             : btchip_context_D.transactionDataRemaining);
                    if (dataAvailable == 0) {
                        goto ok;
                    }
                    transaction_offset_increase(dataAvailable);
                    btchip_context_D.transactionContext.scriptRemaining -=
                        dataAvailable;
                    break;
                }

                case BTCHIP_TRANSACTION_PARSED: {
                    PRINTF("Transaction parsed\n");
                    goto ok;
                }

                case BTCHIP_TRANSACTION_PRESIGN_READY: {
                    PRINTF("Presign ready\n");
                    goto ok;
                }

                case BTCHIP_TRANSACTION_SIGN_READY: {
                    PRINTF("Sign ready\n");
                    goto ok;
                }
                }
            }

        fail:
            PRINTF("Transaction parse - fail\n");
            THROW(EXCEPTION);
        ok : {}
        }
        CATCH_OTHER(e) {
            PRINTF("Transaction parse - surprise fail\n");
            btchip_context_D.transactionContext.transactionState =
                BTCHIP_TRANSACTION_NONE;
            btchip_set_check_internal_structure_integrity(1);
            THROW(e);
        }
        // before the finally to restore the surrounding context if an exception
        // is raised during finally
        FINALLY {
            btchip_set_check_internal_structure_integrity(1);
        }
    }
    END_TRY;
}
