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

#pragma once

#include <stdint.h>
#include "btchip_filesystem_tx.h"
#include "ux.h"

#define COLOR_WHITE 0xFFFFFF
#define UI_NANOS_BACKGROUND() \
    { {BAGL_RECTANGLE, 0, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0, COLOR_WHITE, 0, 0}, NULL }
#define UI_NANOS_TEXT(userid, x, y, w, text, font) \
    {                                              \
        {BAGL_LABELINE,                            \
         userid,                                   \
         x,                                        \
         y,                                        \
         w,                                        \
         12,                                       \
         0,                                        \
         0,                                        \
         0,                                        \
         COLOR_WHITE,                              \
         0,                                        \
         font | BAGL_FONT_ALIGNMENT_CENTER,        \
         0},                                       \
            (char *) text                          \
    }
#define UI_NANOS_ICON_LEFT(userid, glyph) \
    { {BAGL_ICON, userid, 3, 12, 7, 7, 0, 0, 0, COLOR_WHITE, 0, 0, glyph}, NULL }
#define UI_NANOS_ICON_RIGHT(userid, glyph) \
    { {BAGL_ICON, userid, 117, 13, 8, 6, 0, 0, 0, COLOR_WHITE, 0, 0, glyph}, NULL }
// Only one scrolling text per screen can be displayed
#define UI_NANOS_SCROLLING_TEXT(userid, x, y, w, text, font) \
    {                                                        \
        {BAGL_LABELINE,                                      \
         userid,                                             \
         x,                                                  \
         y,                                                  \
         w,                                                  \
         12,                                                 \
         0x80 | 10,                                          \
         0,                                                  \
         0,                                                  \
         COLOR_WHITE,                                        \
         0,                                                  \
         font | BAGL_FONT_ALIGNMENT_CENTER,                  \
         26},                                                \
            (char *) text                                    \
    }

typedef union {
    struct {
        // char addressSummary[40]; // beginning of the output address ... end
        // of

        char fullAddress[43];  // the address
        char fullAmount[20];   // full amount
        char feesAmount[20];   // fees
    } tmp;

    struct {
        char derivation_path[MAX_DERIV_PATH_ASCII_LENGTH];
    } tmp_warning;

    /*
    struct {
      bagl_icon_details_t icon_details;
      unsigned int colors[2];
      unsigned char qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(3)];
    } tmpqr;

    unsigned int dummy; // ensure the whole vars is aligned for the CM0 to
    operate correctly
    */
} vars_u_t;

extern vars_u_t vars;
extern unsigned int ux_step;
extern unsigned int ux_step_count;
extern uint8_t ux_loop_over_curr_element;

unsigned int io_seproxyhal_touch_display_cancel(const void *e);
unsigned int io_seproxyhal_touch_display_ok(const void *e);

unsigned int io_seproxyhal_touch_verify_cancel(const void *e);
unsigned int io_seproxyhal_touch_verify_ok(const void *e);
unsigned int io_seproxyhal_touch_message_signature_verify_cancel(const void *e);
unsigned int io_seproxyhal_touch_message_signature_verify_ok(const void *e);
unsigned int io_seproxyhal_touch_display_token_cancel(const void *e);
unsigned int io_seproxyhal_touch_display_token_ok(const void *e);

uint8_t prepare_fees();
uint8_t prepare_message_signature();
uint8_t prepare_single_output();
uint8_t prepare_full_output(uint8_t checkOnly);