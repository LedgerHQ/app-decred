/*******************************************************************************
 *   Ledger App - Decred Wallet
 *   (c) 2022 Ledger
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
#ifdef HAVE_BAGL
#include "ui_main_menu.h"
#include "ui_pubkey.h"
#include "ui_shared.h"
#include "os.h"
#include "os_io_seproxyhal.h"
#include "ux.h"
#include "string.h"
#include "btchip_internal.h"

UX_STEP_NOCB(ux_display_public_flow_1_step,
             pnn,
             {
                 &C_icon_warning,
                 "The derivation",
                 "path is unusual!",
             });
UX_STEP_NOCB(ux_display_public_flow_2_step,
             bnnn_paging,
             {
                 .title = "Derivation path",
                 .text = vars.tmp_warning.derivation_path,
             });
UX_STEP_VALID(ux_display_public_flow_3_step,
              pnn,
              io_seproxyhal_touch_display_cancel(NULL),
              {
                  &C_icon_crossmark,
                  "Reject if you're",
                  "not sure",
              });
UX_STEP_NOCB(ux_display_public_flow_4_step,
             pnn,
             {
                 &C_icon_validate_14,
                 "Approve derivation",
                 "path",
             });
UX_STEP_NOCB(ux_display_public_flow_5_step,
             bnnn_paging,
             {
                 .title = "Address",
                 .text = (const char *) G_io_apdu_buffer + 200,
             });
UX_STEP_VALID(ux_display_public_flow_6_step,
              pb,
              io_seproxyhal_touch_display_ok(NULL),
              {
                  &C_icon_validate_14,
                  "Approve",
              });
UX_STEP_VALID(ux_display_public_flow_7_step,
              pb,
              io_seproxyhal_touch_display_cancel(NULL),
              {
                  &C_icon_crossmark,
                  "Reject",
              });

UX_FLOW(ux_display_public_with_warning_flow,
        &ux_display_public_flow_1_step,
        &ux_display_public_flow_2_step,
        &ux_display_public_flow_3_step,
        &ux_display_public_flow_4_step,
        FLOW_BARRIER,
        &ux_display_public_flow_5_step,
        &ux_display_public_flow_6_step,
        &ux_display_public_flow_7_step);

UX_FLOW(ux_display_public_flow,
        &ux_display_public_flow_5_step,
        &ux_display_public_flow_6_step,
        &ux_display_public_flow_7_step);

//////////////////////////////////////////////////////////////////////

UX_STEP_VALID(ux_request_pubkey_approval_flow_1_step,
              pbb,
              io_seproxyhal_touch_display_ok(NULL),
              {
                  &C_icon_validate_14,
                  "Export",
                  "public key?",
              });
UX_STEP_VALID(ux_request_pubkey_approval_flow_2_step,
              pb,
              io_seproxyhal_touch_display_cancel(NULL),
              {
                  &C_icon_crossmark,
                  "Reject",
              });

UX_FLOW(ux_request_pubkey_approval_flow,
        &ux_request_pubkey_approval_flow_1_step,
        &ux_request_pubkey_approval_flow_2_step);

//////////////////////////////////////////////////////////////////////

UX_STEP_VALID(ux_display_token_flow_1_step,
              pbb,
              io_seproxyhal_touch_display_ok(NULL),
              {
                  &C_icon_validate_14,
                  "Confirm token",
                  (const char *) G_io_apdu_buffer + 200,
              });
UX_STEP_VALID(ux_display_token_flow_2_step,
              pb,
              io_seproxyhal_touch_display_cancel(NULL),
              {
                  &C_icon_crossmark,
                  "Reject",
              });

UX_FLOW(ux_display_token_flow, &ux_display_token_flow_1_step, &ux_display_token_flow_2_step);

void ui_display_public_key(unsigned char *derivation_path) {
    // append a white space at the end of the address to avoid glitch on nano S
    strlcat((char *) G_io_apdu_buffer + 200, " ", sizeof(G_io_apdu_buffer) - 200);

    bip32_print_path(derivation_path,
                     vars.tmp_warning.derivation_path,
                     MAX_DERIV_PATH_ASCII_LENGTH);
    uint8_t is_derivation_path_unusual = bip44_derivation_guard(derivation_path, false);

    ux_flow_init(
        0,
        is_derivation_path_unusual ? ux_display_public_with_warning_flow : ux_display_public_flow,
        NULL);
}

void ui_display_request_pubkey_approval(void) {
    ux_flow_init(0, ux_request_pubkey_approval_flow, NULL);
}

void ui_display_token(void) {
    ux_flow_init(0, ux_display_token_flow, NULL);
}
#endif  // HAVE_BAGL
