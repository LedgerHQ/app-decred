#!/usr/bin/env python
# *******************************************************************************
# *   Ledger App
# *   (c) 2016-2019 Ledger
# *
# *  Licensed under the Apache License, Version 2.0 (the "License");
# *  you may not use this file except in compliance with the License.
# *  You may obtain a copy of the License at
# *
# *      http://www.apache.org/licenses/LICENSE-2.0
# *
# *  Unless required by applicable law or agreed to in writing, software
# *  distributed under the License is distributed on an "AS IS" BASIS,
# *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# *  See the License for the specific language governing permissions and
# *  limitations under the License.
# ********************************************************************************
from ragger.backend import RaisePolicy
from ragger.navigator import NavInsID
from ragger.navigator.navigation_scenario import NavigateWithScenario
import struct
from pathlib import Path
from inspect import currentframe
from binascii import hexlify
from conftest import ROOT_SCREENSHOT_PATH

# 231 printable-ASCII characters: over MAX_DISPLAYABLE_MESSAGE_LENGTH (200), so this
# always exercises the hash-fallback / blind-signing-gated path, not clear-signing.
LOREM_IPSUM = (
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, "
    "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat."
)

# Screen-position taps for the NBGL "Blind signing" switch: (settings gear, switch, back arrow).
_NBGL_BLIND_SIGNING_POSITIONS = {
    "stax": ((342, 55), (345, 137), (36, 36)),
    "flex": ((405, 75), (420, 142), (45, 45)),
    "apex_p": ((256, 43), (255, 75), (30, 30)),
}


def _set_blind_signing_bagl(backend, enable: bool):
    backend.right_click()  # idle -> "Settings"
    backend.both_click()  # enter settings submenu
    backend.right_click()  # move to "Blind signing"
    backend.both_click()  # enter blind-signing submenu (current state preselected)
    if enable:
        backend.left_click()  # move up to "Enabled"
    else:
        backend.right_click()  # move down to "Disabled"
    backend.both_click()  # confirm, back to idle


def _toggle_blind_signing_nbgl(backend, device_name: str):
    # The switch position only flips the current state - there is no separate
    # "Enabled"/"Disabled" choice to tap on NBGL, unlike the BAGL submenu.
    settings_pos, switch_pos, exit_pos = _NBGL_BLIND_SIGNING_POSITIONS[device_name]
    backend.finger_touch(*settings_pos)
    backend.finger_touch(*switch_pos)
    backend.finger_touch(*exit_pos)


def _sign_message_prepare_packet(message: str) -> str:
    hex_str_data = hexlify(bytes(message, "ascii"))
    hex_str_sign_msg = (
        b"058000002c8000002A800000000000000100000001"
        + hexlify(struct.pack(">h", len(message)))
        + hex_str_data
    )
    str_sign_msg = hex_str_sign_msg.decode("ascii")
    return (
        "e04e0001"
        + hexlify(bytes([int(len(str_sign_msg) / 2)])).decode("utf-8")
        + str_sign_msg
    )


################# SIGN MESSAGE #########################
def test_decred_sign_message(scenario_navigator: NavigateWithScenario):
    scenario_navigator.backend.raise_policy = RaisePolicy.RAISE_NOTHING

    # magic = "\x18 Signed Message:\n"
    message = "Message to be signed"
    hex_str_data = hexlify(bytes(message, "ascii"))
    hex_str_sign_msg = (
        b"058000002c8000002A800000000000000100000001"
        + hexlify(struct.pack(">h", len(message)))
        + hex_str_data
    )
    str_sign_msg = hex_str_sign_msg.decode("ascii")

    packet = (
        "e04e0001"
        + hexlify(bytes([int(len(str_sign_msg) / 2)])).decode("utf-8")
        + str_sign_msg
    )
    scenario_navigator.backend.exchange_raw(data=bytearray.fromhex(packet))

    packet = "e04e80000100"
    path = Path(currentframe().f_code.co_name)
    with scenario_navigator.backend.exchange_async_raw(data=bytearray.fromhex(packet)):
        if scenario_navigator.device.is_nano:
            scenario_navigator.navigator.navigate_until_text_and_compare(
                NavInsID.RIGHT_CLICK,
                [NavInsID.BOTH_CLICK],
                "Accept",
                ROOT_SCREENSHOT_PATH,
                path,
            )
        else:
            scenario_navigator.review_approve()

    result = scenario_navigator.backend.last_async_response

    expected = "3145022100fa1413642c251a1030c2f00304e41f80e7617f993acf233ce48ccb7ab6be51ba02201052f852488af7bab0eef9bb331c1255a5307de7576482fd4a97fa791bb19bfe"
    assert result.status == 0x9000
    if expected not in hexlify(result.data).decode("utf-8"):
        print(
            "Error:\nExpected:%s\nGot:%s\n"
            % (expected, hexlify(result.data).decode("utf-8"))
        )
        exit()


# Message longer than MAX_DISPLAYABLE_MESSAGE_LENGTH (200) can't be shown in clear, and
# blind signing is disabled by default, so the device must reject it with no UI shown.
def test_decred_sign_message_blind_signing_rejected(
    scenario_navigator: NavigateWithScenario,
):
    scenario_navigator.backend.raise_policy = RaisePolicy.RAISE_NOTHING

    packet = _sign_message_prepare_packet(LOREM_IPSUM)
    scenario_navigator.backend.exchange_raw(data=bytearray.fromhex(packet))

    packet = "e04e80000100"
    result = scenario_navigator.backend.exchange_raw(data=bytearray.fromhex(packet))
    assert result.status == 0x6985


################# SIGN MESSAGE - LONG MESSAGE, BLIND SIGNING ENABLED #########################
def test_decred_sign_message_blind_signing_enabled(
    scenario_navigator: NavigateWithScenario,
):
    scenario_navigator.backend.raise_policy = RaisePolicy.RAISE_NOTHING
    backend = scenario_navigator.backend
    device = scenario_navigator.device

    if device.is_nano:
        _set_blind_signing_bagl(backend, enable=True)
    else:
        _toggle_blind_signing_nbgl(backend, device.name)

    try:
        packet = _sign_message_prepare_packet(LOREM_IPSUM)
        scenario_navigator.backend.exchange_raw(data=bytearray.fromhex(packet))

        packet = "e04e80000100"
        path = Path(currentframe().f_code.co_name)
        with backend.exchange_async_raw(data=bytearray.fromhex(packet)):
            if device.is_nano:
                scenario_navigator.navigator.navigate_until_text_and_compare(
                    NavInsID.RIGHT_CLICK,
                    [NavInsID.BOTH_CLICK],
                    "Accept",
                    ROOT_SCREENSHOT_PATH,
                    path,
                )
            else:
                scenario_navigator.review_approve()

        result = backend.last_async_response
        assert result.status == 0x9000
    finally:
        if device.is_nano:
            _set_blind_signing_bagl(backend, enable=False)
        else:
            _toggle_blind_signing_nbgl(backend, device.name)
