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
import struct
from pathlib import Path
from inspect import currentframe
from binascii import hexlify
from ragger.backend import RaisePolicy
from ragger.navigator import NavInsID
from conftest import ROOT_SCREENSHOT_PATH


################# SIGN MESSAGE #########################
def test_decred_sign_message(backend, firmware, navigator):
    backend.raise_policy = RaisePolicy.RAISE_NOTHING

    # magic = "\x18 Signed Message:\n"
    message = "Message to be signed"
    data = chr(len(message)) + message
    hex_str_data = hexlify(bytes(data, 'ascii'))
    hex_str_sign_msg = b'058000002c8000002A800000000000000100000001' + hexlify(
        struct.pack('>h', int(
            len(hex_str_data.decode('ascii')) / 2))) + hex_str_data
    str_sign_msg = hex_str_sign_msg.decode('ascii')

    packet = "e04e0001" + hexlify(bytes([int(len(str_sign_msg) / 2)
                                         ])).decode("utf-8") + str_sign_msg
    backend.exchange_raw(data=bytearray.fromhex(packet))

    packet = "e04e80000100"
    path = Path(currentframe().f_code.co_name)
    with backend.exchange_async_raw(data=bytearray.fromhex(packet)) as r:
        if firmware.device == "stax":
            navigator.navigate_until_text_and_compare(
                NavInsID.TAPPABLE_CENTER_TAP, [
                    NavInsID.USE_CASE_REVIEW_CONFIRM,
                    NavInsID.WAIT_FOR_HOME_SCREEN
                ], "Hold", ROOT_SCREENSHOT_PATH, path)
        else:
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Accept",
                                                      ROOT_SCREENSHOT_PATH,
                                                      path)

    result = backend.last_async_response

    expected = "3045022100e841839bef7147f0bb79e7e301a0bfdb2ff8b1c4a195d9a18c4f167c70dd63e6022061898f34c11561ccd1ad74d627195aa5f08d7fff8f78eb9de605e433d8532783"
    assert result.status == 0x9000
    if expected not in hexlify(result.data).decode("utf-8"):
        print("Error:\nExpected:%s\nGot:%s\n" %
              (expected, hexlify(result.data).decode("utf-8")))
        exit()
