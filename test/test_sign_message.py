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
from ragger.backend import RaisePolicy
from ragger.navigator import NavInsID, NavIns
from binascii import hexlify
import struct

################# SIGN MESSAGE #########################
def test_decred_sign_message(client, firmware, navigator):
    client.raise_policy = RaisePolicy.RAISE_NOTHING

    magic = "\x18Bitcoin Signed Message:\n"
    message = "Message to be signed"
    data = magic + chr(len(message)) + message
    hex_str_data = hexlify(bytes(data,'ascii' ))
    hex_str_sign_msg = b'058000002c8000002A800000000000000100000001' +  hexlify(struct.pack('>h', int(len(hex_str_data.decode('ascii'))/2))) + hex_str_data
    str_sign_msg = hex_str_sign_msg.decode('ascii')

    packet = "e04e0001" + hexlify(bytes([int(len(str_sign_msg)/2)])).decode("utf-8") + str_sign_msg
    result = client.exchange_raw(data=bytearray.fromhex(packet))

    packet = "e04e80000100"
    with client.exchange_async_raw(data=bytearray.fromhex(packet)) as r:
        if firmware.device == "fat":
            navigator.navigate([NavIns(id=NavInsID.TAPPABLE_CENTER_TAP)])
            navigator.navigate([NavIns(id=NavInsID.TAPPABLE_CENTER_TAP)])
            navigator.navigate([NavIns(id=NavInsID.USE_CASE_REVIEW_CONFIRM)])
        if firmware.device in ["nanox","nanosp"]:
            navigator.navigate([NavIns(id=NavInsID.RIGHT_CLICK)])
            navigator.navigate([NavIns(id=NavInsID.RIGHT_CLICK)])
            navigator.navigate([NavIns(id=NavInsID.BOTH_CLICK)])
        if firmware.device == "nanos":
            navigator.navigate([NavIns(id=NavInsID.RIGHT_CLICK)])
            
    # expected = "9e18e3a7e7508bdd151104b4879b350565aac97f031ee6eea5b7bf84029a929d00000000ac211e0000000000"
    # # expected = "7dfed275d5fb94c2b7fd0a0e92f0b62e8324829c40dba2b09231b7763ab43c5700000000ac211e0000000000"
    # if expected not in hexlify(result.data).decode("utf-8"):
    #     print("Error:\nExpected:%s\nGot:     %s\n" % (expected,hexlify(result.data[4:-8]).decode("utf-8")))
    #     exit()
