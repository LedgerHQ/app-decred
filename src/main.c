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

#include "ui_main_menu.h"
#include "ui_pubkey.h"
#include "ui_shared.h"

#define __NAME3(a, b, c) a##b##c
#define NAME3(a, b, c) __NAME3(a, b, c)

#ifndef TARGET_FATSTACKS
bagl_element_t tmp_element;
#endif

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

#define BAGL_FONT_OPEN_SANS_LIGHT_16_22PX_AVG_WIDTH 10
#define BAGL_FONT_OPEN_SANS_REGULAR_10_13PX_AVG_WIDTH 8
#define MAX_CHAR_PER_LINE 25

#define COLOR_BG_1 0xF9F9F9
#define COLOR_APP COIN_COLOR_HDR      // bitcoin 0xFCB653
#define COLOR_APP_LIGHT COIN_COLOR_DB // bitcoin 0xFEDBA9
#define COLOR_BLACK 0x000000

#define COLOR_GRAY 0x999999
#define COLOR_LIGHT_GRAY 0xEEEEEE

#ifndef TARGET_FATSTACKS
unsigned int io_seproxyhal_touch_settings(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_exit(const bagl_element_t *e);
#endif
void ui_idle(void);

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2) || defined(TARGET_FATSTACKS)
#include "ux.h"
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;
#else
ux_state_t ux;
#endif // TARGET_NANOX || TARGET_NANOS2

#ifndef TARGET_FATSTACKS
const bagl_element_t *ui_menu_item_out_over(const bagl_element_t *e) {
    // the selection rectangle is after the none|touchable
    e = (const bagl_element_t *)(((unsigned int)e) + sizeof(bagl_element_t));
    return e;
}

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    if ((element->component.type & (~BAGL_TYPE_FLAGS_MASK)) != BAGL_NONE) {
        io_seproxyhal_display_default((bagl_element_t *)element);
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
            return 0; // nothing received from the master so far (it's a tx
                      // transaction)
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                          sizeof(G_io_apdu_buffer), 0);
        }

    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}

unsigned char io_event(unsigned char channel) {
    UNUSED(channel);
    // nothing done with the event, throw an error on the transport layer if
    // needed
    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_FINGER_EVENT:
        UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
        break;

#ifndef TARGET_FATSTACKS
    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
        UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
        break;
#endif

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
#ifndef TARGET_FATSTACKS
    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        UX_DISPLAYED_EVENT({});
        break;
#endif

    case SEPROXYHAL_TAG_TICKER_EVENT:
        UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
            // don't redisplay if UX not allowed (pin locked in the common bolos
            // ux ?)
            if (ux_step_count && UX_ALLOWED) {
                // prepare next screen
                if(!ux_loop_over_curr_element) {
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
#endif // COIN_FORKID
#ifdef COIN_FLAGS
    .flags = COIN_FLAGS,
#endif // COIN_FLAGS
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
    memmove(&coin_config, &C_coin_config, sizeof(coin_config));
    coin_config.coinid = coinid;
    coin_config.name = name;
    coin_config.name_short = name_short;

    BEGIN_TRY {
        TRY {
            // ensure syscall will accept us
            check_api_level(CX_COMPAT_APILEVEL);
            // delegate to bitcoin app/lib
            libcall_params[0] = "Decred";
            libcall_params[1] = 0x100; // use the Init call, as we won't exit
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
        if (((unsigned int *)arg0)[0] != 0x100) {
            app_exit();
            return 0;
        }
        // grab the coin config structure from the first parameter
        G_coin_config = (btchip_altcoin_config_t *)((unsigned int *)arg0)[1];
    } else {
        G_coin_config = (btchip_altcoin_config_t *)PIC(&C_coin_config);
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
#endif // TARGET_NANOX

                btchip_context_init();

                USB_power(0);
                USB_power(1);

                ui_idle();

#ifdef HAVE_BLE
                BLE_power(0, NULL);
                BLE_power(1, "Nano X");
#endif // HAVE_BLE

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
#endif // USE_LIB_DECRED
    return 0;
}
