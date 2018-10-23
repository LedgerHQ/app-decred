/*******************************************************************************
*   Ledger Blue - Bitcoin Wallet
*   (c) 2016 Ledger
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

// TODO Trustlet, BAGL : process each output separately.
// review nvm_write policy

#include "internal.h"
#include "apdu_constants.h"
#include "bagl_extensions.h"

#define FINALIZE_P1_MORE 0x00
#define FINALIZE_P1_LAST 0x80
#define FINALIZE_P1_CHANGEINFO 0xFF

#define FLAG_SIGNATURE 0x01
#define FLAG_CHANGE_VALIDATED 0x80

extern uint8_t prepare_full_output(uint8_t checkOnly);

static void apdu_hash_input_finalize_full_reset(void)
{
    context_D.currentOutputOffset = 0;
    context_D.outputParsingState = OUTPUT_PARSING_NUMBER_OUTPUTS;
    os_memset(context_D.totalOutputAmount, 0,
              sizeof(context_D.totalOutputAmount));
    context_D.changeOutputFound = 0;
    set_check_internal_structure_integrity(1);
}

static bool check_output_displayable()
{
    PRINTF("Check if output is displayable\n");
    bool displayable = true;
    unsigned char amount[8], isOpReturn, isP2sh, j,nullAmount = 1;
    unsigned char isOpCreate, isOpCall;

    for (j = 0; j < 8; j++)
    {
        if (context_D.currentOutput[j] != 0)
        {
            nullAmount = 0;
            break;
        }
    }
    if (!nullAmount)
    {
        swap_bytes(amount, context_D.currentOutput, 8);
        transaction_amount_add_be(context_D.totalOutputAmount,
                                  context_D.totalOutputAmount, amount);
    }
    isOpReturn =
        output_script_is_op_return(context_D.currentOutput + 8 + 2); // +2 for script version, decred particularity
    isP2sh = output_script_is_p2sh(context_D.currentOutput + 8 + 2);
    isOpCreate =
        output_script_is_op_create(context_D.currentOutput + 8 + 2);
    isOpCall =
        output_script_is_op_call(context_D.currentOutput + 8 + 2);

    if (!output_script_is_regular(context_D.currentOutput + 8 + 2) &&
         !isP2sh && !(nullAmount && isOpReturn))
    {
        PRINTF("Error: Unrecognized input script\n");
        THROW(EXCEPTION);
    }
    if (context_D.tmpCtx.output.changeInitialized && !isOpReturn)
    {
        bool changeFound = false;
        unsigned char addressOffset =
            (isP2sh ? OUTPUT_SCRIPT_P2SH_PRE_LENGTH
                                     : OUTPUT_SCRIPT_REGULAR_PRE_LENGTH);
        if (!isP2sh &&
            os_memcmp(context_D.currentOutput + 8 + 2 + addressOffset,
                      context_D.tmpCtx.output.changeAddress + 1,
                      20) == 0)
        {
            changeFound = true;
        }
        
        if (changeFound)
        {
            if (context_D.changeOutputFound)
            {
                PRINTF("Error : Multiple change output found\n");
                THROW(EXCEPTION);
            }
            context_D.changeOutputFound = true;
            displayable = false;
        }
    }
    PRINTF(displayable ? "displayable\n" : "non displayable\n");
    return displayable;
}

static bool handle_output_state()
{
    uint32_t discardSize = 0;
    context_D.discardSize = 0;
    bool processed = false;
    switch (context_D.outputParsingState)
    {
    case OUTPUT_PARSING_NUMBER_OUTPUTS:
    {
        // this is default init state
        context_D.totalOutputs = 0;
        if (context_D.currentOutputOffset < 1)
        {
            break;
        }
        if (context_D.currentOutput[0] < 0xFD)
        {
            context_D.totalOutputs = context_D.remainingOutputs =
                context_D.currentOutput[0];
            discardSize = 1;
            context_D.outputParsingState = OUTPUT_PARSING_OUTPUT;
            processed = true;
            break;
        }
        if (context_D.currentOutput[0] == 0xFD)
        {
            if (context_D.currentOutputOffset < 3)
            {
                break;
            }
            context_D.totalOutputs = context_D.remainingOutputs =
                (context_D.currentOutput[2] << 8) |
                context_D.currentOutput[1];
            discardSize = 3;
            context_D.outputParsingState = OUTPUT_PARSING_OUTPUT;
            processed = true;
            break;
        }
        else if (context_D.currentOutput[0] == 0xFE)
        {
            if (context_D.currentOutputOffset < 5)
            {
                break;
            }
            context_D.totalOutputs = context_D.remainingOutputs =
                read_u32(context_D.currentOutput + 1, 0, 0);
            discardSize = 5;
            context_D.outputParsingState = OUTPUT_PARSING_OUTPUT;
            processed = true;
            break;
        }
        else
        {
            THROW(EXCEPTION);
        }
    }
    break;

    case OUTPUT_PARSING_OUTPUT:
    {
        PRINTF("OUTPUT_PARSING_OUTPUT:\n");

        unsigned int scriptSize;
        if (context_D.currentOutputOffset < 9)
        {
            break;
        }
        if (context_D.currentOutput[8+2] < 0xFD)
        {
            scriptSize = context_D.currentOutput[8+2];
            discardSize = 1;
        }
        else if (context_D.currentOutput[8+2] == 0xFD)
        {
            if (context_D.currentOutputOffset < 9 + 2)
            {
                break;
            }
            scriptSize =
                read_u32(context_D.currentOutput + 9, 0, 0);
            discardSize = 3;
        }
        else
        {
            // Unrealistically large script
            THROW(EXCEPTION);
        }
        if (context_D.currentOutputOffset <
            8 + 2 + discardSize + scriptSize)
        {
            discardSize = 0;
            break;
        }

        processed = true;

        discardSize += 8 + 2 + scriptSize;

        if (check_output_displayable())
        {
            context_D.io_flags |= IO_ASYNCH_REPLY;

            // The output can be processed by the UI

            context_D.discardSize = discardSize;
            discardSize = 0;
        }
        else
        {
            context_D.remainingOutputs--;
            PRINTF("%d remaining outputs\n", context_D.remainingOutputs);
        }
    }
    break;

    default:
        THROW(EXCEPTION);
    }

    if (discardSize != 0)
    {
        PRINTF("discard %d bytes from output\n", discardSize);
        os_memmove(context_D.currentOutput,
                   context_D.currentOutput + discardSize,
                   context_D.currentOutputOffset - discardSize);
        context_D.currentOutputOffset -= discardSize;
    }

    PRINTF("Processed: %d\n", processed);
    return processed;
}

unsigned short apdu_hash_input_finalize_full_internal(
    transaction_summary_t *transactionSummary)
{
    PRINTF("\n###  HASH_INPUT_FINALIZE:\n");

    unsigned char authorizationHash[32];
    unsigned char apduLength;
    unsigned short sw = SW_OK;
    unsigned char *target = G_io_apdu_buffer;
    unsigned char keycardActivated = 0;
    unsigned char screenPaired = 0;
    unsigned char deepControl = 0;
    unsigned char p1 = G_io_apdu_buffer[ISO_OFFSET_P1];
    unsigned char persistentCommit = 0;
    unsigned char hashOffset = 0;
    unsigned char numOutputs = 0;

    apduLength = G_io_apdu_buffer[ISO_OFFSET_LC];

    if ((p1 != FINALIZE_P1_MORE) && (p1 != FINALIZE_P1_LAST) &&
        (p1 != FINALIZE_P1_CHANGEINFO))
    {
        return SW_INCORRECT_P1_P2;
    }

    // Check state
    BEGIN_TRY
    {
        TRY
        {
            set_check_internal_structure_integrity(0);
            if (context_D.transactionContext.transactionState !=
                TRANSACTION_PRESIGN_READY)
            {
                sw = SW_CONDITIONS_OF_USE_NOT_SATISFIED;
                goto discardTransaction;
            }

            if (p1 == FINALIZE_P1_CHANGEINFO)
            {
                /* derive change addr and store path and values in btchip context*/
                unsigned char keyLength;
                if (!context_D.transactionContext.firstSigned)
                {
                // Already validated, should be prevented on the client side
                return_OK:
                    CLOSE_TRY;
                    return SW_OK;
                }
                if (!context_D.tmpCtx.output.changeAccepted)
                {
                    sw = SW_CONDITIONS_OF_USE_NOT_SATISFIED;
                    goto discardTransaction;
                }
                os_memset(transactionSummary, 0,
                          sizeof(transaction_summary_t));
                if (G_io_apdu_buffer[ISO_OFFSET_CDATA] == 0x00)
                {
                    // Called with no change path, abort, should be prevented on
                    // the client side
                    goto return_OK;
                }
                os_memmove(transactionSummary->summarydata.keyPath,
                           G_io_apdu_buffer + ISO_OFFSET_CDATA,
                           MAX_BIP32_PATH_LENGTH);
                private_derive_keypair(
                    transactionSummary->summarydata.keyPath, 1, NULL);
                if (((N_btchip.bkp.config.options &
                      OPTION_UNCOMPRESSED_KEYS) != 0))
                {
                    keyLength = 65;
                }
                else
                {
                    compress_public_key_value(public_key_D.W);
                    keyLength = 33;
                }
                public_key_hash160(
                    public_key_D.W,                            // IN
                    keyLength,                                        // INLEN
                    transactionSummary->summarydata.changeAddress + 1 // OUT
                );
                os_memmove(
                    context_D.tmpCtx.output.changeAddress,
                    transactionSummary->summarydata.changeAddress,
                    sizeof(transactionSummary->summarydata.changeAddress));

                context_D.tmpCtx.output.changeInitialized = 1;
                context_D.tmpCtx.output.changeAccepted = 0;
                goto return_OK;
            }

            // Always update the transaction & authorization hashes with the
            // given data

            PRINTF("Adding to prefix hash:\n%.*H\n", apduLength - hashOffset, G_io_apdu_buffer + ISO_OFFSET_CDATA + hashOffset);
            blake256_Update(&context_D.transactionHashPrefix, G_io_apdu_buffer + ISO_OFFSET_CDATA + hashOffset, apduLength - hashOffset);
            blake256_Update(&context_D.transactionHashAuthorization, G_io_apdu_buffer + ISO_OFFSET_CDATA + hashOffset, apduLength - hashOffset);
        

            if (context_D.transactionContext.firstSigned)
            {
                if ((context_D.currentOutputOffset + apduLength) >
                    sizeof(context_D.currentOutput))
                {
                    sw = SW_INCORRECT_DATA;
                    goto discardTransaction;
                }
                os_memmove(context_D.currentOutput +
                                context_D.currentOutputOffset,
                            G_io_apdu_buffer + ISO_OFFSET_CDATA, apduLength);
                context_D.currentOutputOffset += apduLength;

                // Check if the legacy UI can be applied
                if (!(G_coin_config->flags & FLAG_QTUM_SUPPORT) &&
                    (G_io_apdu_buffer[ISO_OFFSET_P1] == FINALIZE_P1_LAST) &&
                    !context_D.tmpCtx.output.multipleOutput &&
                    prepare_full_output(1))
                {
                    context_D.io_flags |= IO_ASYNCH_REPLY;
                    context_D.outputParsingState =
                        OUTPUT_HANDLE_LEGACY;
                    context_D.remainingOutputs = 0;
                }
                else
                {
                    // parse outputs until one can be displayed
                    while (handle_output_state() &&
                            (!(context_D.io_flags & IO_ASYNCH_REPLY)))
                        ;

                    // Finalize the TX if necessary

                    if ((context_D.remainingOutputs == 0) &&
                        (!(context_D.io_flags & IO_ASYNCH_REPLY)))
                    {
                        context_D.io_flags |= IO_ASYNCH_REPLY;
                        context_D.outputParsingState =
                            OUTPUT_FINALIZE_TX;
                    }
                }
            }

            if (G_io_apdu_buffer[ISO_OFFSET_P1] == FINALIZE_P1_MORE)
            {
                G_io_apdu_buffer[0] = 0x00;
                context_D.outLength = 1;
                context_D.tmpCtx.output.multipleOutput = 1;
                goto return_OK;
            }

            /* Computes an intermediary hash of the txId that will be checked for each 
            successive inputs to sign to check that they belong to the same tx*/
  
            blake256_Final(&context_D.transactionHashAuthorization, authorizationHash);


            if(context_D.transactionContext.firstSigned)
            {
                if (!context_D.tmpCtx.output.changeInitialized)
                {
                    os_memset(transactionSummary, 0,
                                sizeof(transaction_summary_t));
                }

                transactionSummary->payToAddressVersion =
                    context_D.payToAddressVersion;
                transactionSummary->payToScriptHashVersion =
                    context_D.payToScriptHashVersion;

                // Generate new nonce

                cx_rng(transactionSummary->summarydata.transactionNonce, 8);
            }

            G_io_apdu_buffer[0] = 0x00;
            target++;

            *target = 0x00;
            target++;

            context_D.outLength = (target - G_io_apdu_buffer);

            
            // Check that the input being signed is part of the same
            // transaction, otherwise abort
            // (this is done to keep the transaction counter limit per session
            // synchronized)
            if (context_D.transactionContext.firstSigned)
            {
                os_memmove(transactionSummary->authorizationHash,
                           authorizationHash,
                           sizeof(transactionSummary->authorizationHash));
            goto return_OK;
            }
            else
            {
                if (secure_memcmp(
                        authorizationHash,
                        transactionSummary->authorizationHash,
                        sizeof(transactionSummary->authorizationHash)))
                {
                    PRINTF("Authorization hash doesn't match the previous one\n");
                    sw = SW_CONDITIONS_OF_USE_NOT_SATISFIED;
                
discardTransaction:
                    CLOSE_TRY;
                    goto catch_discardTransaction;
                }
                else
                {
                    PRINTF("Authorization Hash OK:\n%.*H\n", 32, authorizationHash);
                }
            }

            context_D.transactionContext.transactionState =
                TRANSACTION_SIGN_READY;

            sw = SW_OK;
        }
        CATCH_ALL
        {
            sw = SW_TECHNICAL_DETAILS(0x0F);
        catch_discardTransaction:
            context_D.transactionContext.transactionState =
                TRANSACTION_NONE;
            context_D.outLength = 0;

            os_memmove(G_io_apdu_buffer, context_D.currentOutput,
                       context_D.currentOutputOffset);
            context_D.outLength = context_D.currentOutputOffset;
        }
        FINALLY
        {
            apdu_hash_input_finalize_full_reset();
            return sw;
        }
    }
    END_TRY;
}

