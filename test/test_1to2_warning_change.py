#!/usr/bin/env python
#*******************************************************************************
#*   Ledger App
#*   (c) 2016-2019 Ledger
#*
#*  Licensed under the Apache License, Version 2.0 (the "License");
#*  you may not use this file except in compliance with the License.
#*  You may obtain a copy of the License at
#*
#*      http://www.apache.org/licenses/LICENSE-2.0
#*
#*  Unless required by applicable law or agreed to in writing, software
#*  distributed under the License is distributed on an "AS IS" BASIS,
#*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#*  See the License for the specific language governing permissions and
#*  limitations under the License.
#********************************************************************************
# import sys
# import struct
# from contextlib import contextmanager
# from pathlib import Path
from ragger.backend import RaisePolicy
from ragger.navigator import NavInsID, NavIns
from time import sleep
from pathlib import Path
from inspect import currentframe
from binascii import hexlify
from conftest import ROOT_SCREENSHOT_PATH

trusted_input = None


################# GET TRUSTED INPUT #########################
def test_1to2_warning_get_trusted_input(backend, firmware, navigator):
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    packets = [
        "000000010100000001",  #input index (UTXO) (from 0, normal endian) + (begin tx streaming) version + number of inputs
        "60fe7d21bfbd946b5bc96e7819b531d42961300ecf451d428d6ea866f02e98e901000000006b",  #wrong endian txid + outpout index + tree + witness size (could be deleted for decred)
        "4830450221009af49a50a71cfefb659ed4a61e8d5813622971a59187262c43a32c99a4e2daf00220344981e523f1e2bc19db62e3abaf346c7f1ae82d340438945183e1a23adb968c0121023bb5d9b33f895dddaadc173c1ccce15eb7800b0261775081e2d026f35d9249a5ffffffff",  #witness + sequence
        "02",  # outputs
        "069046800000000000001976a914beb827a5a42918e1def4fe9f635cc1aa3abdfd4f88ac",  #amount + script version + script
        "405489000000000000001976a914b0809bbfc9c10ed4d70a0efe932e589ca11239d188ac",  #amount + script version + script
        "0000000000000000"  #locktime + expiry 
    ]

    packets[0] = "e0420000" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]
    result = backend.exchange_raw(data=bytearray.fromhex(packets[0]))

    for packet in packets[1:]:
        packet = "e0428000" + hexlify(bytes([int(len(packet) / 2)
                                             ])).decode("utf-8") + packet
        result = backend.exchange_raw(data=bytearray.fromhex(packet))

    global trusted_input
    trusted_input = result

    # [magic + 00 + rand(2) + input txid (LE) + input index + amount + HMAC]
    #<= 32 00 7f62 334462e04608ca0441afe495cc5760c23914e553e0f0996c50095e39e13b1804 01000000 4054890000000000 cc76201c83268593 9000

    expected = "334462e04608ca0441afe495cc5760c23914e553e0f0996c50095e39e13b1804010000004054890000000000"

    if expected not in hexlify(result.data).decode("utf-8"):
        print("Error:\nExpected:%s\nGot:     %s\n" %
              (hexlify(result.data[4:-8]).decode("utf-8"), expected))
        exit()


################# HASH INPUT START #########################
def test_1to2_warning_hash_input_start(backend, firmware, navigator):
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    packets = [
        "0100000001",  #version + number of input
        #"0138320006a640c65057afdd582f4f086c6e6e8c160092e4c0d32b9faa9fa91b8feb1048379c020000002ac503f20100000085bdec7eae8ace3a01",
        "01" + "%0.2X" % len(trusted_input.data) +
        hexlify(trusted_input.data).decode("utf-8") + "00" +
        "19",  # trusted input flag + [magic + 00 + rand(2) + input txid + input index + amount + HMAC] + tree + script len
        "76a914b0809bbfc9c10ed4d70a0efe932e589ca11239d188acffffffff"  # spend output script + sequence
    ]

    packets[0] = "e0440000" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]
    result = backend.exchange_raw(data=bytearray.fromhex(packets[0]))

    for packet in packets[1:]:
        packet = "e0448000" + hexlify(bytes([int(len(packet) / 2)
                                             ])).decode("utf-8") + packet
        result = backend.exchange_raw(data=bytearray.fromhex(packet))


