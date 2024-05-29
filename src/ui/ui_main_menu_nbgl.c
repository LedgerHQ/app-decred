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
#include "ui_main_menu.h"
#include "ui_shared.h"
#include "os.h"
#include "os_io_seproxyhal.h"
#include "ux.h"
#include "string.h"

#include "glyphs.h"
#include "nbgl_page.h"
#include "nbgl_use_case.h"

#include "btchip_internal.h"

static void displaySettingsMenu(void);
static void settingsControlsCallback(int token, uint8_t index);
static bool settingsNavCallback(uint8_t page, nbgl_pageContent_t *content);

#define SWITCH_KEY_EXPORT_TOKEN FIRST_USER_TOKEN

#define NB_INFO_FIELDS 2
static const char *const infoTypes[] = {"Version", "Developer"};
static const char *const infoContents[] = {APPVERSION, "Ledger"};

static nbgl_layoutSwitch_t setting_switch;

void onQuitCallback(void) {
    os_sched_exit(-1);
}

static bool settingsNavCallback(uint8_t page, nbgl_pageContent_t *content) {
    if (page == 0) {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = NB_INFO_FIELDS;
        content->infosList.infoTypes = (const char **) infoTypes;
        content->infosList.infoContents = (const char **) infoContents;
    } else if (page == 1) {
        setting_switch.text = "U2F public keys export";
        setting_switch.subText = "Enable manual approval";
        setting_switch.token = SWITCH_KEY_EXPORT_TOKEN;
        setting_switch.tuneId = TUNE_TAP_CASUAL;
        setting_switch.initState = (bool) N_btchip.pubKeyRequestRestriction;
        content->type = SWITCHES_LIST;
        content->switchesList.nbSwitches = 1;
        content->switchesList.switches = (nbgl_layoutSwitch_t *) &setting_switch;
    } else {
        return false;
    }
    return true;
}

static void settingsControlsCallback(int token, uint8_t index) {
    UNUSED(index);
    switch (token) {
        case SWITCH_KEY_EXPORT_TOKEN: {
            unsigned int setting_value = (unsigned int) !N_btchip.pubKeyRequestRestriction;
            nvm_write((void *) &N_btchip.pubKeyRequestRestriction, &setting_value, 1);
            break;
        }
        default: {
            PRINTF("Should not happen !");
            break;
        }
    }
}

static void displaySettingsMenu(void) {
    nbgl_useCaseSettings("Decred settings",
                         0,
                         2,
                         false,
                         ui_idle,
                         settingsNavCallback,
                         settingsControlsCallback);
}

void ui_idle(void) {
    nbgl_useCaseHome("Decred",
                     &C_decred_icon_64px,
                     NULL,
                     true,
                     displaySettingsMenu,
                     onQuitCallback);
}
#endif  // HAVE_NBGL
