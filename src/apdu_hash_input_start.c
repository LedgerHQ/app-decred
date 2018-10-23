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

#include "internal.h"
#include "apdu_constants.h"

#define P1_FIRST 0x00
#define P1_NEXT 0x80
#define P2_NEW 0x00
#define P2_CONTINUE 0x80


unsigned short apdu_hash_input_start()
{
    unsigned char apduLength;
    apduLength = G_io_apdu_buffer[ISO_OFFSET_LC];

    PRINTF("\n### HASH_INPUT_START:\n");

    SB_CHECK(N_btchip.bkp.config.operationMode);
    switch (SB_GET(N_btchip.bkp.config.operationMode))
    {
    case MODE_WALLET:
    case MODE_RELAXED_WALLET:
    case MODE_SERVER:
        break;
    default:
        return SW_CONDITIONS_OF_USE_NOT_SATISFIED;
    }

    if (G_io_apdu_buffer[ISO_OFFSET_P1] == P1_FIRST)
    {
        // Initialize
        context_D.transactionContext.transactionState =
            TRANSACTION_NONE;
        set_check_internal_structure_integrity(1);
        context_D.transactionHashOption = TRANSACTION_HASH_BOTH;
    }
    else if (G_io_apdu_buffer[ISO_OFFSET_P1] != P1_NEXT)
    {
        return SW_INCORRECT_P1_P2;
    }

    if (G_io_apdu_buffer[ISO_OFFSET_P2] == P2_NEW)
    {
        // context_D.transactionContext.consumeP2SH =
        // ((N_btchip.bkp.config.options & OPTION_SKIP_2FA_P2SH) != 0);
        if (G_io_apdu_buffer[ISO_OFFSET_P1] == P1_FIRST)
        {
            // Request PIN validation
            // Only request PIN validation (user presence) to start a new
            // transaction signing flow.
            // Thus allowing for numerous output to be processed in the
            // background without
            // requiring to disable autolock/autopoweroff
            if (!context_D.transactionContext.firstSigned &&
                !os_global_pin_is_validated())
            {
                return SW_SECURITY_STATUS_NOT_SATISFIED;
            }
            // Master transaction reset
            context_D.transactionContext.firstSigned = 1;
            context_D.transactionContext.consumeP2SH = 0;
            context_D.transactionContext.relaxed = 0;
            set_check_internal_structure_integrity(1);
            // Initialize for screen pairing
            os_memset(&context_D.tmpCtx.output, 0,
                      sizeof(context_D.tmpCtx.output));
            context_D.tmpCtx.output.changeAccepted = 1;
        }
    }
    else if (G_io_apdu_buffer[ISO_OFFSET_P2] != P2_CONTINUE)
    {
        return SW_INCORRECT_P1_P2;
    }

    context_D.transactionBufferPointer =
        G_io_apdu_buffer + ISO_OFFSET_CDATA;
    context_D.transactionDataRemaining = apduLength;

    transaction_parse(PARSE_MODE_SIGNATURE);

    return SW_OK;
}
