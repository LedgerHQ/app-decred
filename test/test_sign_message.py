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
def test_decred_sign_message_blind_signing_rejected(scenario_navigator: NavigateWithScenario):
    scenario_navigator.backend.raise_policy = RaisePolicy.RAISE_NOTHING

    message = "A" * 201
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
    result = scenario_navigator.backend.exchange_raw(data=bytearray.fromhex(packet))
    assert result.status == 0x6985
