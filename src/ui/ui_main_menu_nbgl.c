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

#define SWITCH_KEY_EXPORT_TOKEN FIRST_USER_TOKEN
#define BLIND_SIGNING_TOKEN (FIRST_USER_TOKEN + 1)

#define SETTING_INFO_NB 3
static const char *const infoTypes[SETTING_INFO_NB] = {"Version", "Developer", "Copyright"};
static const char *const infoContents[SETTING_INFO_NB] = {APPVERSION, "Ledger", "Ledger (c) 2025"};

static const nbgl_contentInfoList_t infoList = {
    .nbInfos = SETTING_INFO_NB,
    .infoTypes = infoTypes,
    .infoContents = infoContents,
};

static nbgl_contentSwitch_t blindSigningSwitch = {0};
static nbgl_content_t settingContentsArray[1] = {0};
static nbgl_genericContents_t settingContents = {0};

void onQuitCallback(void) {
    os_sched_exit(-1);
}

static void setting_toggle_callback(int token, uint8_t index, int page) {
    UNUSED(index);
    UNUSED(page);
    if (token == BLIND_SIGNING_TOKEN) {
        unsigned char options = N_btchip.bkp.config.options;
        unsigned char enabled = (options & BTCHIP_OPTION_BLIND_SIGNING) == 0;
        if (enabled) {
            options |= BTCHIP_OPTION_BLIND_SIGNING;
        } else {
            options &= ~BTCHIP_OPTION_BLIND_SIGNING;
        }
        nvm_write((void*) &N_btchip.bkp.config.options, &options, 1);
        blindSigningSwitch.initState = (nbgl_state_t) enabled;
    }
}

void ui_idle(void) {
    blindSigningSwitch.initState =
        (N_btchip.bkp.config.options & BTCHIP_OPTION_BLIND_SIGNING) ? ON_STATE : OFF_STATE;
    blindSigningSwitch.text = "Blind signing";
    blindSigningSwitch.subText = "Allow signing messages that can't be shown in full";
    blindSigningSwitch.token = BLIND_SIGNING_TOKEN;

    settingContentsArray[0].type = SWITCHES_LIST;
    settingContentsArray[0].content.switchesList.nbSwitches = 1;
    settingContentsArray[0].content.switchesList.switches = &blindSigningSwitch;
    settingContentsArray[0].contentActionCallback = setting_toggle_callback;

    settingContents.callbackCallNeeded = false;
    settingContents.contentsList = settingContentsArray;
    settingContents.nbContents = 1;

    nbgl_useCaseHomeAndSettings(APPNAME,
                                &ICON_APP,
                                NULL,
                                INIT_HOME_PAGE,
                                &settingContents,
                                &infoList,
                                NULL,
                                onQuitCallback);
}

#endif  // HAVE_NBGL
