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

#include "bagl_extensions.h"

#define P1_NO_DISPLAY 0x00
#define P1_DISPLAY 0x01

#define P2_LEGACY 0x00

unsigned short apdu_get_wallet_public_key() {
    unsigned char keyLength;
    unsigned char uncompressedPublicKeys =
        ((N_btchip.bkp.config.options & OPTION_UNCOMPRESSED_KEYS) != 0);
    unsigned char keyPath[MAX_BIP32_PATH_LENGTH];
    unsigned char chainCode[32];
    bool display = (G_io_apdu_buffer[ISO_OFFSET_P1] == P1_DISPLAY);

    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_NO_DISPLAY:
    case P1_DISPLAY:
        break;
    default:
        return SW_INCORRECT_P1_P2;
    }

    switch (G_io_apdu_buffer[ISO_OFFSET_P2]) {
    case P2_LEGACY:
        break;
    default:
        return SW_INCORRECT_P1_P2;
    }

    if (G_io_apdu_buffer[ISO_OFFSET_LC] < 0x01) {
        return SW_INCORRECT_LENGTH;
    }
    os_memmove(keyPath, G_io_apdu_buffer + ISO_OFFSET_CDATA,
               MAX_BIP32_PATH_LENGTH);

    SB_CHECK(N_btchip.bkp.config.operationMode);
    switch (SB_GET(N_btchip.bkp.config.operationMode)) {
    case MODE_WALLET:
    case MODE_RELAXED_WALLET:
    case MODE_SERVER:
        break;
    default:
        return SW_CONDITIONS_OF_USE_NOT_SATISFIED;
    }

    if (!os_global_pin_is_validated()) {
        return SW_SECURITY_STATUS_NOT_SATISFIED;
    }

    private_derive_keypair(keyPath, 1, chainCode);
    G_io_apdu_buffer[0] = 65;

    // Then encode it
    if (uncompressedPublicKeys) {
        keyLength = 65;
    } else {
        compress_public_key_value(public_key_D.W);
        keyLength = 33;
    }

    os_memmove(G_io_apdu_buffer + 1, public_key_D.W,
               sizeof(public_key_D.W));

    keyLength = public_key_to_encoded_base58(G_io_apdu_buffer + 1,  // IN
                                                    keyLength,             // INLEN
                                                    G_io_apdu_buffer + 67, // OUT
                                                    150,                   // MAXOUTLEN
                                                    context_D.payToAddressVersion, 0);

    G_io_apdu_buffer[66] = keyLength;

    if (!uncompressedPublicKeys) {
        // Restore for the full key component
        G_io_apdu_buffer[1] = 0x04;
    }

    // output chain code
    os_memmove(G_io_apdu_buffer + 1 + 65 + 1 + keyLength, chainCode,
               sizeof(chainCode));
    context_D.outLength = 1 + 65 + 1 + keyLength + sizeof(chainCode);

    if (display) {
        if (keyLength > 50) {
            return SW_INCORRECT_DATA;
        }
        // Hax, avoid wasting space
        os_memmove(G_io_apdu_buffer + 200, G_io_apdu_buffer + 67, keyLength);
        G_io_apdu_buffer[200 + keyLength] = '\0';
        context_D.io_flags |= IO_ASYNCH_REPLY;
        if (!bagl_display_public_key()) {
            context_D.io_flags &= ~IO_ASYNCH_REPLY;
            context_D.outLength = 0;
            return SW_INCORRECT_DATA;
        }
    }

    return SW_OK;
}

void bagl_user_action_display(unsigned char confirming) {
    unsigned short sw = SW_OK;
    // confirm and finish the apdu exchange //spaghetti
    if (confirming) {
        context_D.outLength -=
            2; // status was already set by the last call

    } else {
        sw = SW_CONDITIONS_OF_USE_NOT_SATISFIED;
        context_D.outLength = 0;
    }
    G_io_apdu_buffer[context_D.outLength++] = sw >> 8;
    G_io_apdu_buffer[context_D.outLength++] = sw;

    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, context_D.outLength);
}