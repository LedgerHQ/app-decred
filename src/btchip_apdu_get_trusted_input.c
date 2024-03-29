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

#define GET_TRUSTED_INPUT_P1_FIRST 0x00
#define GET_TRUSTED_INPUT_P1_NEXT  0x80

#define TRUSTEDINPUT_SIZE 48

unsigned short btchip_apdu_get_trusted_input() {
    unsigned char apduLength;
    unsigned char dataOffset = 0;
    unsigned char trustedInputSignature[32];
    apduLength = G_io_apdu_buffer[ISO_OFFSET_LC];

    if (G_io_apdu_buffer[ISO_OFFSET_P1] == GET_TRUSTED_INPUT_P1_FIRST) {
        // Initialize
        btchip_context_D.transactionTargetInput =
            btchip_read_u32(G_io_apdu_buffer + ISO_OFFSET_CDATA, 1, 0);
        btchip_context_D.transactionContext.transactionState = BTCHIP_TRANSACTION_NONE;
        btchip_context_D.trustedInputProcessed = 0;
        btchip_context_D.transactionContext.consumeP2SH = 0;
        btchip_set_check_internal_structure_integrity(1);
        dataOffset = 4;
        btchip_context_D.transactionHashOption = TRANSACTION_HASH_FULL;
    } else if (G_io_apdu_buffer[ISO_OFFSET_P1] != GET_TRUSTED_INPUT_P1_NEXT) {
        return BTCHIP_SW_INCORRECT_P1_P2;
    }

    if (G_io_apdu_buffer[ISO_OFFSET_P2] != 0x00) {
        return BTCHIP_SW_INCORRECT_P1_P2;
    }
    btchip_context_D.transactionBufferPointer = G_io_apdu_buffer + ISO_OFFSET_CDATA + dataOffset;
    btchip_context_D.transactionDataRemaining = apduLength - dataOffset;

    transaction_parse(PARSE_MODE_TRUSTED_INPUT);

    if (btchip_context_D.transactionContext.transactionState == BTCHIP_TRANSACTION_PARSED) {
        // unsigned char targetHash[32];

        btchip_context_D.transactionContext.transactionState = BTCHIP_TRANSACTION_NONE;
        btchip_set_check_internal_structure_integrity(1);
        if (!btchip_context_D.trustedInputProcessed) {
            // Output was not found
            return BTCHIP_SW_INCORRECT_DATA;
        }

        // cx_hash(&btchip_context_D.transactionHashPrefix.header, CX_LAST,
        //       (unsigned char *)NULL, 0, targetHash);

        // Otherwise prepare
        cx_rng(G_io_apdu_buffer, 8);
        G_io_apdu_buffer[0] = MAGIC_TRUSTED_INPUT;
        G_io_apdu_buffer[1] = 0x00;

        blake256_Final(&btchip_context_D.transactionHashPrefix, G_io_apdu_buffer + 4);
        // cx_hash(&btchip_context_D.transactionHashPrefix, CX_LAST, (unsigned char *)NULL, 0,
        // G_io_apdu_buffer + 4);

        btchip_write_u32_le(G_io_apdu_buffer + 4 + 32, btchip_context_D.transactionTargetInput);
        memmove(G_io_apdu_buffer + 4 + 32 + 4,
                btchip_context_D.transactionContext.transactionAmount,
                8);

        cx_hmac_sha256((const uint8_t*) N_btchip.bkp.trustedinput_key,
                       sizeof(N_btchip.bkp.trustedinput_key),
                       G_io_apdu_buffer,
                       TRUSTEDINPUT_SIZE,
                       trustedInputSignature,
                       32);
        memmove(G_io_apdu_buffer + TRUSTEDINPUT_SIZE, trustedInputSignature, 8);

        btchip_context_D.outLength = 0x38;
    }
    return BTCHIP_SW_OK;
}
