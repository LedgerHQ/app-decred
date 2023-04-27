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
#include "ui_transaction.h"
#include "ui_shared.h"
#include "os.h"
#include "os_io_seproxyhal.h"
#include "ux.h"
#include "string.h"
#include "btchip_internal.h"

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(ux_sign_flow_1_step,
             pnn,
             {
                 &C_icon_certificate,
                 "Sign",
                 "message",
             });
UX_STEP_NOCB(ux_sign_flow_2_step,
             bnnn_paging,
             {
                 .title = "Message hash",
                 .text = vars.tmp.fullAddress,
             });
UX_STEP_VALID(ux_sign_flow_3_step,
              pbb,
              io_seproxyhal_touch_message_signature_verify_ok(NULL),
              {
                  &C_icon_validate_14,
                  "Accept",
                  "and sign",
              });
UX_STEP_VALID(ux_sign_flow_4_step,
              pbb,
              io_seproxyhal_touch_message_signature_verify_cancel(NULL),
              {
                  &C_icon_crossmark,
                  "Cancel",
                  "signature",
              });

UX_FLOW(ux_sign_flow,
        &ux_sign_flow_1_step,
        &ux_sign_flow_2_step,
        &ux_sign_flow_3_step,
        &ux_sign_flow_4_step);

//////////////////////////////////////////////////////////////////////

UX_STEP_NOCB(ux_confirm_full_flow_1_step,
             pnn,
             {
                 &C_icon_eye,
                 "Review",
                 "transaction",
             });
UX_STEP_NOCB(ux_confirm_full_flow_2_step,
             bnnn_paging,
             {.title = "Amount", .text = vars.tmp.fullAmount});
UX_STEP_NOCB(ux_confirm_full_flow_3_step,
             bnnn_paging,
             {
                 .title = "Address",
                 .text = vars.tmp.fullAddress,
             });
UX_STEP_NOCB(ux_confirm_full_flow_4_step,
             bnnn_paging,
             {
                 .title = "Fees",
                 .text = vars.tmp.feesAmount,
             });
UX_STEP_VALID(ux_confirm_full_flow_5_step,
              pbb,
              io_seproxyhal_touch_verify_ok(NULL),
              {
                  &C_icon_validate_14,
                  "Accept",
                  "and send",
              });
UX_STEP_VALID(ux_confirm_full_flow_6_step,
              pb,
              io_seproxyhal_touch_verify_cancel(NULL),
              {
                  &C_icon_crossmark,
                  "Reject",
              });
// confirm_full: confirm transaction / Amount: fullAmount / Address: fullAddress / Fees: feesAmount
UX_FLOW(ux_confirm_full_flow,
        &ux_confirm_full_flow_1_step,
        &ux_confirm_full_flow_2_step,
        &ux_confirm_full_flow_3_step,
        &ux_confirm_full_flow_4_step,
        &ux_confirm_full_flow_5_step,
        &ux_confirm_full_flow_6_step);

//////////////////////////////////////////////////////////////////////

UX_STEP_NOCB(ux_confirm_single_flow_1_step,
             pnn,
             {&C_icon_eye, "Confirm output", vars.tmp.feesAmount});
UX_STEP_NOCB(ux_confirm_single_flow_2_step,
             bnnn_paging,
             {
                 .title = "Amount",
                 .text = vars.tmp.fullAmount,
             });
UX_STEP_NOCB(ux_confirm_single_flow_3_step,
             bnnn_paging,
             {
                 .title = "Address",
                 .text = vars.tmp.fullAddress,
             });
UX_STEP_VALID(ux_confirm_single_flow_4_step,
              pb,
              io_seproxyhal_touch_verify_ok(NULL),
              {
                  &C_icon_validate_14,
                  "Accept",
              });
UX_STEP_VALID(ux_confirm_single_flow_5_step,
              pb,
              io_seproxyhal_touch_verify_cancel(NULL),
              {
                  &C_icon_crossmark,
                  "Reject",
              });
