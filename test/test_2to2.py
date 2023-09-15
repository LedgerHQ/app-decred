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
from binascii import hexlify
from pathlib import Path
from inspect import currentframe
from ragger.navigator import NavInsID, NavIns
from conftest import ROOT_SCREENSHOT_PATH

trusted_input_1 = None
trusted_input_2 = None
'''

################# GET PUBKEY #########################

packets = [
"058000002c80000001800000000000000000000001" # BIP32 path len, BIP32 path
]


packets[0] = "e0400100" + hexlify(bytes([int(len(packets[0])/2)])).decode("utf-8") + packets[0] 
result = dongle.exchange(bytearray.fromhex(packets[0]))

# 41 04589ae7c835ce76e23cf8feb32f1adf4a7f2ba0ed2ad70801802b0bcd70e99c1c2c03b4c945b672f5d9dc0e5f9cce42afb893299dbf0fce6f02e8f3de580ac5bf pub key
# 23 5473636f46366d566741416b664e78776e716f5a553936654e3534355247594c376135 addr base58
# c191668478d204284390538897117f8c66ef8dafd2f3e67c0d83ce4fe4f09e53  chaincode

'''


################# GET TRUSTED INPUT 1 #########################
def test_2to2_get_trusted_input_1(backend):
    packets = [
        "000000000100000001",  #input index (UTXO) (from 0, normal endian) + (begin tx streaming) version + number of inputs
        "334462e04608ca0441afe495cc5760c23914e553e0f0996c50095e39e13b1804010000000000",  #wrong endian txid + outpout index + tree + witness size (0 for decred)
        "ffffffff",  #witness (0 in decred) + sequence
        "02",  # outputs
        "ac211e000000000000001976a914fdeea9711e6c81027d677b2ceddf5c14d84977d288ac",  #amount + script version + script
        "c0cf6a000000000000001976a9149e882fd6fe9ff8da3f0309b15ff009f1e534719888ac",  #amount + script version + script
        "0000000000000000"  #locktime + expiry 
    ]

    packets[0] = "e0420000" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]
    result = backend.exchange_raw(data=bytearray.fromhex(packets[0]))

    for packet in packets[1:]:
        packet = "e0428000" + hexlify(bytes([int(len(packet) / 2)
                                             ])).decode("utf-8") + packet
        result = backend.exchange_raw(data=bytearray.fromhex(packet))
    global trusted_input_1
    trusted_input_1 = result

    # [magic + 00 + rand(2) + input txid (LE) + input index + amount + HMAC]
    #<= 32 00 16c5 9e18e3a7e7508bdd151104b4879b350565aac97f031ee6eea5b7bf84029a929d 00000000 ac211e0000000000 598355bee9a1e576 9000

    expected = "9e18e3a7e7508bdd151104b4879b350565aac97f031ee6eea5b7bf84029a929d00000000ac211e0000000000"
    if expected not in hexlify(result.data).decode("utf-8"):
        print("Error:\nExpected:%s\nGot:     %s\n" %
              (expected, hexlify(result.data[4:-8]).decode("utf-8")))
        exit()


################# GET TRUSTED INPUT 2 #########################
def test_2to2_get_trusted_input_2(backend):
    packets = [
        "000000010100000001",  #input index (UTXO) (from 0, normal endian) + (begin tx streaming) version + number of inputs
        "334462e04608ca0441afe495cc5760c23914e553e0f0996c50095e39e13b1804010000000000",  #wrong endian txid + outpout index + tree + witness size (0 for decred)
        "ffffffff",  #witness (0 in decred) + sequence
        "02",  # outputs
        "ac211e000000000000001976a914fdeea9711e6c81027d677b2ceddf5c14d84977d288ac",  #amount + script version + script
        "c0cf6a000000000000001976a9149e882fd6fe9ff8da3f0309b15ff009f1e534719888ac",  #amount + script version + script
        "0000000000000000"  #locktime + expiry 
    ]

    packets[0] = "e0420000" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]
    result = backend.exchange_raw(data=bytearray.fromhex(packets[0]))

    for packet in packets[1:]:
        packet = "e0428000" + hexlify(bytes([int(len(packet) / 2)
                                             ])).decode("utf-8") + packet
        result = backend.exchange_raw(data=bytearray.fromhex(packet))

    global trusted_input_2
    trusted_input_2 = result

    # [magic + 00 + rand(2) + input txid (LE) + input index + amount + HMAC]
    #<= 32 00 dcac 9e18e3a7e7508bdd151104b4879b350565aac97f031ee6eea5b7bf84029a929d 01000000 c0cf6a0000000000 f0d368a53f42bdcd 9000

    expected = "9e18e3a7e7508bdd151104b4879b350565aac97f031ee6eea5b7bf84029a929d01000000c0cf6a0000000000"

    if expected not in hexlify(result.data).decode("utf-8"):
        print("Error:\nExpected:%s\nGot:     %s\n" %
              (expected, hexlify(result.data[4:-8]).decode("utf-8")))
        exit()


