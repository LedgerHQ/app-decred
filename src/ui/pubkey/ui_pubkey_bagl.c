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
#include "ui_pubkey.h"
#include "ui_shared.h"
#include "os.h"
#include "os_io_seproxyhal.h"
#include "ux.h"
#include "string.h"
#include "btchip_internal.h"

#if defined(TARGET_NANOS)

unsigned int ui_display_address_nanos_button(unsigned int button_mask,
                                             unsigned int button_mask_counter);
unsigned int ui_request_pubkey_approval_nanos_button(unsigned int button_mask,
                                             unsigned int button_mask_counter);
unsigned int ui_display_token_nanos_button(unsigned int button_mask,
                                             unsigned int button_mask_counter);

const bagl_element_t ui_display_address_nanos[] = {

    UI_NANOS_BACKGROUND(),

    /* Displayed when derivation path is unusual */

    UI_NANOS_TEXT(1, 0, 22, 128, "Warning !", BAGL_FONT_OPEN_SANS_LIGHT_16px),

    UI_NANOS_TEXT(2, 0, 12, 128, "The derivation", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(2, 0, 26, 128, "path is unusual", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(3, 0, 12, 128, "Derivation path", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(0x83, 15, 26, 98, vars.tmp_warning.derivation_path, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

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
    UI_NANOS_SCROLLING_TEXT(0x86, 15, 26, 98, G_io_apdu_buffer + 199, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)
};

const bagl_element_t ui_request_pubkey_approval_nanos[] = {
    UI_NANOS_BACKGROUND(),
    UI_NANOS_ICON_LEFT(0, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(0, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(1, 0, 12, 128, "Export", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    // Hax, avoid wasting space
    UI_NANOS_TEXT(1, 0, 26, 128, "public key?", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)
};

const bagl_element_t ui_display_token_nanos[] = {

    UI_NANOS_BACKGROUND(),
    UI_NANOS_ICON_LEFT(0, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(0, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(1, 0, 12, 128, "Confirm token", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    // Hax, avoid wasting space
    UI_NANOS_TEXT(1, 0, 26, 128, G_io_apdu_buffer + 200, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)
};


unsigned int ui_display_address_nanos_prepro(const bagl_element_t *element) {

    if (element->component.userid > 0) {
        unsigned int display = (ux_step == (0x7F & element->component.userid) - 1);
        if (display) {
            switch (element->component.userid) {
            case 0x83:
                ux_loop_over_curr_element = 1;
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                break;
            case 5:
                UX_CALLBACK_SET_INTERVAL(2000);
                ux_loop_over_curr_element = 0; // allow next timer to increment ux_step when triggered
                break;
            case 0x86:
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                // ugly ux tricks, loops around last 2 screens
                ux_step -= 1; // loops back to previous element on next redraw
                ux_loop_over_curr_element = 1; // when the timer will trigger, ux_step won't be incremented, only redraw
                break;
            }
        }
        return display;
    }
    return 1;
}

unsigned int ui_display_address_nanos_button(unsigned int button_mask,
                                             unsigned int button_mask_counter) {
    if (ux_step == 3)
    {
        switch (button_mask)
        {
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
    }
    else if (ux_step >= 4)
    {
        switch (button_mask)
        {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            io_seproxyhal_touch_display_cancel(NULL);
            break;
    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
        io_seproxyhal_touch_display_ok(NULL);
        break;
        }
    }
    else
    {
        if(button_mask == (BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT))
        {
                // if we were looping over a single element, disable this loop and diffuse the redisplay timeout (used by scrolling text)
                if(ux_loop_over_curr_element) {
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

unsigned int ui_request_pubkey_approval_nanos_button(unsigned int button_mask,
                                             unsigned int button_mask_counter)
{
    switch (button_mask)
    {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        io_seproxyhal_touch_display_cancel(NULL);
        break;
     case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
        io_seproxyhal_touch_display_ok(NULL);
        break;
    }
    return 0;
}

unsigned int ui_display_token_nanos_button(unsigned int button_mask,
                                             unsigned int button_mask_counter)
{
    switch (button_mask)
    {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        io_seproxyhal_touch_display_token_cancel(NULL);
        break;
    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
        io_seproxyhal_touch_display_token_ok(NULL);
        break;
    }
    return 0;
}

#endif

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)

UX_STEP_NOCB(
    ux_display_public_flow_1_step,
    pnn, 
    {
      &C_icon_warning,
      "The derivation",
      "path is unusual!",
    });
UX_STEP_NOCB(
    ux_display_public_flow_2_step, 
    bnnn_paging, 
    {
      .title = "Derivation path",
      .text = vars.tmp_warning.derivation_path,
    });
UX_STEP_VALID(
    ux_display_public_flow_3_step, 
    pnn,
    io_seproxyhal_touch_display_cancel(NULL),
    {
      &C_icon_crossmark,
      "Reject if you're",
      "not sure",
    });
UX_STEP_NOCB(
    ux_display_public_flow_4_step,
    pnn, 
    {
      &C_icon_validate_14,
      "Approve derivation",
      "path",
    });
UX_STEP_NOCB(
    ux_display_public_flow_5_step, 
    bnnn_paging, 
    {
      .title = "Address",
      .text = G_io_apdu_buffer+200,
    });
UX_STEP_VALID(
    ux_display_public_flow_6_step, 
    pb,
    io_seproxyhal_touch_display_ok(NULL),
    {
      &C_icon_validate_14,
      "Approve",
    });
UX_STEP_VALID(
    ux_display_public_flow_7_step, 
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
  &ux_display_public_flow_7_step
);

UX_FLOW(ux_display_public_flow,
  &ux_display_public_flow_5_step,
  &ux_display_public_flow_6_step,
  &ux_display_public_flow_7_step
);

//////////////////////////////////////////////////////////////////////

UX_STEP_VALID(
    ux_request_pubkey_approval_flow_1_step,
    pbb,
    io_seproxyhal_touch_display_ok(NULL),
    {
      &C_icon_validate_14,
      "Export",
      "public key?",
    });
UX_STEP_VALID(
    ux_request_pubkey_approval_flow_2_step,
    pb,
    io_seproxyhal_touch_display_cancel(NULL),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_FLOW(ux_request_pubkey_approval_flow,
  &ux_request_pubkey_approval_flow_1_step,
  &ux_request_pubkey_approval_flow_2_step
);

//////////////////////////////////////////////////////////////////////

UX_STEP_VALID(
    ux_display_token_flow_1_step,
    pbb,
    io_seproxyhal_touch_display_ok(NULL),
    {
      &C_icon_validate_14,
      "Confirm token",
      G_io_apdu_buffer+200,
    });
UX_STEP_VALID(
    ux_display_token_flow_2_step,
    pb,
    io_seproxyhal_touch_display_cancel(NULL),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_FLOW(ux_display_token_flow,
  &ux_display_token_flow_1_step,
  &ux_display_token_flow_2_step
);

#endif

void btchip_bagl_display_public_key(unsigned char* derivation_path) {
    // append a white space at the end of the address to avoid glitch on nano S
    strcat(G_io_apdu_buffer + 200, " ");

    bip32_print_path(derivation_path, vars.tmp_warning.derivation_path, MAX_DERIV_PATH_ASCII_LENGTH);
    uint8_t is_derivation_path_unusual = bip44_derivation_guard(derivation_path, false);

#if defined(TARGET_NANOS)
    // prepend a white space to the address
    G_io_apdu_buffer[199] = ' ';
    ux_step = is_derivation_path_unusual?0:4;
    ux_step_count = 6;
    UX_DISPLAY(ui_display_address_nanos, ui_display_address_nanos_prepro);

#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, is_derivation_path_unusual?ux_display_public_with_warning_flow:ux_display_public_flow, NULL);
#endif // TARGET_NANOX
}

void btchip_bagl_request_pubkey_approval(void)
{
#if defined(TARGET_NANOS)
    // append and prepend a white space to the address
    ux_step = 0;
    ux_step_count = 1;
    UX_DISPLAY(ui_request_pubkey_approval_nanos, NULL);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_request_pubkey_approval_flow, NULL);
#endif // #if TARGET_ID
}

void btchip_bagl_display_token(void)
{
#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 1;
    UX_DISPLAY(ui_display_token_nanos, NULL);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_display_token_flow, NULL);
#endif // #if TARGET_ID
}
#endif // HAVE_BAGL
