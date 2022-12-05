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
#include "ui_transaction.h"
#include "ui_shared.h"
#include "os.h"
#include "os_io_seproxyhal.h"
#include "ux.h"
#include "string.h"
#include "btchip_internal.h"

#if defined(TARGET_NANOS)

const bagl_element_t ui_request_change_path_approval_nanos[] = {
    UI_NANOS_BACKGROUND(),

    UI_NANOS_TEXT(1, 0, 22, 128, "Warning !", BAGL_FONT_OPEN_SANS_LIGHT_16px),

    UI_NANOS_TEXT(2, 0, 12, 128, "The change path", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(2, 0, 26, 128, "is unusual", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(3, 0, 12, 128, "Change path", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(0x83, 15, 26, 98, vars.tmp_warning.derivation_path, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_ICON_LEFT(4, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(4, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(4, 0, 12, 128, "Reject if you're", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(4, 0, 26, 128, "not sure", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)
};

unsigned int ui_request_change_path_approval_nanos_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == (0x7F & element->component.userid) - 1);
        if (display) {
            if (element->component.userid & 0x80) {
                ux_loop_over_curr_element = 1;
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            }
        }
        return display;
    }
    return 1;
}

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
    UI_NANOS_SCROLLING_TEXT(3, 23, 26, 82, vars.tmp.fullAddress, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(4, 0, 12, 128, "Fees", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(4, 23, 26, 82, vars.tmp.feesAmount, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)

};
unsigned int ui_verify_nanos_button(unsigned int button_mask,
                                    unsigned int button_mask_counter);

const bagl_element_t ui_verify_output_nanos[] = {

    UI_NANOS_BACKGROUND(),
    UI_NANOS_ICON_LEFT(0, BAGL_GLYPH_ICON_CROSS),
    UI_NANOS_ICON_RIGHT(0, BAGL_GLYPH_ICON_CHECK),
    UI_NANOS_TEXT(1, 0, 12, 128, "Confirm", BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),
    UI_NANOS_TEXT(1, 0, 26, 128, vars.tmp.feesAmount, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(2, 0, 12, 128, "Amount", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(2, 23, 26, 82, vars.tmp.fullAmount, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px),

    UI_NANOS_TEXT(3, 0, 12, 128, "Address", BAGL_FONT_OPEN_SANS_REGULAR_11px),
    UI_NANOS_SCROLLING_TEXT(3, 23, 26, 82, vars.tmp.fullAddress, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)
};

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
unsigned int ui_finalize_nanos_button(unsigned int button_mask,
                                      unsigned int button_mask_counter);

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
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
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
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
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
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
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
    UI_NANOS_SCROLLING_TEXT(2, 23, 26, 82, vars.tmp.fullAddress, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px)
};
unsigned int
ui_verify_message_signature_nanos_button(unsigned int button_mask,
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
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                break;
            }
        }
        return display;
    }
    return 1;
}

unsigned int ui_verify_nanos_button(unsigned int button_mask,
                                    unsigned int button_mask_counter) {
    UNUSED(button_mask_counter);
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
    UNUSED(button_mask_counter);
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

unsigned int ui_finalize_nanos_button(unsigned int button_mask,
                                      unsigned int button_mask_counter) {
    UNUSED(button_mask_counter);
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
    UNUSED(button_mask_counter);
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

 unsigned int ui_request_change_path_approval_nanos_button(unsigned int button_mask,
                                             unsigned int button_mask_counter)
{
    UNUSED(button_mask_counter);
    if (ux_step == 3)
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

#endif // #if defined(TARGET_NANOS)

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(
    ux_sign_flow_1_step,
    pnn, 
    {
      &C_icon_certificate,
      "Sign",
      "message",
    });
UX_STEP_NOCB(
    ux_sign_flow_2_step,
    bnnn_paging, 
    {
      .title = "Message hash",
      .text = vars.tmp.fullAddress,
    });
UX_STEP_VALID(
    ux_sign_flow_3_step,
    pbb,
    io_seproxyhal_touch_message_signature_verify_ok(NULL),
    {
      &C_icon_validate_14,
      "Sign",
      "message",
    });
UX_STEP_VALID(
    ux_sign_flow_4_step,
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
  &ux_sign_flow_4_step
);

//////////////////////////////////////////////////////////////////////

UX_STEP_NOCB(ux_confirm_full_flow_1_step,
    pnn, 
    {
      &C_icon_eye,
      "Review",
      "transaction",
    });
UX_STEP_NOCB(
    ux_confirm_full_flow_2_step,
    bnnn_paging, 
    {
      .title = "Amount",
      .text = vars.tmp.fullAmount
    });
UX_STEP_NOCB(
    ux_confirm_full_flow_3_step,
    bnnn_paging, 
    {
      .title = "Address",
      .text = vars.tmp.fullAddress,
    });
UX_STEP_NOCB(
    ux_confirm_full_flow_4_step,
    bnnn_paging, 
    {
      .title = "Fees",
      .text = vars.tmp.feesAmount,
    });
UX_STEP_VALID(
    ux_confirm_full_flow_5_step,
    pbb,
    io_seproxyhal_touch_verify_ok(NULL),
    {
      &C_icon_validate_14,
      "Accept",
      "and send",
    });
UX_STEP_VALID(
    ux_confirm_full_flow_6_step,
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
  &ux_confirm_full_flow_6_step
);

//////////////////////////////////////////////////////////////////////

UX_STEP_NOCB(
    ux_confirm_single_flow_1_step, 
    pnn, 
    {
      &C_icon_eye,
      "Review",
      "transaction"
    });
UX_STEP_NOCB(
    ux_confirm_single_flow_2_step, 
    bnnn_paging, 
    {
      .title = "Amount",
      .text = vars.tmp.fullAmount,
    });
UX_STEP_NOCB(
    ux_confirm_single_flow_3_step, 
    bnnn_paging, 
    {
      .title = "Address",
      .text = vars.tmp.fullAddress,
    });
UX_STEP_NOCB(
    ux_confirm_single_flow_4_step, 
    bnnn_paging, 
    {
      .title = "Fees",
      .text = vars.tmp.feesAmount,
    });
UX_STEP_VALID(
    ux_confirm_single_flow_5_step,
    pb,
    io_seproxyhal_touch_verify_ok(NULL), 
    {
      &C_icon_validate_14,
      "Accept",
    });
UX_STEP_VALID(
    ux_confirm_single_flow_6_step,
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
  &ux_confirm_single_flow_6_step
);

//////////////////////////////////////////////////////////////////////

UX_STEP_NOCB(
    ux_finalize_flow_1_step, 
    pnn, 
    {
      &C_icon_eye,
      "Review",
      "transaction"
    });
UX_STEP_NOCB(
    ux_finalize_flow_4_step,
    bnnn_paging, 
    {
      .title = "Fees",
      .text = vars.tmp.feesAmount,
    });
UX_STEP_VALID(
    ux_finalize_flow_5_step,
    pbb,
    io_seproxyhal_touch_verify_ok(NULL),
    {
      &C_icon_validate_14,
      "Accept",
      "and send"
    });
UX_STEP_VALID(
    ux_finalize_flow_6_step,
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
  &ux_finalize_flow_6_step
);

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(
    ux_request_change_path_approval_flow_1_step,
    pbb, 
    {
      &C_icon_eye,
      "The change path",
      "is unusual",
    });
UX_STEP_NOCB(
    ux_request_change_path_approval_flow_2_step,
    bnnn_paging, 
    {
      .title = "Change path",
      .text = vars.tmp_warning.derivation_path,
    });
UX_STEP_VALID(
    ux_request_change_path_approval_flow_3_step,
    pbb,
    io_seproxyhal_touch_display_cancel(NULL),
    {
      &C_icon_crossmark,
      "Reject if you're",
      "not sure",
    });
UX_STEP_VALID(
    ux_request_change_path_approval_flow_4_step,
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
  &ux_request_change_path_approval_flow_4_step
);

#endif // TARGET_NANOX || TARGET_NANOS2


unsigned int ui_tx_confirm_full_output() {
    if (!prepare_full_output(0)) {
        return 0;
    }

#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 4;
    UX_DISPLAY(ui_verify_nanos, ui_verify_prepro);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_confirm_full_flow, NULL);
#endif // TARGET_NANOX || TARGET_NANOS2
    return 1;
}

unsigned int ui_tx_confirm_single_output() {
// TODO : remove when supporting multi output
    if (!prepare_single_output()) {
        return 0;
    }

    snprintf(vars.tmp.feesAmount, sizeof(vars.tmp.feesAmount), "output #%d",
             btchip_context_D.totalOutputs - btchip_context_D.remainingOutputs +
                 1);

#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 3;
    UX_DISPLAY(ui_verify_output_nanos, ui_verify_output_prepro);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_confirm_single_flow, NULL);
#endif // TARGET_NANOX || TARGET_NANOS2
    return 1;
}

unsigned int ui_tx_finalize() {
    if (!prepare_fees()) {
        return 0;
    }

#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 2;
    UX_DISPLAY(ui_finalize_nanos, ui_finalize_prepro);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_finalize_flow, NULL);
#endif // TARGET_NANOX
    return 1;
}

void ui_tx_confirm_message_signature() {
    if (!prepare_message_signature()) {
        return;
    }

#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 2;
    UX_DISPLAY(ui_verify_message_signature_nanos, ui_verify_message_prepro);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_sign_flow, NULL);
#endif // TARGET_NANOX
}

void ui_tx_request_change_path_approval(unsigned char* change_path)
{
    bip32_print_path(change_path, vars.tmp_warning.derivation_path, MAX_DERIV_PATH_ASCII_LENGTH);
#if defined(TARGET_NANOS)
    // append and prepend a white space to the address
    ux_step = 0;
    ux_step_count = 4;
    UX_DISPLAY(ui_request_change_path_approval_nanos, ui_request_change_path_approval_nanos_prepro);
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    ux_flow_init(0, ux_request_change_path_approval_flow, NULL);
#endif // #if TARGET_ID
}

#endif