unsigned short apdu_hash_input_finalize_full()
{
    unsigned short sw = apdu_hash_input_finalize_full_internal(
        &context_D.transactionSummary);
    if (context_D.io_flags & IO_ASYNCH_REPLY)
    {
        // if the UI reject the processing of the request, then reply
        // immediately
        bool status;
        if (context_D.outputParsingState == OUTPUT_FINALIZE_TX)
        {   
            PRINTF("BAGL finalize tx:\n");
            status = bagl_finalize_tx();
        }
        else if (context_D.outputParsingState ==
                 OUTPUT_HANDLE_LEGACY)
        {
            PRINTF("BAGL confirm output legacy:\n");
            status = bagl_confirm_full_output();
        }
        else
        {
            PRINTF("BAGL confirm single output:\n");
            status = bagl_confirm_single_output();
        }
        if (!status)
        {
            context_D.io_flags &= ~IO_ASYNCH_REPLY;
            context_D.transactionContext.transactionState =
                TRANSACTION_NONE;
            context_D.outLength = 0;
            sw = SW_INCORRECT_DATA;
        }
    }
    return sw;
}

unsigned char bagl_user_action(unsigned char confirming)
{
    unsigned short sw = SW_OK;
    // confirm and finish the apdu exchange //spaghetti
    if (confirming)
    {
        // Check if all inputs have been confirmed
        if (context_D.outputParsingState ==
            OUTPUT_PARSING_OUTPUT)
        {
            context_D.remainingOutputs--;
            PRINTF("%d remaining outputs\n", context_D.remainingOutputs);
        }

        while (context_D.remainingOutputs != 0)
        {
            os_memmove(context_D.currentOutput,
                       context_D.currentOutput +
                           context_D.discardSize,
                       context_D.currentOutputOffset -
                           context_D.discardSize);
            context_D.currentOutputOffset -=
                context_D.discardSize;
            context_D.io_flags &= ~IO_ASYNCH_REPLY;
            while (handle_output_state() &&
                   (!(context_D.io_flags & IO_ASYNCH_REPLY)))
                ;
            if (context_D.io_flags & IO_ASYNCH_REPLY)
            {
                if (!bagl_confirm_single_output())
                {
                    context_D.transactionContext.transactionState =
                        TRANSACTION_NONE;
                    sw = SW_INCORRECT_DATA;
                    break;
                }
                else
                {
                    // Let the UI play
                    return 1;
                }
            }
            else
            {
                // Out of data to process, wait for the next call
                break;
            }
        }

        if ((context_D.outputParsingState ==
             OUTPUT_PARSING_OUTPUT) &&
            (context_D.remainingOutputs == 0))
        {
            context_D.outputParsingState = OUTPUT_FINALIZE_TX;
            if (!bagl_finalize_tx())
            {
                context_D.outputParsingState =
                    OUTPUT_PARSING_NONE;
                context_D.transactionContext.transactionState =
                    TRANSACTION_NONE;
                sw = SW_INCORRECT_DATA;
            }
            else
            {
                // Let the UI play
                return 1;
            }
        } 

        if ((context_D.outputParsingState ==
             OUTPUT_FINALIZE_TX) ||
            (context_D.outputParsingState ==
             OUTPUT_HANDLE_LEGACY))
        {
            context_D.transactionContext.firstSigned = 0;

            context_D.transactionContext.transactionState =
                TRANSACTION_SIGN_READY;
            
        }
        context_D.outLength -=
            2; // status was already set by the last call
    }
    else
    {
        // Discard transaction
        context_D.transactionContext.transactionState =
            TRANSACTION_NONE;
        sw = SW_CONDITIONS_OF_USE_NOT_SATISFIED;
        context_D.outLength = 0;
    }
    G_io_apdu_buffer[context_D.outLength++] = sw >> 8;
    G_io_apdu_buffer[context_D.outLength++] = sw;

    if ((context_D.outputParsingState == OUTPUT_FINALIZE_TX) ||
        (context_D.outputParsingState == OUTPUT_HANDLE_LEGACY) ||
        (sw != SW_OK))
    {
        // we've finished the processing of the input
        apdu_hash_input_finalize_full_reset();
    }

    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, context_D.outLength);

    return 0;
}
