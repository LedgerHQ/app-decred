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
#include "ui_shared.h"
#include "btchip_bagl_extensions.h"

unsigned int ux_step;
unsigned int ux_step_count;
uint8_t ux_loop_over_curr_element;
vars_u_t vars;

unsigned int io_seproxyhal_touch_display_cancel(const void* e){
    UNUSED(e);
    // user denied the transaction, tell the USB side
    btchip_bagl_user_action_display(0);
    // redraw ui
#ifndef HAVE_NBGL
    ui_idle();
#endif
    return 0; // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_display_ok(const void* e){
    UNUSED(e);
    // user accepted the transaction, tell the USB side
    btchip_bagl_user_action_display(1);
    // redraw ui
#ifndef HAVE_NBGL
    ui_idle();
#endif
    return 0; // DO NOT REDRAW THE BUTTON
}
