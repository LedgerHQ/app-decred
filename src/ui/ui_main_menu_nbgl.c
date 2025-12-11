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

#define SETTING_INFO_NB 3
static const char *const infoTypes[SETTING_INFO_NB] = {"Version", "Developer", "Copyright"};
static const char *const infoContents[SETTING_INFO_NB] = {APPVERSION, "Ledger", "Ledger (c) 2025"};

static const nbgl_contentInfoList_t infoList = {
    .nbInfos = SETTING_INFO_NB,
    .infoTypes = infoTypes,
    .infoContents = infoContents,
};

void onQuitCallback(void) {
    os_sched_exit(-1);
}

void ui_idle(void) {
    nbgl_useCaseHomeAndSettings(APPNAME,
                                &ICON_APP,
                                NULL,
                                INIT_HOME_PAGE,
                                NULL,
                                &infoList,
                                NULL,
                                onQuitCallback);
}

#endif  // HAVE_NBGL
