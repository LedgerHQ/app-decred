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

#define GET_TRUSTED_INPUT_P1_FIRST 0x00
#define GET_TRUSTED_INPUT_P1_NEXT 0x80

#define TRUSTEDINPUT_SIZE 48

unsigned short apdu_get_trusted_input()
{
    unsigned char apduLength;
    unsigned char dataOffset = 0;
    unsigned char trustedInputSignature[32];
    cx_sha256_t hash;
    apduLength = G_io_apdu_buffer[ISO_OFFSET_LC];

    SB_CHECK(N_btchip.bkp.config.operationMode);
    switch (SB_GET(N_btchip.bkp.config.operationMode))
    {
    case MODE_WALLET:
        break;
    default:
        return SW_CONDITIONS_OF_USE_NOT_SATISFIED;
    }

    if (G_io_apdu_buffer[ISO_OFFSET_P1] == GET_TRUSTED_INPUT_P1_FIRST)
    {
        // Initialize
        context_D.transactionTargetInput =
            read_u32(G_io_apdu_buffer + ISO_OFFSET_CDATA, 1, 0);
        context_D.transactionContext.transactionState =
            TRANSACTION_NONE;
        context_D.trustedInputProcessed = 0;
        context_D.transactionContext.consumeP2SH = 0;
        set_check_internal_structure_integrity(1);
        dataOffset = 4;
        context_D.transactionHashOption = TRANSACTION_HASH_FULL;
    }
    else if (G_io_apdu_buffer[ISO_OFFSET_P1] != GET_TRUSTED_INPUT_P1_NEXT)
    {
        return SW_INCORRECT_P1_P2;
    }

    if (G_io_apdu_buffer[ISO_OFFSET_P2] != 0x00)
    {
        return SW_INCORRECT_P1_P2;
    }
    context_D.transactionBufferPointer =
        G_io_apdu_buffer + ISO_OFFSET_CDATA + dataOffset;
    context_D.transactionDataRemaining = apduLength - dataOffset;

    transaction_parse(PARSE_MODE_TRUSTED_INPUT);

    if (context_D.transactionContext.transactionState == TRANSACTION_PARSED)
    {
        //unsigned char targetHash[32];

        context_D.transactionContext.transactionState =
            TRANSACTION_NONE;
        set_check_internal_structure_integrity(1);
        if (!context_D.trustedInputProcessed)
        {
            // Output was not found
            return SW_INCORRECT_DATA;
        }

        //cx_hash(&context_D.transactionHashPrefix.header, CX_LAST,
        //       (unsigned char WIDE *)NULL, 0, targetHash);

        // Otherwise prepare
        cx_rng(G_io_apdu_buffer, 8);
        G_io_apdu_buffer[0] = MAGIC_TRUSTED_INPUT;
        G_io_apdu_buffer[1] = 0x00;

        blake256_Final(&context_D.transactionHashPrefix, G_io_apdu_buffer + 4);
        //cx_hash(&context_D.transactionHashPrefix, CX_LAST, (unsigned char WIDE *)NULL, 0, G_io_apdu_buffer + 4);zzzzzzzzz
        
        write_u32_le(G_io_apdu_buffer + 4 + 32,
                            context_D.transactionTargetInput);
        os_memmove(G_io_apdu_buffer + 4 + 32 + 4,
                   context_D.transactionContext.transactionAmount, 8);

        cx_hmac_sha256(N_btchip.bkp.trustedinput_key,
                       sizeof(N_btchip.bkp.trustedinput_key), G_io_apdu_buffer,
                       TRUSTEDINPUT_SIZE, trustedInputSignature);
        os_memmove(G_io_apdu_buffer + TRUSTEDINPUT_SIZE, trustedInputSignature,
                   8);

        context_D.outLength = 0x38;
    }
    return SW_OK;
}
