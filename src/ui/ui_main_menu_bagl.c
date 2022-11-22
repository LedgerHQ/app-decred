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
#include "ui_shared.h"
#include "os.h"
#include "os_io_seproxyhal.h"
#include "ux.h"
#include "string.h"

#include "btchip_internal.h"

// #include "btchip_bagl_extensions.h"

#if defined(TARGET_NANOS)

const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_settings[];

// change the setting
void menu_settings_pubKeyRequestRestriction_change(unsigned int enabled) {
    nvm_write((void *)&N_btchip.pubKeyRequestRestriction, &enabled, 1);
    // go back to the menu entry
    UX_MENU_DISPLAY(0, menu_main, NULL);
}
const ux_menu_entry_t menu_settings_pubKeyRequestRestriction[] = {
  {NULL, menu_settings_pubKeyRequestRestriction_change, 1, NULL, "Manual approval", NULL, 0, 0},
  {NULL, menu_settings_pubKeyRequestRestriction_change, 0, NULL, "Auto approval", NULL, 0, 0},
  UX_MENU_END
};
const ux_menu_entry_t menu_settings[] = {
    {menu_settings_pubKeyRequestRestriction, NULL, 0, NULL, "Public keys", "export approval", 0, 0},
    {menu_main, NULL, 1, &C_nanos_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END};

const ux_menu_entry_t menu_about[] = {
    {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
    {menu_main, NULL, 1, &C_nanos_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END};

const ux_menu_entry_t menu_main[] = {
    //{NULL, NULL, 0, &NAME3(C_nanos_badge_, COINID, ), "Use wallet to", "view
    //accounts", 33, 12},
    {NULL, NULL, 0, NULL, "Use wallet to", "view accounts", 0, 0},
    {menu_settings, NULL, 0, NULL, "Settings", NULL, 0, 0},
    {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_nanos_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END};

#endif // #if defined(TARGET_NANOS)

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)

const char* settings_submenu_getter(unsigned int idx);
void settings_submenu_selector(unsigned int idx);


void settings_pubkey_export_change(unsigned int enabled) {
    nvm_write((void *)&N_btchip.pubKeyRequestRestriction, &enabled, 1);
    ui_idle();
}
//////////////////////////////////////////////////////////////////////////////////////
// Public keys export submenu:

const char* const settings_pubkey_export_getter_values[] = {
  "Auto Approval",
  "Manual Approval",
  "Back"
};

const char* settings_pubkey_export_getter(unsigned int idx) {
  if (idx < ARRAYLEN(settings_pubkey_export_getter_values)) {
    return settings_pubkey_export_getter_values[idx];
  }
  return NULL;
}

void settings_pubkey_export_selector(unsigned int idx) {
  switch(idx) {
    case 0:
      settings_pubkey_export_change(0);
      break;
    case 1:
      settings_pubkey_export_change(1);
      break;
    default:
      ux_menulist_init(0, settings_submenu_getter, settings_submenu_selector);
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// Settings menu:

const char* const settings_submenu_getter_values[] = {
  "Public keys export",
  "Back",
};

const char* settings_submenu_getter(unsigned int idx) {
  if (idx < ARRAYLEN(settings_submenu_getter_values)) {
    return settings_submenu_getter_values[idx];
  }
  return NULL;
}

void settings_submenu_selector(unsigned int idx) {
  switch(idx) {
    case 0:
      ux_menulist_init_select(0, settings_pubkey_export_getter, settings_pubkey_export_selector, N_btchip.pubKeyRequestRestriction);
      break;
    default:
      ui_idle();
  }
}

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(
    ux_idle_flow_1_step,
    nn, 
    {
      "Application",
      "is ready",
    });
UX_STEP_VALID(
    ux_idle_flow_2_step,
    pb,
    ux_menulist_init(0, settings_submenu_getter, settings_submenu_selector),
    {
      &C_icon_coggle,
      "Settings",
    });
UX_STEP_NOCB(
    ux_idle_flow_3_step, 
    bn, 
    {
      "Version",
      APPVERSION,
    });
UX_STEP_VALID(
    ux_idle_flow_4_step,
    pb,
    os_sched_exit(-1),
    {
      &C_icon_dashboard_x,
      "Quit",
    });
UX_FLOW(ux_idle_flow,
  &ux_idle_flow_1_step,
  &ux_idle_flow_2_step,
  &ux_idle_flow_3_step,
  &ux_idle_flow_4_step
);
#endif

void ui_idle(void) {
    ux_step_count = 0;
    ux_loop_over_curr_element = 0;

#if defined(TARGET_NANOS)
    UX_MENU_DISPLAY(0, menu_main, NULL);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    // reserve a display stack slot if none yet
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
#endif // #if TARGET_ID
}
#endif // HAVE_BAGL
