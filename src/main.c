/*******************************************************************************
 *   Ledger App - Bitcoin Wallet
 *   (c) 2016-2019 Ledger
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

#include "os.h"
#include "cx.h"

#include "os_io_seproxyhal.h"
#include "string.h"

#include "btchip_internal.h"

#include "btchip_bagl_extensions.h"

#include "ux.h"

#define __NAME3(a, b, c) a##b##c
#define NAME3(a, b, c)   __NAME3(a, b, c)

#ifndef TARGET_FATSTACKS
bagl_element_t tmp_element;
#endif

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

#define BAGL_FONT_OPEN_SANS_LIGHT_16_22PX_AVG_WIDTH   10
#define BAGL_FONT_OPEN_SANS_REGULAR_10_13PX_AVG_WIDTH 8
#define MAX_CHAR_PER_LINE                             25

#define COLOR_BG_1       0xF9F9F9
#define COLOR_APP        COIN_COLOR_HDR  // bitcoin 0xFCB653
#define COLOR_APP_LIGHT  COIN_COLOR_DB   // bitcoin 0xFEDBA9
#define COLOR_BLACK      0x000000
#define COLOR_WHITE      0xFFFFFF
#define COLOR_GRAY       0x999999
#define COLOR_LIGHT_GRAY 0xEEEEEE

#define UI_NANOS_BACKGROUND()                                                                     \
    {                                                                                             \
        {BAGL_RECTANGLE, 0, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0, COLOR_WHITE, 0, 0}, NULL, 0, 0, 0, \
            NULL, NULL, NULL                                                                      \
    }
#define UI_NANOS_ICON_LEFT(userid, glyph)                                                         \
    {                                                                                             \
        {BAGL_ICON, userid, 3, 12, 7, 7, 0, 0, 0, COLOR_WHITE, 0, 0, glyph}, NULL, 0, 0, 0, NULL, \
            NULL, NULL                                                                            \
    }
#define UI_NANOS_ICON_RIGHT(userid, glyph)                                                    \
    {                                                                                         \
        {BAGL_ICON, userid, 117, 13, 8, 6, 0, 0, 0, COLOR_WHITE, 0, 0, glyph}, NULL, 0, 0, 0, \
            NULL, NULL, NULL                                                                  \
    }
#define UI_NANOS_TEXT(userid, x, y, w, text, font)   \
    {                                                \
        {BAGL_LABELINE,                              \
         userid,                                     \
         x,                                          \
         y,                                          \
         w,                                          \
         12,                                         \
         0,                                          \
         0,                                          \
         0,                                          \
         COLOR_WHITE,                                \
         0,                                          \
         font | BAGL_FONT_ALIGNMENT_CENTER,          \
         0},                                         \
            (char *) text, 0, 0, 0, NULL, NULL, NULL \
    }
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
            (char *) text, 0, 0, 0, NULL, NULL, NULL         \
    }

union {
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
} vars;

#ifndef TARGET_FATSTACKS
unsigned int io_seproxyhal_touch_verify_cancel(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_verify_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_message_signature_verify_cancel(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_message_signature_verify_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_display_cancel(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_display_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_display_token_cancel(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_display_token_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_settings(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_exit(const bagl_element_t *e);
#endif
void ui_idle(void);

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#include "ux.h"
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;
#else
ux_state_t ux;
#endif  // TARGET_NANOX || TARGET_NANOS2

// display stepped screens
unsigned int ux_step;
unsigned int ux_step_count;
uint8_t ux_loop_over_curr_element;  // Nano S only

#ifndef TARGET_FATSTACKS
const bagl_element_t *ui_menu_item_out_over(const bagl_element_t *e) {
    // the selection rectangle is after the none|touchable
    e = (const bagl_element_t *) (((unsigned int) e) + sizeof(bagl_element_t));
    return e;
}
#endif

#if defined(TARGET_NANOS)

const bagl_element_t ui_display_address_nanos[] = {

    UI_NANOS_BACKGROUND(),

    /* Displayed when derivation path is unusual */

    UI_NANOS_TEXT(1, 0, 22, 128, "Warning !", BAGL_FONT_OPEN_SANS_LIGHT_16px),

    UI_NANOS_TEXT(2, 0, 12, 128, "The derivation", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(2, 0, 26, 128, "path is unusual", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(3, 0, 12, 128, "Derivation path", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(0x83,
                            15,
                            26,
                            98,
                            vars.tmp_warning.derivation_path,
                            BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_ICON_LEFT(4, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(4, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(4, 0, 12, 128, "Reject if you're", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(4, 0, 26, 128, "not sure", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    /* Always displayed */

    UI_NANOS_ICON_LEFT(5, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(5, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(5, 0, 12, 128, "Confirm", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(5, 0, 26, 128, "address", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(6, 0, 12, 128, "Address", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    // Hax, avoid wasting space
    UI_NANOS_SCROLLING_TEXT(0x86,
                            15,
                            26,
                            98,
                            G_io_apdu_buffer + 199,
                            BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)};

const bagl_element_t ui_display_token_nanos[] = {

    UI_NANOS_BACKGROUND(),
    UI_NANOS_ICON_LEFT(0, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(0, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(1, 0, 12, 128, "Confirm token", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    // Hax, avoid wasting space
    UI_NANOS_TEXT(1, 0, 26, 128, G_io_apdu_buffer + 200, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)};
const bagl_element_t ui_request_pubkey_approval_nanos[] = {
    UI_NANOS_BACKGROUND(),
    UI_NANOS_ICON_LEFT(0, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(0, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(1, 0, 12, 128, "Export", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    // Hax, avoid wasting space
    UI_NANOS_TEXT(1, 0, 26, 128, "public key?", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)};

const bagl_element_t ui_request_change_path_approval_nanos[] = {
    UI_NANOS_BACKGROUND(),

    UI_NANOS_TEXT(1, 0, 22, 128, "Warning !", BAGL_FONT_OPEN_SANS_LIGHT_16px),

    UI_NANOS_TEXT(2, 0, 12, 128, "The change path", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(2, 0, 26, 128, "is unusual", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(3, 0, 12, 128, "Change path", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(0x83,
                            15,
                            26,
                            98,
                            vars.tmp_warning.derivation_path,
                            BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_ICON_LEFT(4, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(4, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(4, 0, 12, 128, "Reject if you're", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(4, 0, 26, 128, "not sure", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)};

unsigned int ui_display_address_nanos_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == (0x7F & element->component.userid) - 1);
        if (display) {
            switch (element->component.userid) {
                case 0x83:
                    ux_loop_over_curr_element = 1;
                    UX_CALLBACK_SET_INTERVAL(
                        MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                    break;
                case 5:
                    UX_CALLBACK_SET_INTERVAL(2000);
                    ux_loop_over_curr_element =
                        0;  // allow next timer to increment ux_step when triggered
                    break;
                case 0x86:
                    UX_CALLBACK_SET_INTERVAL(
                        MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                    // ugly ux tricks, loops around last 2 screens
                    ux_step -= 1;                   // loops back to previous element on next redraw
                    ux_loop_over_curr_element = 1;  // when the timer will trigger, ux_step won't be
                                                    // incremented, only redraw
                    break;
            }
        }
        return display;
    }
    return 1;
}

unsigned int ui_request_change_path_approval_nanos_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == (0x7F & element->component.userid) - 1);
        if (display) {
            if (element->component.userid & 0x80) {
                ux_loop_over_curr_element = 1;
                UX_CALLBACK_SET_INTERVAL(
                    MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            }
        }
        return display;
    }
    return 1;
}

unsigned int ui_display_address_nanos_button(unsigned int button_mask,
                                             unsigned int button_mask_counter);
unsigned int ui_display_token_nanos_button(unsigned int button_mask,
                                           unsigned int button_mask_counter);
unsigned int ui_request_pubkey_approval_nanos_button(unsigned int button_mask,
                                                     unsigned int button_mask_counter);
unsigned int ui_request_change_path_approval_nanos_button(unsigned int button_mask,
                                                          unsigned int button_mask_counter);

const bagl_element_t ui_verify_nanos[] = {
    UI_NANOS_BACKGROUND(),
    UI_NANOS_ICON_LEFT(0, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(0, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(1, 0, 12, 128, "Confirm", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(1, 0, 26, 128, "transaction", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(2, 0, 12, 128, "Amount", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(2, 23, 26, 82, vars.tmp.fullAmount, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(3, 0, 12, 128, "Address", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(3,
                            23,
                            26,
                            82,
                            vars.tmp.fullAddress,
                            BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(4, 0, 12, 128, "Fees", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(4, 23, 26, 82, vars.tmp.feesAmount, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)

};
unsigned int ui_verify_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);

const bagl_element_t ui_verify_output_nanos[] = {

    UI_NANOS_BACKGROUND(),
    UI_NANOS_ICON_LEFT(0, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(0, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(1, 0, 12, 128, "Confirm", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(1, 0, 26, 128, vars.tmp.feesAmount, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(2, 0, 12, 128, "Amount", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(2, 23, 26, 82, vars.tmp.fullAmount, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(3, 0, 12, 128, "Address", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(3,
                            23,
                            26,
                            82,
                            vars.tmp.fullAddress,
                            BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)};

unsigned int ui_verify_output_nanos_button(unsigned int button_mask,
                                           unsigned int button_mask_counter);

const bagl_element_t ui_finalize_nanos[] = {
    UI_NANOS_BACKGROUND(),
    UI_NANOS_ICON_LEFT(0, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(0, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(1, 0, 12, 128, "Confirm", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(1, 0, 26, 128, "transaction", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(2, 0, 12, 128, "Fees", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_SCROLLING_TEXT(2, 23, 26, 82, vars.tmp.feesAmount, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)

    /* TODO
    {{BAGL_LABELINE                       , 0x02,   0,  12, 128,  12, 0, 0, 0 ,
    COLOR_WHITE, COLOR_BLACK,
    BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0  }, "Amount",
    0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE                       , 0x02,  23,  26,  82,  12, 0x80|10,
    0, 0        , COLOR_WHITE, COLOR_BLACK,
    BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 26  },
    vars.tmp.fullAmount, 0, 0, 0, NULL, NULL, NULL },
    */
};
unsigned int ui_finalize_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);

// display or not according to step, and adjust delay
unsigned int ui_verify_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == element->component.userid - 1);
        if (display) {
            switch (element->component.userid) {
                case 1:
                    UX_CALLBACK_SET_INTERVAL(2000);
                    break;
                case 2:
                case 3:
                case 4:
                    UX_CALLBACK_SET_INTERVAL(
                        MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                    break;
            }
        }
        return display;
    }
    return 1;
}

unsigned int ui_verify_output_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == element->component.userid - 1);
        if (display) {
            switch (element->component.userid) {
                case 1:
                    UX_CALLBACK_SET_INTERVAL(2000);
                    break;
                case 2:
                case 3:
                    UX_CALLBACK_SET_INTERVAL(
                        MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                    break;
            }
        }
        return display;
    }
    return 1;
}

unsigned int ui_finalize_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == element->component.userid - 1);
        if (display) {
            switch (element->component.userid) {
                case 1:
                    UX_CALLBACK_SET_INTERVAL(2000);
                    break;
                case 2:
                    UX_CALLBACK_SET_INTERVAL(
                        MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                    break;
            }
        }
        return display;
    }
    return 1;
}

const bagl_element_t ui_verify_message_signature_nanos[] = {
    UI_NANOS_BACKGROUND(),
    UI_NANOS_ICON_LEFT(0, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(0, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(1, 0, 12, 128, "Sign the", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(1, 0, 26, 128, "message", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(2, 0, 12, 128, "Message hash", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(2,
                            23,
                            26,
                            82,
                            vars.tmp.fullAddress,
                            BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)};
unsigned int ui_verify_message_signature_nanos_button(unsigned int button_mask,
                                                      unsigned int button_mask_counter);

unsigned int ui_verify_message_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == element->component.userid - 1);
        if (display) {
            switch (element->component.userid) {
                case 1:
                    UX_CALLBACK_SET_INTERVAL(2000);
                    break;
                case 2:
                    UX_CALLBACK_SET_INTERVAL(
                        MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                    break;
            }
        }
        return display;
    }
    return 1;
}

#endif // #if defined(TARGET_NANOS)
#ifndef TARGET_FATSTACKS
unsigned int io_seproxyhal_touch_verify_cancel(const bagl_element_t *e) {
    // user denied the transaction, tell the USB side
    if (!btchip_bagl_user_action(0)) {
        // redraw ui
        ui_idle();
    }
    return 0;  // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_verify_ok(const bagl_element_t *e) {
    // user accepted the transaction, tell the USB side
    if (!btchip_bagl_user_action(1)) {
        // redraw ui
        ui_idle();
    }
    return 0;  // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_message_signature_verify_cancel(const bagl_element_t *e) {
    // user denied the transaction, tell the USB side
    btchip_bagl_user_action_message_signing(0);
    // redraw ui
    ui_idle();
    return 0;  // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_message_signature_verify_ok(const bagl_element_t *e) {
    // user accepted the transaction, tell the USB side
    btchip_bagl_user_action_message_signing(1);
    // redraw ui
    ui_idle();
    return 0;  // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_display_cancel(const bagl_element_t *e) {
    // user denied the transaction, tell the USB side
    btchip_bagl_user_action_display(0);
    // redraw ui
    ui_idle();
    return 0;  // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_display_ok(const bagl_element_t *e) {
    // user accepted the transaction, tell the USB side
    btchip_bagl_user_action_display(1);
    // redraw ui
    ui_idle();
    return 0;  // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_display_token_cancel(const bagl_element_t *e) {
    // revoke previous valid token if there was one
    btchip_context_D.has_valid_token = false;
    // user denied the token, tell the USB side
    btchip_bagl_user_action_display(0);
    // redraw ui
    ui_idle();
    return 0;  // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_display_token_ok(const bagl_element_t *e) {
    // Set the valid token flag
    btchip_context_D.has_valid_token = true;
    // user approved the token, tell the USB side
    btchip_bagl_user_action_display(1);
    // redraw ui
    ui_idle();
    return 0;  // DO NOT REDRAW THE BUTTON
}
#endif

#if defined(TARGET_NANOS)
unsigned int ui_verify_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            io_seproxyhal_touch_verify_cancel(NULL);
            break;

        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            io_seproxyhal_touch_verify_ok(NULL);
            break;
    }
    return 0;
}

unsigned int ui_verify_output_nanos_button(unsigned int button_mask,
                                           unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            io_seproxyhal_touch_verify_cancel(NULL);
            break;

        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            io_seproxyhal_touch_verify_ok(NULL);
            break;
    }
    return 0;
}

unsigned int ui_finalize_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            io_seproxyhal_touch_verify_cancel(NULL);
            break;

        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            io_seproxyhal_touch_verify_ok(NULL);
            break;
    }
    return 0;
}

unsigned int ui_verify_message_signature_nanos_button(unsigned int button_mask,
                                                      unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            io_seproxyhal_touch_message_signature_verify_cancel(NULL);
            break;

        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            io_seproxyhal_touch_message_signature_verify_ok(NULL);
            break;
    }
    return 0;
}

unsigned int ui_display_address_nanos_button(unsigned int button_mask,
                                             unsigned int button_mask_counter) {
    if (ux_step == 3) {
        switch (button_mask) {
            case BUTTON_EVT_RELEASED | BUTTON_LEFT:
                io_seproxyhal_touch_display_cancel(NULL);
                break;
            case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
                // prepare next screen
                ux_step = (ux_step + 1) % ux_step_count;
                // redisplay screen
                UX_REDISPLAY();
                break;
        }
    } else if (ux_step >= 4) {
        switch (button_mask) {
            case BUTTON_EVT_RELEASED | BUTTON_LEFT:
                io_seproxyhal_touch_display_cancel(NULL);
                break;
            case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
                io_seproxyhal_touch_display_ok(NULL);
                break;
        }
    } else {
        if (button_mask == (BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT)) {
            // if we were looping over a single element, disable this loop and diffuse the redisplay
            // timeout (used by scrolling text)
            if (ux_loop_over_curr_element) {
                ux_loop_over_curr_element = 0;
                ux.callback_interval_ms = 0;
            }
            // prepare next screen
            ux_step = (ux_step + 1) % ux_step_count;
            // redisplay screen
            UX_REDISPLAY();
        }
    }
    return 0;
}

unsigned int ui_display_token_nanos_button(unsigned int button_mask,
                                           unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            io_seproxyhal_touch_display_token_cancel(NULL);
            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            io_seproxyhal_touch_display_token_ok(NULL);
            break;
    }
    return 0;
}
unsigned int ui_request_pubkey_approval_nanos_button(unsigned int button_mask,
                                                     unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            io_seproxyhal_touch_display_cancel(NULL);
            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            io_seproxyhal_touch_display_ok(NULL);
            break;
    }
    return 0;
}

unsigned int ui_request_change_path_approval_nanos_button(unsigned int button_mask,
                                                          unsigned int button_mask_counter) {
    if (ux_step == 3) {
        switch (button_mask) {
            case BUTTON_EVT_RELEASED | BUTTON_LEFT:
                io_seproxyhal_touch_display_cancel(NULL);
                break;
            case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
                io_seproxyhal_touch_display_ok(NULL);
                break;
        }
    } else {
        if (button_mask == (BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT)) {
            // if we were looping over a single element, disable this loop and diffuse the redisplay
            // timeout (used by scrolling text)
            if (ux_loop_over_curr_element) {
                ux_loop_over_curr_element = 0;
                ux.callback_interval_ms = 0;
            }
            // prepare next screen
            ux_step = (ux_step + 1) % ux_step_count;
            // redisplay screen
            UX_REDISPLAY();
        }
    }
    return 0;
}

#endif  // #if defined(TARGET_NANOS)

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)

const char *settings_submenu_getter(unsigned int idx);
void settings_submenu_selector(unsigned int idx);

void settings_pubkey_export_change(unsigned int enabled) {
    nvm_write((void *) &N_btchip.pubKeyRequestRestriction, &enabled, 1);
    ui_idle();
}
//////////////////////////////////////////////////////////////////////////////////////
// Public keys export submenu:

const char *const settings_pubkey_export_getter_values[] = {"Auto Approval",
                                                            "Manual Approval",
                                                            "Back"};

const char *settings_pubkey_export_getter(unsigned int idx) {
    if (idx < ARRAYLEN(settings_pubkey_export_getter_values)) {
        return settings_pubkey_export_getter_values[idx];
    }
    return NULL;
}

void settings_pubkey_export_selector(unsigned int idx) {
    switch (idx) {
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

const char *const settings_submenu_getter_values[] = {
    "Public keys export",
    "Back",
};

const char *settings_submenu_getter(unsigned int idx) {
    if (idx < ARRAYLEN(settings_submenu_getter_values)) {
        return settings_submenu_getter_values[idx];
    }
    return NULL;
}

void settings_submenu_selector(unsigned int idx) {
    switch (idx) {
        case 0:
            ux_menulist_init_select(0,
                                    settings_pubkey_export_getter,
                                    settings_pubkey_export_selector,
                                    N_btchip.pubKeyRequestRestriction);
            break;
        default:
            ui_idle();
    }
}

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(ux_idle_flow_1_step,
             nn,
             {
                 "Application",
                 "is ready",
             });
UX_STEP_VALID(ux_idle_flow_2_step,
              pb,
              ux_menulist_init(0, settings_submenu_getter, settings_submenu_selector),
              {
                  &C_icon_coggle,
                  "Settings",
              });
UX_STEP_NOCB(ux_idle_flow_3_step,
             bn,
             {
                 "Version",
                 APPVERSION,
             });
UX_STEP_VALID(ux_idle_flow_4_step,
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
        &ux_idle_flow_4_step);

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
                  "Sign",
                  "message",
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

UX_STEP_NOCB(ux_confirm_single_flow_1_step, pnn, {&C_icon_eye, "Review", "transaction"});
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
UX_STEP_NOCB(ux_confirm_single_flow_4_step,
             bnnn_paging,
             {
                 .title = "Fees",
                 .text = vars.tmp.feesAmount,
             });
UX_STEP_VALID(ux_confirm_single_flow_5_step,
              pb,
              io_seproxyhal_touch_verify_ok(NULL),
              {
                  &C_icon_validate_14,
                  "Accept",
              });
UX_STEP_VALID(ux_confirm_single_flow_6_step,
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
        &ux_confirm_single_flow_5_step,
        &ux_confirm_single_flow_6_step);

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
                 .text = G_io_apdu_buffer + 200,
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
UX_STEP_VALID(ux_display_token_flow_1_step,
              pbb,
              io_seproxyhal_touch_display_ok(NULL),
              {
                  &C_icon_validate_14,
                  "Confirm token",
                  G_io_apdu_buffer + 200,
              });
UX_STEP_VALID(ux_display_token_flow_2_step,
              pb,
              io_seproxyhal_touch_display_cancel(NULL),
              {
                  &C_icon_crossmark,
                  "Reject",
              });

UX_FLOW(ux_display_token_flow, &ux_display_token_flow_1_step, &ux_display_token_flow_2_step);

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

#endif  // TARGET_NANOX || TARGET_NANOS2

void ui_idle(void) {
    ux_step_count = 0;
    ux_loop_over_curr_element = 0;

#elif defined(TARGET_NANOS)
    UX_MENU_DISPLAY(0, menu_main, NULL);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    // reserve a display stack slot if none yet
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
#endif  // #if TARGET_ID
}

#ifndef TARGET_FATSTACKS
// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    if ((element->component.type & (~BAGL_TYPE_FLAGS_MASK)) != BAGL_NONE) {
        io_seproxyhal_display_default((bagl_element_t *) element);
    }
}
#endif

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
        case CHANNEL_KEYBOARD:
            break;

        // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
        case CHANNEL_SPI:
            if (tx_len) {
                io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

                if (channel & IO_RESET_AFTER_REPLIED) {
                    reset();
                }
                return 0;  // nothing received from the master so far (it's a tx
                           // transaction)
            } else {
                return io_seproxyhal_spi_recv(G_io_apdu_buffer, sizeof(G_io_apdu_buffer), 0);
            }

        default:
            THROW(INVALID_PARAMETER);
    }
    return 0;
}

unsigned char io_event(unsigned char channel) {
    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_FINGER_EVENT:
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
            UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_STATUS_EVENT:
            if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
                !(U4BE(G_io_seproxyhal_spi_buffer, 3) &
                  SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
                THROW(EXCEPTION_IO_RESET);
            }
        // no break is intentional
        default:
            UX_DEFAULT_EVENT();
            break;

        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
            UX_DISPLAYED_EVENT({});
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT:
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
                // don't redisplay if UX not allowed (pin locked in the common bolos
                // ux ?)
                if (ux_step_count && UX_ALLOWED) {
                    // prepare next screen
                    if (!ux_loop_over_curr_element) {
                        ux_step = (ux_step + 1) % ux_step_count;
                    }
                    // redisplay screen
                    UX_REDISPLAY();
                }
            });
            break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

uint8_t prepare_fees() {
    if (btchip_context_D.transactionContext.relaxed) {
        os_memmove(vars.tmp.feesAmount, "UNKNOWN", 7);
        vars.tmp.feesAmount[7] = '\0';
    } else {
        unsigned char fees[8];
        unsigned short textSize;
        if (transaction_amount_sub_be(fees,
                                      btchip_context_D.transactionContext.transactionAmount,
                                      btchip_context_D.totalOutputAmount)) {
            PRINTF("Fees: %.*H\n", 8, fees);
            PRINTF("transactionAmount:: %.*H\n",
                   8,
                   btchip_context_D.transactionContext.transactionAmount);
            PRINTF("totalOutputAmount: %.*H\n", 8, btchip_context_D.totalOutputAmount);
            PRINTF("Error : Fees not consistent");
            goto error;
        }
        os_memmove(vars.tmp.feesAmount,
                   btchip_context_D.shortCoinId,
                   btchip_context_D.shortCoinIdLength);
        vars.tmp.feesAmount[btchip_context_D.shortCoinIdLength] = ' ';
        btchip_context_D.tmp =
            (unsigned char *) (vars.tmp.feesAmount + btchip_context_D.shortCoinIdLength + 1);
        textSize = btchip_convert_hex_amount_to_displayable(fees);
        vars.tmp.feesAmount[textSize + btchip_context_D.shortCoinIdLength + 1] = '\0';
    }
    return 1;
error:
    return 0;
}

uint8_t prepare_single_output() {
    // TODO : special display for OP_RETURN
    unsigned char amount[8];
    char tmp[80];
    unsigned int offset = 0;
    unsigned char versionSize;
    int addressOffset;
    unsigned char address[22];
    unsigned short version;  // addr prefix, or net id
    unsigned short textSize;
    unsigned char script_version[2];  // Decred thing

    vars.tmp.fullAddress[0] = '\0';
    btchip_swap_bytes(amount, btchip_context_D.currentOutput + offset, 8);
    offset += 8;

    btchip_swap_bytes(script_version, btchip_context_D.currentOutput + offset, 2);
    offset += 2;

    PRINTF("amount: %.*H\n", 8, amount);

    if (btchip_output_script_is_op_return(btchip_context_D.currentOutput + offset)) {
        strcpy(vars.tmp.fullAddress, "OP_RETURN");
    } else if (btchip_output_script_is_regular(btchip_context_D.currentOutput + offset)) {
        addressOffset = offset + 4;
        version = btchip_context_D.payToAddressVersion;
    } else {
        addressOffset = offset + 3;
        version = btchip_context_D.payToScriptHashVersion;
    }
    if (vars.tmp.fullAddress[0] == 0) {
        if (version > 255) {
            versionSize = 2;
            address[0] = (version >> 8);
            address[1] = version;
        } else {
            versionSize = 1;
            address[0] = version;
        }
        os_memmove(address + versionSize, btchip_context_D.currentOutput + addressOffset, 20);

        // Prepare address
        textSize = btchip_public_key_to_encoded_base58(address,
                                                       20 + versionSize,
                                                       (unsigned char *) tmp,
                                                       sizeof(tmp),
                                                       version,
                                                       1);
        tmp[textSize] = '\0';

        strcpy(vars.tmp.fullAddress, tmp);
    }

    // Prepare amount

    os_memmove(vars.tmp.fullAmount,
               btchip_context_D.shortCoinId,
               btchip_context_D.shortCoinIdLength);
    vars.tmp.fullAmount[btchip_context_D.shortCoinIdLength] = ' ';
    btchip_context_D.tmp =
        (unsigned char *) (vars.tmp.fullAmount + btchip_context_D.shortCoinIdLength + 1);
    textSize = btchip_convert_hex_amount_to_displayable(amount);
    vars.tmp.fullAmount[textSize + btchip_context_D.shortCoinIdLength + 1] = '\0';

    return 1;
}

uint8_t prepare_full_output(uint8_t checkOnly) {
    PRINTF("prepare full output (check= %d):\n", checkOnly);
    unsigned int offset = 0;
    int numberOutputs;
    int i;
    unsigned int currentPos = 0;
    unsigned char amount[8], totalOutputAmount[8], fees[8];
    char tmp[80];
    unsigned char outputPos = 0, changeFound = 0;
    unsigned char script_version[2];  // Decred thing

    if (btchip_context_D.transactionContext.relaxed &&
        !btchip_context_D.transactionContext.consumeP2SH) {
        if (!checkOnly) {
            PRINTF("Error : Mixed inputs");
        }
        goto error;
    }
    if (btchip_context_D.transactionContext.consumeP2SH) {
        if (checkOnly) {
            goto error;
        }
        vars.tmp.fullAmount[0] = '\0';
        vars.tmp.feesAmount[0] = '\0';
        strcpy(vars.tmp.fullAddress, "P2SH");
        return 1;
    }
    // Parse output, locate the change output location
    os_memset(totalOutputAmount, 0, sizeof(totalOutputAmount));
    numberOutputs = btchip_context_D.currentOutput[offset++];
    PRINTF("%d outputs\n", numberOutputs);
    if (numberOutputs > 3) {
        if (!checkOnly) {
            PRINTF("Error : Too many outputs");
        }
        goto error;
    }
    for (i = 0; i < numberOutputs; i++) {
        unsigned char nullAmount = 1;
        unsigned int j;
        unsigned char isOpReturn, isP2sh;
        unsigned char isOpCreate, isOpCall;

        for (j = 0; j < 8; j++) {
            if (btchip_context_D.currentOutput[offset + j] != 0) {
                nullAmount = 0;
                break;
            }
        }
        btchip_swap_bytes(amount, btchip_context_D.currentOutput + offset, 8);
        transaction_amount_add_be(totalOutputAmount, totalOutputAmount, amount);
        offset += 8;  // skip amount

        btchip_swap_bytes(script_version, btchip_context_D.currentOutput + offset, 2);
        offset += 2;  // skip script_version

        isOpReturn = btchip_output_script_is_op_return(btchip_context_D.currentOutput + offset);
        isP2sh = btchip_output_script_is_p2sh(btchip_context_D.currentOutput + offset);
        isOpCreate = btchip_output_script_is_op_create(btchip_context_D.currentOutput + offset);
        isOpCall = btchip_output_script_is_op_call(btchip_context_D.currentOutput + offset);
        PRINTF("REGULAR SCRIPT: %d\n",
               btchip_output_script_is_regular(btchip_context_D.currentOutput + offset));
        // Always notify OP_RETURN to the user
        if (nullAmount && isOpReturn) {
            if (!checkOnly) {
                PRINTF("Error : Unexpected OP_RETURN");
            }
            goto error;
        }
        if (!btchip_output_script_is_regular(btchip_context_D.currentOutput + offset) && !isP2sh &&
            !(nullAmount && isOpReturn) && (!isOpCreate && !isOpCall)) {
            if (!checkOnly) {
                PRINTF("Error : Unrecognized input script");
            }
            goto error;
        } else if (!btchip_output_script_is_regular(btchip_context_D.currentOutput + offset) &&
                   !isP2sh && !(nullAmount && isOpReturn)) {
            if (!checkOnly) {
                PRINTF("Error : Unrecognized input script");
            }
            goto error;
        }
        if (btchip_context_D.tmpCtx.output.changeInitialized && !isOpReturn) {
            unsigned char addressOffset =
                (isP2sh ? OUTPUT_SCRIPT_P2SH_PRE_LENGTH : OUTPUT_SCRIPT_REGULAR_PRE_LENGTH);
            if (os_memcmp(btchip_context_D.currentOutput + offset + addressOffset,
                          btchip_context_D.tmpCtx.output.changeAddress + 1,
                          20) == 0) {
                if (changeFound) {
                    if (!checkOnly) {
                        PRINTF("Error : Multiple change output found");
                    }
                    goto error;
                }
                changeFound = 1;
            } else {
                // outputPos is the real output pointer (opposed to the change address output)
                outputPos = currentPos;
            }
        }
        offset += 1 + btchip_context_D.currentOutput[offset];
        currentPos++;
    }
    if (btchip_context_D.tmpCtx.output.changeInitialized && !changeFound) {
        if (!checkOnly) {
            PRINTF("Error : change output not found");
        }
        goto error;
    }
    if (transaction_amount_sub_be(fees,
                                  btchip_context_D.transactionContext.transactionAmount,
                                  totalOutputAmount)) {
        PRINTF("tx_amount: ", 8, btchip_context_D.transactionContext.transactionAmount);
        PRINTF("total_amount: ", 8, totalOutputAmount);
        if (!checkOnly) {
            PRINTF("Error : Fees not consistent");
        }
        goto error;
    }
    if (!checkOnly) {
        // Format validation message
        currentPos = 0;
        offset = 1;
        btchip_context_D.tmp = (unsigned char *) tmp;
        for (i = 0; i < numberOutputs; i++) {
            if (!btchip_output_script_is_op_return(btchip_context_D.currentOutput + offset + 8 +
                                                   2)) {
                unsigned char versionSize;
                int addressOffset;
                unsigned char address[22];
                unsigned short version;

                btchip_swap_bytes(amount, btchip_context_D.currentOutput + offset, 8);
                offset += 8;  // skip amount

                btchip_swap_bytes(script_version, btchip_context_D.currentOutput + offset, 2);
                offset += 2;  // skip script_version

                if (btchip_output_script_is_regular(btchip_context_D.currentOutput + offset)) {
                    addressOffset = offset + 4;
                    version = btchip_context_D.payToAddressVersion;
                } else {
                    addressOffset = offset + 3;
                    version = btchip_context_D.payToScriptHashVersion;
                }
                if (version > 255) {
                    versionSize = 2;
                    address[0] = (version >> 8);
                    address[1] = version;
                } else {
                    versionSize = 1;
                    address[0] = version;
                }
                os_memmove(address + versionSize,
                           btchip_context_D.currentOutput + addressOffset,
                           20);

                // if we're processing the real output (not the change one)
                if (currentPos == outputPos) {
                    unsigned short textSize = 0;

                    // Prepare address
                    textSize = btchip_public_key_to_encoded_base58(address,
                                                                   20 + versionSize,
                                                                   (unsigned char *) tmp,
                                                                   sizeof(tmp),
                                                                   version,
                                                                   1);
                    tmp[textSize] = '\0';

                    strcpy(vars.tmp.fullAddress, tmp);

                    // Prepare amount
                    PRINTF("prepare amount\n");

                    os_memmove(vars.tmp.fullAmount,
                               btchip_context_D.shortCoinId,
                               btchip_context_D.shortCoinIdLength);
                    vars.tmp.fullAmount[btchip_context_D.shortCoinIdLength] = ' ';
                    btchip_context_D.tmp =
                        (unsigned char *) (vars.tmp.fullAmount +
                                           btchip_context_D.shortCoinIdLength + 1);
                    textSize = btchip_convert_hex_amount_to_displayable(amount);
                    vars.tmp.fullAmount[textSize + btchip_context_D.shortCoinIdLength + 1] = '\0';

                    // prepare fee display
                    PRINTF("prepare fee display\n");
                    os_memmove(vars.tmp.feesAmount,
                               btchip_context_D.shortCoinId,
                               btchip_context_D.shortCoinIdLength);
                    vars.tmp.feesAmount[btchip_context_D.shortCoinIdLength] = ' ';
                    btchip_context_D.tmp =
                        (unsigned char *) (vars.tmp.feesAmount +
                                           btchip_context_D.shortCoinIdLength + 1);
                    textSize = btchip_convert_hex_amount_to_displayable(fees);
                    vars.tmp.feesAmount[textSize + btchip_context_D.shortCoinIdLength + 1] = '\0';
                    break;
                }
            } else {
                // amount + version
                offset += 8 + 2;
            }
            offset += 1 + btchip_context_D.currentOutput[offset];
            currentPos++;
        }
    }
    return 1;
error:
    return 0;
}

#define HASH_LENGTH 4
uint8_t prepare_message_signature() {
    uint8_t buffer[32];

    cx_hash(&btchip_context_D.transactionHashWitness.header,
            CX_LAST,
            vars.tmp.fullAmount,
            0,
            buffer,
            32);

    snprintf(vars.tmp.fullAddress,
             sizeof(vars.tmp.fullAddress),
             "%.*H...%.*H",
             8,
             buffer,
             8,
             buffer + 32 - 8);
    return 1;
}

unsigned int btchip_bagl_confirm_full_output() {
    if (!prepare_full_output(0)) {
        return 0;
    }

#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 4;
    UX_DISPLAY(ui_verify_nanos, ui_verify_prepro);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_confirm_full_flow, NULL);
#endif  // TARGET_NANOX || TARGET_NANOS2
    return 1;
}

unsigned int btchip_bagl_confirm_single_output() {
// TODO : remove when supporting multi output
    if (!prepare_single_output()) {
        return 0;
    }

    snprintf(vars.tmp.feesAmount,
             sizeof(vars.tmp.feesAmount),
             "output #%d",
             btchip_context_D.totalOutputs - btchip_context_D.remainingOutputs + 1);

#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 3;
    UX_DISPLAY(ui_verify_output_nanos, ui_verify_output_prepro);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_confirm_single_flow, NULL);
#endif  // TARGET_NANOX || TARGET_NANOS2
    return 1;
}

unsigned int btchip_bagl_finalize_tx() {
    if (!prepare_fees()) {
        return 0;
    }

#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 2;
    UX_DISPLAY(ui_finalize_nanos, ui_finalize_prepro);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_finalize_flow, NULL);
#endif  // TARGET_NANOX
    return 1;
}

void btchip_bagl_confirm_message_signature() {
    if (!prepare_message_signature()) {
        return;
    }

#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 2;
    UX_DISPLAY(ui_verify_message_signature_nanos, ui_verify_message_prepro);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_sign_flow, NULL);
#endif  // TARGET_NANOX
}

void btchip_bagl_display_public_key(unsigned char *derivation_path) {
    // append a white space at the end of the address to avoid glitch on nano S
    strcat(G_io_apdu_buffer + 200, " ");

    bip32_print_path(derivation_path,
                     vars.tmp_warning.derivation_path,
                     MAX_DERIV_PATH_ASCII_LENGTH);
    uint8_t is_derivation_path_unusual = bip44_derivation_guard(derivation_path, false);

#if defined(TARGET_NANOS)
    // prepend a white space to the address
    G_io_apdu_buffer[199] = ' ';
    ux_step = is_derivation_path_unusual ? 0 : 4;
    ux_step_count = 6;
    UX_DISPLAY(ui_display_address_nanos, ui_display_address_nanos_prepro);

#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(
        0,
        is_derivation_path_unusual ? ux_display_public_with_warning_flow : ux_display_public_flow,
        NULL);
#endif  // TARGET_NANOX
}

void btchip_bagl_display_token()
{
#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 1;
    UX_DISPLAY(ui_display_token_nanos, NULL);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_display_token_flow, NULL);
#endif  // #if TARGET_ID
}

void btchip_bagl_request_pubkey_approval()
{
#if defined(TARGET_NANOS)
    // append and prepend a white space to the address
    ux_step = 0;
    ux_step_count = 1;
    UX_DISPLAY(ui_request_pubkey_approval_nanos, NULL);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_request_pubkey_approval_flow, NULL);
#endif  // #if TARGET_ID
}

void btchip_bagl_request_change_path_approval(unsigned char *change_path) {
    bip32_print_path(change_path, vars.tmp_warning.derivation_path, MAX_DERIV_PATH_ASCII_LENGTH);
#if defined(TARGET_NANOS)
    // append and prepend a white space to the address
    ux_step = 0;
    ux_step_count = 4;
    UX_DISPLAY(ui_request_change_path_approval_nanos, ui_request_change_path_approval_nanos_prepro);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_request_change_path_approval_flow, NULL);
#endif  // #if TARGET_ID
}

void app_exit(void) {
    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {
        }
    }
    END_TRY_L(exit);
}

// used when application is compiled statically (no lib dependency)
btchip_altcoin_config_t const C_coin_config = {
    .p2pkh_version = COIN_P2PKH_VERSION,
    .p2sh_version = COIN_P2SH_VERSION,
    .family = COIN_FAMILY,
// unsigned char* iconsuffix;// will use the icon provided on the stack (maybe)
    .coinid = COIN_COINID,
    .name = COIN_COINID_NAME,
    .name_short = COIN_COINID_SHORT,

#ifdef COIN_FORKID
    .forkid = COIN_FORKID,
#endif  // COIN_FORKID
#ifdef COIN_FLAGS
    .flags = COIN_FLAGS,
#endif  // COIN_FLAGS
    .kind = COIN_KIND,
};

__attribute__((section(".boot"))) int main(int arg0) {
#ifdef USE_LIB_DECRED
    // in RAM allocation (on stack), to allow simple simple traversal into the
    // bitcoin app (separate NVRAM zone)
    unsigned int libcall_params[3];
    unsigned char coinid[sizeof(COIN_COINID)];
    strcpy(coinid, COIN_COINID);
    unsigned char name[sizeof(COIN_COINID_NAME)];
    strcpy(name, COIN_COINID_NAME);
    unsigned char name_short[sizeof(COIN_COINID_SHORT)];
    strcpy(name_short, COIN_COINID_SHORT);

    btchip_altcoin_config_t coin_config;
    os_memmove(&coin_config, &C_coin_config, sizeof(coin_config));
    coin_config.coinid = coinid;
    coin_config.name = name;
    coin_config.name_short = name_short;

    BEGIN_TRY {
        TRY {
            // ensure syscall will accept us
            check_api_level(CX_COMPAT_APILEVEL);
            // delegate to bitcoin app/lib
            libcall_params[0] = "Decred";
            libcall_params[1] = 0x100;  // use the Init call, as we won't exit
            libcall_params[2] = &coin_config;
            os_lib_call(&libcall_params);
        }
        FINALLY {
            app_exit();
        }
    }
    END_TRY;
// no return
#else
    // exit critical section
    __asm volatile("cpsie i");

    if (arg0) {
        // is ID 1 ?
        if (((unsigned int *) arg0)[0] != 0x100) {
            app_exit();
            return 0;
        }
        // grab the coin config structure from the first parameter
        G_coin_config = (btchip_altcoin_config_t *) ((unsigned int *) arg0)[1];
    } else {
        G_coin_config = (btchip_altcoin_config_t *) PIC(&C_coin_config);
    }

    // ensure exception will work as planned
    os_boot();

    for (;;) {
        UX_INIT();
        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

#ifdef TARGET_NANOX
                // grab the current plane mode setting
                G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
#endif  // TARGET_NANOX

                btchip_context_init();

                USB_power(0);
                USB_power(1);

                ui_idle();

#ifdef HAVE_BLE
                BLE_power(0, NULL);
                BLE_power(1, "Nano X");
#endif  // HAVE_BLE

                app_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
                // reset IO and UX
                CLOSE_TRY;
                continue;
            }
            CATCH_ALL {
                CLOSE_TRY;
                break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }
    app_exit();
#endif  // USE_LIB_DECRED
    return 0;
}
