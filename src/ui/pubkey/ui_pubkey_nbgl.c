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
#include "ui_pubkey.h"
#include "ui_shared.h"
#include "btchip_internal.h"

typedef enum {
    DISPLAY_ADDRESS = 0,
    EXPORT_REQUEST,
    DISPLAY_TOKEN,
} pubkey_display_usecase_t;

static char confirm_text[20];
static char choice_text[70];
static pubkey_display_usecase_t display_type;

static void confirmationChoiceClbk(bool confirm) {
    explicit_bzero(confirm_text, sizeof(confirm_text));
    switch(display_type){
        case DISPLAY_ADDRESS:
            if(confirm)
            {
                strncpy(confirm_text,"ADDRESS\nAPPROVED", sizeof(confirm_text));
            }
            else
            {
                strncpy(confirm_text,"Address rejected", sizeof(confirm_text));
            }
            break;
        case EXPORT_REQUEST:
            if(confirm)
            {
                strncpy(confirm_text,"PUBLIC KEY\nEXPORTED", sizeof(confirm_text));
            }
            else
            {
                strncpy(confirm_text,"Export cancelled", sizeof(confirm_text));
            }
            break;
        case DISPLAY_TOKEN:
            if(confirm)
            {
                strncpy(confirm_text,"TOKEN\nAPPROVED", sizeof(confirm_text));
            }
            else
            {
                strncpy(confirm_text,"Token rejected", sizeof(confirm_text));
            }
            break;
        default:
            PRINTF("Should not happen !\n");
            break;
    }
    confirm ? io_seproxyhal_touch_display_ok(NULL) : io_seproxyhal_touch_display_cancel(NULL);
    nbgl_useCaseStatus(confirm_text,confirm,ui_idle);
}

static void warningChoiceClbk(bool reject) {
  if (reject) {
    io_seproxyhal_touch_display_cancel(NULL);
    nbgl_useCaseStatus("Address rejected",false,ui_idle);
  }
  else {
    nbgl_useCaseAddressConfirmation((char*)G_io_apdu_buffer+200,confirmationChoiceClbk);
  }
}

void ui_display_public_key(unsigned char* derivation_path) {
    display_type = DISPLAY_ADDRESS;
    bip32_print_path(derivation_path, vars.tmp_warning.derivation_path, MAX_DERIV_PATH_ASCII_LENGTH);
    uint8_t is_derivation_path_unusual = bip44_derivation_guard(derivation_path, false);
    if(is_derivation_path_unusual)
    {
        explicit_bzero(choice_text, sizeof(choice_text));
        snprintf(choice_text,
                 sizeof(choice_text),
                 "WARNING !\nThe derivation path is\nunusual :\n%s",
                 vars.tmp_warning.derivation_path);
        nbgl_useCaseChoice(NULL, choice_text,"Reject if you're not sure","Reject","Continue",warningChoiceClbk);
    }
    else
    {
        nbgl_useCaseAddressConfirmation((char*)G_io_apdu_buffer+200,confirmationChoiceClbk);
    }
}

void ui_display_request_pubkey_approval(void)
{
    display_type = EXPORT_REQUEST;
    nbgl_useCaseChoice(NULL,"Export public key ?",NULL,"Export","Cancel",confirmationChoiceClbk);
}

void ui_display_token(void)
{
    display_type = DISPLAY_TOKEN;
    explicit_bzero(choice_text, sizeof(choice_text));
    snprintf(choice_text,
             sizeof(choice_text),
             "Approve token :\n%s",
             (char*)G_io_apdu_buffer+200);
    nbgl_useCaseChoice(NULL,choice_text,NULL,"Approve","Reject",confirmationChoiceClbk);
}

#endif // HAVE_NBGL