################# HASH INPUT START #########################
def test_2to2_input_start_1(backend):
    packets = [
        "0100000002",  #version + number of input
        "01" + "%0.2X" % len(trusted_input_1.data) +
        hexlify(trusted_input_1.data).decode("utf-8") + "00" +
        "19",  # trusted input flag + [magic + 00 + rand(2) + input txid + input index + amount + HMAC] + tree + script len
        "76a914fdeea9711e6c81027d677b2ceddf5c14d84977d288acffffffff",  # spend output script + sequence
        "01" + "%0.2X" % len(trusted_input_2.data) +
        hexlify(trusted_input_2.data).decode("utf-8") + "00" +
        "00",  # trusted input flag + [magic + 00 + rand(2) + input txid + input index + amount + HMAC] + tree + script len
        "ffffffff"  # spend output script + sequence
    ]

    packets[0] = "e0440000" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]
    result = backend.exchange_raw(data=bytearray.fromhex(packets[0]))

    for packet in packets[1:]:
        packet = "e0448000" + hexlify(bytes([int(len(packet) / 2)
                                             ])).decode("utf-8") + packet
        result = backend.exchange_raw(data=bytearray.fromhex(packet))


################# HASH INPUT FINALIZE WITH CHANGE #########################
def test_2to2_finalize_1(backend, firmware, navigator):
    packets = [
        "058000002c8000002A800000000000000100000002",  # change address bip44 path (size + path)
        # "02c03b0e000000000000001976a914a6b939449096f2595113b659e55df41bbd236b5e88ac00127a000000000000001976a91498d35df43b654993f16e3f9979678b0eb941ea8d88ac", #num output + amount + script version + new lock script + same for change addr
        "02c03b0e000000000000001976a914a6b939449096f2595113b659e55df41bbd236b5e88ac00127a000000000000001976a91498d35df43b654993f16e3f9979678b0eb941ea8d88ac",
        # "00127a000000000000001976a91498d35df43b654993f16e3f9979678b0eb941ea8d88ac" #num output + amount + script version + new lock script + same for change addr
    ]

    packets[0] = "e04aFF00" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]
    result = backend.exchange_raw(data=bytearray.fromhex(packets[0]))

    for packet in packets[1:-1]:
        packet = "e04a0000" + hexlify(bytes([int(len(packet) / 2)
                                             ])).decode("utf-8") + packet
        result = backend.exchange_raw(data=bytearray.fromhex(packet))

    packet = "e04a8000" + hexlify(bytes([int(len(packets[-1]) / 2)
                                         ])).decode("utf-8") + packets[-1]

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


################# HASH SIGN N°1 #########################
def test_2to2_sign_1(backend):
    packets = [
        "058000002c8000002A800000000000000100000001000000000000000001"  #signing key path len + path + lock time + expiry + sighash type
    ]

    packets[0] = "e0480000" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]
    result = backend.exchange_raw(data=bytearray.fromhex(packets[0]))

    for packet in packets[1:]:
        packet = "e0480000" + hexlify(bytes([int(len(packet) / 2)
                                             ])).decode("utf-8") + packet
        result = backend.exchange_raw(data=bytearray.fromhex(packet))

    expected = "3045022100ce8f37f615e60bd604b5c2e78a64068e0fc00a2fd06932060f27bc1e804ba90b02204f72ae4161f8935504d04242a5841b45e8f2776c22655aa3ac7f430f196af03801"

    if expected not in hexlify(result.data).decode("utf-8"):
        print("Error:\nExpected:%s\nGot:     %s\n" %
              (expected, hexlify(result.data).decode("utf-8")))
        exit()


