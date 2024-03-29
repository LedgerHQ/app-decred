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

#include "btchip_bagl_extensions.h"
#include "ui_pubkey.h"

#define P1_NO_DISPLAY    0x00
#define P1_DISPLAY       0x01
#define P1_REQUEST_TOKEN 0x02

#define P2_LEGACY 0x00

unsigned short btchip_apdu_get_wallet_public_key() {
    unsigned char keyLength;
    unsigned char uncompressedPublicKeys =
        ((N_btchip.bkp.config.options & BTCHIP_OPTION_UNCOMPRESSED_KEYS) != 0);
    unsigned char keyPath[MAX_BIP32_PATH_LENGTH];
    uint32_t request_token;
    unsigned char chainCode[32];
    bool display = (G_io_apdu_buffer[ISO_OFFSET_P1] == P1_DISPLAY);
    bool display_request_token = N_btchip.pubKeyRequestRestriction &&
                                 (G_io_apdu_buffer[ISO_OFFSET_P1] == P1_REQUEST_TOKEN) &&
                                 G_io_apdu_media == IO_APDU_MEDIA_U2F;
    bool require_user_approval = N_btchip.pubKeyRequestRestriction &&
                                 !(display_request_token || display) &&
                                 G_io_apdu_media == IO_APDU_MEDIA_U2F;

    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
        case P1_NO_DISPLAY:
        case P1_DISPLAY:
        case P1_REQUEST_TOKEN:
            break;
        default:
            return BTCHIP_SW_INCORRECT_P1_P2;
    }

    switch (G_io_apdu_buffer[ISO_OFFSET_P2]) {
        case P2_LEGACY:
            break;
        default:
            return BTCHIP_SW_INCORRECT_P1_P2;
    }

    if (G_io_apdu_buffer[ISO_OFFSET_LC] < 0x01) {
        return BTCHIP_SW_INCORRECT_LENGTH;
    }
    memmove(keyPath, G_io_apdu_buffer + ISO_OFFSET_CDATA, MAX_BIP32_PATH_LENGTH);

    if (display_request_token) {
        uint8_t request_token_offset =
            ISO_OFFSET_CDATA + G_io_apdu_buffer[ISO_OFFSET_CDATA] * 4 + 1;
        request_token = btchip_read_u32(G_io_apdu_buffer + request_token_offset, true, false);
    }

    if (os_global_pin_is_validated() != BOLOS_UX_OK) {
        return BTCHIP_SW_SECURITY_STATUS_NOT_SATISFIED;
    }

    btchip_private_derive_keypair(keyPath, 1, chainCode);
    G_io_apdu_buffer[0] = 65;

    // Then encode it
    if (uncompressedPublicKeys) {
        keyLength = 65;
    } else {
        btchip_compress_public_key_value(btchip_public_key_D.W);
        keyLength = 33;
    }

    memmove(G_io_apdu_buffer + 1, btchip_public_key_D.W, sizeof(btchip_public_key_D.W));

    keyLength = btchip_public_key_to_encoded_base58(G_io_apdu_buffer + 1,   // IN
                                                    keyLength,              // INLEN
                                                    G_io_apdu_buffer + 67,  // OUT
                                                    150,                    // MAXOUTLEN
                                                    btchip_context_D.payToAddressVersion,
                                                    0);

    G_io_apdu_buffer[66] = keyLength;
    PRINTF("Length %d\n", keyLength);
    if (!uncompressedPublicKeys) {
        // Restore for the full key component
        G_io_apdu_buffer[1] = 0x04;
    }

    // output chain code
    memmove(G_io_apdu_buffer + 1 + 65 + 1 + keyLength, chainCode, sizeof(chainCode));
    btchip_context_D.outLength = 1 + 65 + 1 + keyLength + sizeof(chainCode);

    if (display) {
        if (keyLength > 50) {
            return BTCHIP_SW_INCORRECT_DATA;
        }
        // Hax, avoid wasting space
        memmove(G_io_apdu_buffer + 200, G_io_apdu_buffer + 67, keyLength);
        G_io_apdu_buffer[200 + keyLength] = '\0';
        btchip_context_D.io_flags |= IO_ASYNCH_REPLY;
        ui_display_public_key(keyPath);
    }
    // If the token requested has already been approved in a previous call, the source is trusted so
    // don't ask for approval again
    else if (display_request_token && (!btchip_context_D.has_valid_token ||
                                       memcmp(&request_token, btchip_context_D.last_token, 4))) {
        // disable the has_valid_token flag and store the new token
        btchip_context_D.has_valid_token = false;
        memcpy(btchip_context_D.last_token, &request_token, 4);
        // Hax, avoid wasting space
        snprintf((char*) G_io_apdu_buffer + 200, 9, "%08X", request_token);
        G_io_apdu_buffer[200 + 8] = '\0';
        btchip_context_D.io_flags |= IO_ASYNCH_REPLY;
        ui_display_token();
    } else if (require_user_approval) {
        btchip_context_D.io_flags |= IO_ASYNCH_REPLY;
        ui_display_request_pubkey_approval();
    }

    return BTCHIP_SW_OK;
}

void btchip_bagl_user_action_display(unsigned char confirming) {
    unsigned short sw = BTCHIP_SW_OK;
    // confirm and finish the apdu exchange //spaghetti
    if (confirming) {
        btchip_context_D.outLength -= 2;  // status was already set by the last call

    } else {
        sw = BTCHIP_SW_CONDITIONS_OF_USE_NOT_SATISFIED;
        btchip_context_D.outLength = 0;
    }
    G_io_apdu_buffer[btchip_context_D.outLength++] = sw >> 8;
    G_io_apdu_buffer[btchip_context_D.outLength++] = sw;

    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, btchip_context_D.outLength);
}