// confirm_single: confirm output #x(feesAmount) / Amount: fullAmount / Address: fullAddress
UX_FLOW(ux_confirm_single_flow,
        &ux_confirm_single_flow_1_step,
        &ux_confirm_single_flow_2_step,
        &ux_confirm_single_flow_3_step,
        &ux_confirm_single_flow_4_step,
        &ux_confirm_single_flow_5_step);

//////////////////////////////////////////////////////////////////////

UX_STEP_NOCB(ux_finalize_flow_1_step, pnn, {&C_icon_eye, "Review", "transaction"});
UX_STEP_NOCB(ux_finalize_flow_4_step,
             bnnn_paging,
             {
                 .title = "Fees",
                 .text = vars.tmp.feesAmount,
             });
UX_STEP_VALID(ux_finalize_flow_5_step,
              pbb,
              io_seproxyhal_touch_verify_ok(NULL),
              {&C_icon_validate_14, "Accept", "and send"});
UX_STEP_VALID(ux_finalize_flow_6_step,
              pb,
              io_seproxyhal_touch_verify_cancel(NULL),
              {
                  &C_icon_crossmark,
                  "Reject",
              });
// finalize: confirm transaction / Fees: feesAmount
UX_FLOW(ux_finalize_flow,
        &ux_finalize_flow_1_step,
        &ux_finalize_flow_4_step,
        &ux_finalize_flow_5_step,
        &ux_finalize_flow_6_step);

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(ux_request_change_path_approval_flow_1_step,
             pbb,
             {
                 &C_icon_eye,
                 "The change path",
                 "is unusual",
             });
UX_STEP_NOCB(ux_request_change_path_approval_flow_2_step,
             bnnn_paging,
             {
                 .title = "Change path",
                 .text = vars.tmp_warning.derivation_path,
             });
UX_STEP_VALID(ux_request_change_path_approval_flow_3_step,
              pbb,
              io_seproxyhal_touch_display_cancel(NULL),
              {
                  &C_icon_crossmark,
                  "Reject if you're",
                  "not sure",
              });
UX_STEP_VALID(ux_request_change_path_approval_flow_4_step,
              pb,
              io_seproxyhal_touch_display_ok(NULL),
              {
                  &C_icon_validate_14,
                  "Approve",
              });

UX_FLOW(ux_request_change_path_approval_flow,
        &ux_request_change_path_approval_flow_1_step,
        &ux_request_change_path_approval_flow_2_step,
        &ux_request_change_path_approval_flow_3_step,
        &ux_request_change_path_approval_flow_4_step);

unsigned int ui_tx_confirm_full_output() {
    if (!prepare_full_output(0)) {
        return 0;
    }
    ux_flow_init(0, ux_confirm_full_flow, NULL);
    return 1;
}

unsigned int ui_tx_confirm_single_output() {
    // TODO : remove when supporting multi output
    if (!prepare_single_output()) {
        return 0;
    }

    snprintf(vars.tmp.feesAmount,
             sizeof(vars.tmp.feesAmount),
             "#%d",
             btchip_context_D.totalOutputs - btchip_context_D.remainingOutputs + 1);

    ux_flow_init(0, ux_confirm_single_flow, NULL);
    return 1;
}

unsigned int ui_tx_finalize() {
    if (!prepare_fees()) {
        return 0;
    }
    ux_flow_init(0, ux_finalize_flow, NULL);
    return 1;
}

void ui_tx_confirm_message_signature() {
    if (!prepare_message_signature()) {
        return;
    }
    ux_flow_init(0, ux_sign_flow, NULL);
}

void ui_tx_request_change_path_approval(unsigned char *change_path) {
    bip32_print_path(change_path, vars.tmp_warning.derivation_path, MAX_DERIV_PATH_ASCII_LENGTH);
    ux_flow_init(0, ux_request_change_path_approval_flow, NULL);
}

#endif