################# 					 #########################
#################    WITNESS N°2     #########################
################# 					 #########################


################# HASH INPUT START N°2 #########################
def test_2to2_input_start_2(backend):
    packets = [
        "0100000002",  #version + number of input
        "01" + "%0.2X" % len(trusted_input_1.data) +
        hexlify(trusted_input_1.data).decode("utf-8") + "00" +
        "00",  # trusted input flag + [magic + 00 + rand(2) + input txid + input index + amount + HMAC] + tree + script len
        "ffffffff",  # spend output script + sequence
        "01" + "%0.2X" % len(trusted_input_2.data) +
        hexlify(trusted_input_2.data).decode("utf-8") + "00" +
        "19",  # trusted input flag + [magic + 00 + rand(2) + input txid + input index + amount + HMAC] + tree + script len
        "76a9149e882fd6fe9ff8da3f0309b15ff009f1e534719888acffffffff"  # spend output script + sequence
    ]

    packets[0] = "e0440080" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]
    result = backend.exchange_raw(data=bytearray.fromhex(packets[0]))

    for packet in packets[1:]:

        packet = "e0448080" + hexlify(bytes([int(len(packet) / 2)
                                             ])).decode("utf-8") + packet
        result = backend.exchange_raw(data=bytearray.fromhex(packet))


################# HASH INPUT FINALIZE WITH CHANGE N°2 #########################
def test_2to2_finalize_2(backend):
    packets = [
        # "058000002c8000002A800000000000000100000002", # change address bip44 path (size + path)
        "02c03b0e000000000000001976a914a6b939449096f2595113b659e55df41bbd236b5e88ac00127a000000000000001976a91498d35df43b654993f16e3f9979678b0eb941ea8d88ac"  #num output + amount + script version + new lock script + same for change addr
    ]

    # packets[0] = "e04aFF00" + hexlify(bytes([int(len(packets[0])/2)])).decode("utf-8") + packets[0]

    # unused in this case, but useful when packet is splitted in smaller ones
    for packet in packets[1:-1]:
        packet = "e04a0000" + hexlify(bytes([int(len(packet) / 2)
                                             ])).decode("utf-8") + packet
        result = backend.exchange_raw(data=bytearray.fromhex(packet))

    packet = "e04a8000" + hexlify(bytes([int(len(packets[-1]) / 2)
                                         ])).decode("utf-8") + packets[-1]
    result = backend.exchange_raw(data=bytearray.fromhex(packet))


################# HASH SIGN N°2 #########################
def test_2to2_sign_2(backend):
    packets = [
        "058000002c8000002A800000000000000000000002000000000000000001"  #signing key path len + path + lock time + expiry + sighash type
    ]

    packets[0] = "e0480000" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]
    result = backend.exchange_raw(data=bytearray.fromhex(packets[0]))

    for packet in packets[1:]:
        packet = "e0480000" + hexlify(bytes([int(len(packet) / 2)
                                             ])).decode("utf-8") + packet
        result = backend.exchange_raw(data=bytearray.fromhex(packet))

    expected = "3144022041b371311dc2f2dc72b83e8249d3fc0f53f6bfc9ccdb214eeea7b35914ba187602200de4313f2dca0aa2ca857afb65e06d693128e6e4b9b127eff018ddcfe98c462e01"

    if expected not in hexlify(result.data).decode("utf-8"):
        print("Error:\nExpected:%s\nGot:     %s\n" %
              (expected, hexlify(result.data).decode("utf-8")))
        exit()