################# HASH INPUT FINALIZE WITH CHANGE #########################
def test_1to2_warning_finalize(backend, firmware, navigator):
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    packets = [
        "058000002c8000002A800000000000000110000001",  # change address bip44 path (very high index) (should update next line to be valid, this is just to display the warning)
        "02ac211e000000000000001976a914fdeea9711e6c81027d677b2ceddf5c14d84977d288acc0cf6a000000000000001976a9149e882fd6fe9ff8da3f0309b15ff009f1e534719888ac"  #num output + amount + script version + new lock script + same for change addr
    ]

    test_name = Path(currentframe().f_code.co_name)
    packets[0] = "e04aFF00" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]
    with backend.exchange_async_raw(data=bytearray.fromhex(packets[0])) as r:
        if firmware.device == "stax":
            navigator.navigate_and_compare(
                ROOT_SCREENSHOT_PATH,
                test_case_name=test_name,
                instructions=[NavInsID.USE_CASE_CHOICE_REJECT],
                screen_change_after_last_instruction=False)
        else:
            navigator.navigate_until_text_and_compare(
                NavInsID.RIGHT_CLICK, [NavInsID.BOTH_CLICK],
                "Approve",
                ROOT_SCREENSHOT_PATH,
                test_name,
                screen_change_after_last_instruction=False)

    for packet in packets[1:-1]:
        packet = "e04a0000" + hexlify(bytes([int(len(packet) / 2)
                                             ])).decode("utf-8") + packet
        result = backend.exchange_raw(data=bytearray.fromhex(packet))
        assert (result.status == 0x9000)

    packet = "e04a8000" + hexlify(bytes([int(len(packets[-1]) / 2)
                                         ])).decode("utf-8") + packets[-1]

    with backend.exchange_async_raw(data=bytearray.fromhex(packet)) as r:
        if firmware.device == "stax":
            snap_idx = 1
            for _ in range(3):
                instructions = [
                    NavIns(id=NavInsID.TAPPABLE_CENTER_TAP),
                    NavIns(id=NavInsID.TAPPABLE_CENTER_TAP),
                    NavIns(id=NavInsID.USE_CASE_REVIEW_CONFIRM)
                ]
                if _ == 2:
                    instructions.append(NavInsID.WAIT_FOR_HOME_SCREEN)

                navigator.navigate_and_compare(
                    ROOT_SCREENSHOT_PATH,
                    test_case_name=test_name,
                    instructions=instructions,
                    screen_change_after_last_instruction=False,
                    snap_start_idx=snap_idx)
                snap_idx += 3
        else:
            idx = 1
            for _ in range(3):
                test = Path(str(test_name) + f"_{idx}")
                navigator.navigate_until_text_and_compare(
                    NavInsID.RIGHT_CLICK, [NavInsID.BOTH_CLICK],
                    "Accept",
                    ROOT_SCREENSHOT_PATH,
                    test,
                    screen_change_after_last_instruction=False)
                idx += 1


################# HASH SIGN #########################
def test_1to2_warning_sign(backend, firmware, navigator):
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    packets = [
        "058000002c8000002A800000000000000000000001000000000000000001"  #signing key path len + path + lock time + expiry + sighash type
    ]

    packets[0] = "e0480000" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]
    result = backend.exchange_raw(data=bytearray.fromhex(packets[0]))

    assert (result.status == 0x9000)

    for packet in packets[1:]:

        packet = "e0480000" + hexlify(bytes([int(len(packet) / 2)
                                             ])).decode("utf-8") + packet
        result = backend.exchange_raw(data=bytearray.fromhex(packet))

        assert (result.status == 0x9000)

    # 31450221008a38ca160729e7c381d50126eec970b5c82cb8ba0625498dd414681710da250a0220409b1c2fe867261d1624ad695be464c8a4c37a9e931d8687183697302221c9ce 01 witness signature + sighash flag
    expected = "31450221008a38ca160729e7c381d50126eec970b5c82cb8ba0625498dd414681710da250a0220409b1c2fe867261d1624ad695be464c8a4c37a9e931d8687183697302221c9ce01"

    if expected not in hexlify(result.data).decode("utf-8"):
        print("Error:\nExpected:%s\nGot:     %s\n" %
              (hexlify(result.data).decode("utf-8"), expected))
        exit()


''' 
APDU EXCHANGE RECAP:

HID => e042000009000000010100000001
HID <= 9000
HID => e04280002660fe7d21bfbd946b5bc96e7819b531d42961300ecf451d428d6ea866f02e98e901000000006b
HID <= 9000
HID => e04280006f4830450221009af49a50a71cfefb659ed4a61e8d5813622971a59187262c43a32c99a4e2daf00220344981e523f1e2bc19db62e3abaf346c7f1ae82d340438945183e1a23adb968c0121023bb5d9b33f895dddaadc173c1ccce15eb7800b0261775081e2d026f35d9249a5ffffffff
HID <= 9000
HID => e04280000102
HID <= 9000
HID => e042800024069046800000000000001976a914beb827a5a42918e1def4fe9f635cc1aa3abdfd4f88ac
HID <= 9000
HID => e042800024405489000000000000001976a914b0809bbfc9c10ed4d70a0efe932e589ca11239d188ac
HID <= 9000
HID => e0428000080000000000000000
HID <= 32007f62334462e04608ca0441afe495cc5760c23914e553e0f0996c50095e39e13b1804010000004054890000000000cc76201c832685939000
HID => e0440000050100000001
HID <= 9000
HID => e04480003c013832007f62334462e04608ca0441afe495cc5760c23914e553e0f0996c50095e39e13b1804010000004054890000000000cc76201c832685930019
HID <= 9000
HID => e04480001d76a914b0809bbfc9c10ed4d70a0efe932e589ca11239d188acffffffff
HID <= 9000
HID => e04aff0015058000002c8000002a800000000000000100000001
HID <= 9000
HID => e04a80004902ac211e000000000000001976a914fdeea9711e6c81027d677b2ceddf5c14d84977d288acc0cf6a000000000000001976a9149e882fd6fe9ff8da3f0309b15ff009f1e534719888ac
HID <= 00009000
HID => e04800001e058000002c8000002a800000000000000000000001000000000000000001
HID <= 31450221008a38ca160729e7c381d50126eec970b5c82cb8ba0625498dd414681710da250a0220409b1c2fe867261d1624ad695be464c8a4c37a9e931d8687183697302221c9ce019000

'''
