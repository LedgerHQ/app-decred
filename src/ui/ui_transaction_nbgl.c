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
#ifdef HAVE_NBGL
#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "os.h"
#include "os_io_seproxyhal.h"
#include "ux.h"
#include "nbgl_use_case.h"

#include "ui_main_menu.h"
#include "ui_transaction.h"
#include "ui_shared.h"
#include "btchip_internal.h"
#include "glyphs.h"

typedef enum {
    TX_TYPE_SINGLE_OUTPUT = 0,
    TX_TYPE_FULL_REVIEW,
    TX_TYPE_FINALIZE,
    TX_TYPE_SIGN_MESSAGE,
} transaction_type_t;

typedef struct {
    const char* reviewStart;
    const char* finishOk;
} messages_t;

static nbgl_layoutTagValueList_t pairList;
static nbgl_layoutTagValue_t pairs[4];
static nbgl_pageInfoLongPress_t infoLongPress;
static transaction_type_t txType;
static messages_t msgs;
static char genericText[70];

static void reviewChoice(bool confirm);
static void rejectChoice(void);
static void displayTransaction(void);
static void reviewStart(void);

static void reviewChoice(bool confirm) {
    if (confirm) {
        if (txType != TX_TYPE_SINGLE_OUTPUT) {
            nbgl_useCaseStatus(msgs.finishOk, true, ui_idle);
        }
        txType == TX_TYPE_SIGN_MESSAGE ? io_seproxyhal_touch_message_signature_verify_ok(NULL)
                                       : io_seproxyhal_touch_verify_ok(NULL);
    } else {
        rejectChoice();
    }
}

static void rejectConfirmation(void) {
    nbgl_useCaseStatus("Transaction rejected", false, ui_idle);
    txType == TX_TYPE_SIGN_MESSAGE ? io_seproxyhal_touch_message_signature_verify_cancel(NULL)
                                   : io_seproxyhal_touch_verify_cancel(NULL);
}

static void rejectChoice(void) {
    nbgl_useCaseConfirm("Reject transaction ?",
                        NULL,
                        "Yes, Reject",
                        "Go back to transaction",
                        rejectConfirmation);
}

static void displayTransaction(void) {
    nbgl_useCaseStaticReview(&pairList, &infoLongPress, "Reject transaction", reviewChoice);
}

static void reviewStart(void) {
    explicit_bzero(pairs, sizeof(pairs));
    explicit_bzero(&infoLongPress, sizeof(infoLongPress));

    msgs.reviewStart = "Review\ntransaction";
    msgs.finishOk = "TRANSACTION\nSIGNED";

    infoLongPress.text = "Sign transaction";
    infoLongPress.longPressText = "Hold to sign";
    infoLongPress.icon = &C_decred_icon_64px;

    pairList.pairs = (nbgl_layoutTagValue_t*) pairs;

    switch (txType) {
        case TX_TYPE_SINGLE_OUTPUT:
            pairs[0].item = "Amount";
            pairs[0].value = vars.tmp.fullAmount;
            pairs[1].item = "To";
            pairs[1].value = vars.tmp.fullAddress;
            pairList.nbPairs = 2;
            explicit_bzero(genericText, sizeof(genericText));
            snprintf(genericText,
                     sizeof(genericText),
                     "Review output\n%d of %d",
                     btchip_context_D.totalOutputs - btchip_context_D.remainingOutputs + 1,
                     btchip_context_D.totalOutputs);
            msgs.reviewStart = genericText;
            infoLongPress.text = "Approve output";
            infoLongPress.longPressText = "Hold to approve";
            break;
        case TX_TYPE_FINALIZE:
            pairs[0].item = "Fees";
            pairs[0].value = vars.tmp.feesAmount;
            pairList.nbPairs = 1;
            msgs.reviewStart = "Finalize\n transaction";
            break;
        case TX_TYPE_SIGN_MESSAGE:
            pairs[0].item = "Message Hash";
            pairs[0].value = vars.tmp.fullAddress;
            pairList.nbPairs = 1;
            msgs.reviewStart = "Sign\nMessage";
            msgs.finishOk = "MESSAGE SIGNED";
            break;
        case TX_TYPE_FULL_REVIEW:
            __attribute__((fallthrough));
        default:
            pairs[0].item = "Amount";
            pairs[0].value = vars.tmp.fullAmount;
            pairs[1].item = "To";
            pairs[1].value = vars.tmp.fullAddress;
            pairs[2].item = "Fees";
            pairs[2].value = vars.tmp.feesAmount;
            pairList.nbPairs = 3;
            break;
    }

    nbgl_useCaseReviewStart(&C_decred_icon_64px,
                            msgs.reviewStart,
                            NULL,
                            "Reject transaction",
                            displayTransaction,
                            rejectChoice);
}

unsigned int ui_tx_confirm_full_output() {
    txType = TX_TYPE_FULL_REVIEW;
    if (!prepare_full_output(0)) {
        return 0;
    }
    reviewStart();
    return 1;
}

unsigned int ui_tx_finalize() {
    txType = TX_TYPE_FINALIZE;
    if (!prepare_fees()) {
        return 0;
    }
    reviewStart();
    return 1;
}

void ui_tx_confirm_message_signature() {
    txType = TX_TYPE_SIGN_MESSAGE;
    if (!prepare_message_signature()) {
        return;
    }
    reviewStart();
}

unsigned int ui_tx_confirm_single_output() {
    txType = TX_TYPE_SINGLE_OUTPUT;
    if (!prepare_single_output()) {
        return 0;
    }
    reviewStart();
    return 1;
}

static void changePathWarningChoice(bool reject) {
    if (reject) {
        io_seproxyhal_touch_display_cancel(NULL);
        nbgl_useCaseStatus("Transaction rejected", false, ui_idle);
    } else {
        io_seproxyhal_touch_display_ok(NULL);
    }
}

void ui_tx_request_change_path_approval(unsigned char* change_path) {
    bip32_print_path(change_path, vars.tmp_warning.derivation_path, MAX_DERIV_PATH_ASCII_LENGTH);
    explicit_bzero(genericText, sizeof(genericText));
    snprintf(genericText,
             sizeof(genericText),
             "WARNING !\nThe change path is\nunusual :\n%s",
             vars.tmp_warning.derivation_path);
    nbgl_useCaseChoice(NULL,
                       genericText,
                       "Reject if you're not sure",
                       "Reject",
                       "Continue",
                       changePathWarningChoice);
}
#endif