''' 
APDU EXCHANGE RECAP:

HID => e042000009000000000100000001
HID <= 9000
HID => e042800026334462e04608ca0441afe495cc5760c23914e553e0f0996c50095e39e13b180401000000006b
HID <= 9000
HID => e04280006f4830450221008a38ca160729e7c381d50126eec970b5c82cb8ba0625498dd414681710da250a0220409b1c2fe867261d1624ad695be464c8a4c37a9e931d8687183697302221c9ce01210306c8988cab4694d1969c77ee5253e6ed5de7368f3f7713e3d5c512b8fd312562ffffffff
HID <= 9000
HID => e04280000102
HID <= 9000
HID => e042800024ac211e000000000000001976a914fdeea9711e6c81027d677b2ceddf5c14d84977d288ac
HID <= 9000
HID => e042800024c0cf6a000000000000001976a9149e882fd6fe9ff8da3f0309b15ff009f1e534719888ac
HID <= 9000
HID => e0428000080000000000000000
HID <= 320016c59e18e3a7e7508bdd151104b4879b350565aac97f031ee6eea5b7bf84029a929d00000000ac211e0000000000598355bee9a1e5769000
HID => e042000009000000010100000001
HID <= 9000
HID => e042800026334462e04608ca0441afe495cc5760c23914e553e0f0996c50095e39e13b180401000000006b
HID <= 9000
HID => e04280006f4830450221008a38ca160729e7c381d50126eec970b5c82cb8ba0625498dd414681710da250a0220409b1c2fe867261d1624ad695be464c8a4c37a9e931d8687183697302221c9ce01210306c8988cab4694d1969c77ee5253e6ed5de7368f3f7713e3d5c512b8fd312562ffffffff
HID <= 9000
HID => e04280000102
HID <= 9000
HID => e042800024ac211e000000000000001976a914fdeea9711e6c81027d677b2ceddf5c14d84977d288ac
HID <= 9000
HID => e042800024c0cf6a000000000000001976a9149e882fd6fe9ff8da3f0309b15ff009f1e534719888ac
HID <= 9000
HID => e0428000080000000000000000
HID <= 3200dcac9e18e3a7e7508bdd151104b4879b350565aac97f031ee6eea5b7bf84029a929d01000000c0cf6a0000000000f0d368a53f42bdcd9000
HID => e0440000050100000002
HID <= 9000
HID => e04480003c0138320016c59e18e3a7e7508bdd151104b4879b350565aac97f031ee6eea5b7bf84029a929d00000000ac211e0000000000598355bee9a1e5760019
HID <= 9000
HID => e04480001d76a914fdeea9711e6c81027d677b2ceddf5c14d84977d288acffffffff
HID <= 9000
HID => e04480003c01383200dcac9e18e3a7e7508bdd151104b4879b350565aac97f031ee6eea5b7bf84029a929d01000000c0cf6a0000000000f0d368a53f42bdcd0000
HID <= 9000
HID => e044800004ffffffff
HID <= 9000
HID => e04aff0015058000002c8000002a800000000000000100000002
HID <= 9000
HID => e04a80004902c03b0e000000000000001976a914a6b939449096f2595113b659e55df41bbd236b5e88ac00127a000000000000001976a91498d35df43b654993f16e3f9979678b0eb941ea8d88ac
HID <= 00009000
HID => e04800001e058000002c8000002a800000000000000100000001000000000000000001
HID <= 3045022100ce8f37f615e60bd604b5c2e78a64068e0fc00a2fd06932060f27bc1e804ba90b02204f72ae4161f8935504d04242a5841b45e8f2776c22655aa3ac7f430f196af038019000
HID => e0440080050100000002
HID <= 9000
HID => e04480803c0138320016c59e18e3a7e7508bdd151104b4879b350565aac97f031ee6eea5b7bf84029a929d00000000ac211e0000000000598355bee9a1e5760000
HID <= 9000
HID => e044808004ffffffff
HID <= 9000
HID => e04480803c01383200dcac9e18e3a7e7508bdd151104b4879b350565aac97f031ee6eea5b7bf84029a929d01000000c0cf6a0000000000f0d368a53f42bdcd0019
HID <= 9000
HID => e04480801d76a9149e882fd6fe9ff8da3f0309b15ff009f1e534719888acffffffff
HID <= 9000
HID => e04a80004902c03b0e000000000000001976a914a6b939449096f2595113b659e55df41bbd236b5e88ac00127a000000000000001976a91498d35df43b654993f16e3f9979678b0eb941ea8d88ac
HID <= 00009000
HID => e04800001e058000002c8000002a800000000000000000000002000000000000000001
HID <= 3144022041b371311dc2f2dc72b83e8249d3fc0f53f6bfc9ccdb214eeea7b35914ba187602200de4313f2dca0aa2ca857afb65e06d693128e6e4b9b127eff018ddcfe98c462e019000
'''
