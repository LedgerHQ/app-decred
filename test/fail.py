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

from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
from binascii import hexlify
from time import sleep

dongle = getDongle(True)
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
'''
FAIL:

HID => 000000010100000002
HID <= 9000
HID => b85175d43ddb3916230953ecd19db78be5a5054cb86acf34283ae06e3ab7bd8200000000006b
HID <= 9000
HID => 483045022100f508399521615aa61999a8ad25a3fee56565f0f760476d2699b86c93350c821d022010a2262fc46111eeb961a910e138ae1cdf2993bf37afea10c1dd45f0fa24dbfa01210355d934ca47cf6b34a73e5bcf128ee3322164f0ce494eccece2a8dbe1dedace76ffffff00
HID <= 9000
HID => f8a59eef9e5dbdbfafd235276fe585366728a2e96e716e77a9431698683e1ac600000000006b
HID <= 9000
HID => 4830450221008f43f80bb9b37908d4bfdb27aa1c0fddea616a9c04862de541ad33eaffb766c9022010c5989bb665cd17257c375a1ac22e09978ab50fa334f8232d526f98b1d589ce012102c4a3667333cbbdaf0c16d13cf18af81a2faea52d1f855819115bd028680c1ad3ffffff00
HID <= 9000
HID => 02
HID <= 9000
HID => f9dd1a000000000000001976a914977c43a6d5c17f7ec34ef2108ec54b773f009c4d88ac
HID <= 9000
HID => 5f310e000000000000001976a914e1a5c4a61d89d11f8a2f3c4204ad5c58024df60188ac
HID <= 9000
HID => 0000000000000000
HID <= 320099322f0067c760720ed3ba067b36454533d1aaa5c3d7460b968b04139fc369fbd3f1010000005f310e00000000008ee7b08301316cda9000

#### Returned Tx ID should be : 1985a7ba6aba5cc3b5d42a36dfef01914670c6ffe3f1f8637f7ddf10cf25f565  
'''

################# GET TRUSTED INPUT #########################

packets = [
    "000000010100000002",  #input index (UTXO) (from 0, normal endian) + (begin tx streaming) version + number of inputs
    "b85175d43ddb3916230953ecd19db78be5a5054cb86acf34283ae06e3ab7bd8201000000006b",  #wrong endian txid + outpout index + tree + witness size (could be deleted for decred)
    "483045022100f508399521615aa61999a8ad25a3fee56565f0f760476d2699b86c93350c821d022010a2262fc46111eeb961a910e138ae1cdf2993bf37afea10c1dd45f0fa24dbfa01210355d934ca47cf6b34a73e5bcf128ee3322164f0ce494eccece2a8dbe1dedace76ffffff00",  #witness + sequence
    "f8a59eef9e5dbdbfafd235276fe585366728a2e96e716e77a9431698683e1ac600000000006b",  #wrong endian txid + outpout index + tree + witness size (could be deleted for decred)
    "4830450221008f43f80bb9b37908d4bfdb27aa1c0fddea616a9c04862de541ad33eaffb766c9022010c5989bb665cd17257c375a1ac22e09978ab50fa334f8232d526f98b1d589ce012102c4a3667333cbbdaf0c16d13cf18af81a2faea52d1f855819115bd028680c1ad3ffffff00",  #witness + sequence
    "02",  # outputs
    "f9dd1a000000000000001976a914977c43a6d5c17f7ec34ef2108ec54b773f009c4d88ac",  #amount + script version + script
    "5f310e000000000000001976a914e1a5c4a61d89d11f8a2f3c4204ad5c58024df60188ac",  #amount + script version + script
    "0000000000000000"  #locktime + expiry 
]

packets[0] = "e0420000" + hexlify(bytes([int(len(packets[0]) / 2)
                                         ])).decode("utf-8") + packets[0]
result = dongle.exchange(bytearray.fromhex(packets[0]))

for packet in packets[1:]:

    packet = "e0428000" + hexlify(bytes([int(len(packet) / 2)
                                         ])).decode("utf-8") + packet
    result = dongle.exchange(bytearray.fromhex(packet))

trusted_input = result

exit()
# [magic + 00 + rand(2) + input txid (LE) + input index + amount + HMAC]
#<= 32 00 7f62 334462e04608ca0441afe495cc5760c23914e553e0f0996c50095e39e13b1804 01000000 4054890000000000 cc76201c83268593 9000

expected = "334462e04608ca0441afe495cc5760c23914e553e0f0996c50095e39e13b1804010000004054890000000000"

if expected not in hexlify(result).decode("utf-8"):
    print("Error:\nExpected:%s\nGot:     %s\n" %
          (hexlify(result[4:-8]).decode("utf-8"), expected))
    exit()

################# HASH INPUT START #########################

packets = [
    "0100000001",  #version + number of input
    #"0138320006a640c65057afdd582f4f086c6e6e8c160092e4c0d32b9faa9fa91b8feb1048379c020000002ac503f20100000085bdec7eae8ace3a01",
    "01" + "%0.2X" % len(trusted_input) +
    hexlify(trusted_input).decode("utf-8") + "00" +
    "19",  # trusted input flag + [magic + 00 + rand(2) + input txid + input index + amount + HMAC] + tree + script len
    "76a914b0809bbfc9c10ed4d70a0efe932e589ca11239d188acffffffff"  # spend output script + sequence
]

packets[0] = "e0440000" + hexlify(bytes([int(len(packets[0]) / 2)
                                         ])).decode("utf-8") + packets[0]
result = dongle.exchange(bytearray.fromhex(packets[0]))

for packet in packets[1:]:

    packet = "e0448000" + hexlify(bytes([int(len(packet) / 2)
                                         ])).decode("utf-8") + packet
    result = dongle.exchange(bytearray.fromhex(packet))

################# HASH INPUT FINALIZE WITH CHANGE #########################

packets = [
    "058000002c8000002A800000000000000100000001",  # change address bip44 path
    "02ac211e000000000000001976a914fdeea9711e6c81027d677b2ceddf5c14d84977d288acc0cf6a000000000000001976a9149e882fd6fe9ff8da3f0309b15ff009f1e534719888ac"  #num output + amount + script version + new lock script + same for change addr
]

packets[0] = "e04aFF00" + hexlify(bytes([int(len(packets[0]) / 2)
                                         ])).decode("utf-8") + packets[0]
result = dongle.exchange(bytearray.fromhex(packets[0]))

for packet in packets[1:-1]:

    packet = "e04a0000" + hexlify(bytes([int(len(packet) / 2)
                                         ])).decode("utf-8") + packet
    result = dongle.exchange(bytearray.fromhex(packet))

packet = "e04a8000" + hexlify(bytes([int(len(packets[-1]) / 2)
                                     ])).decode("utf-8") + packets[-1]
result = dongle.exchange(bytearray.fromhex(packet))

################# HASH SIGN #########################

packets = [
    "058000002c8000002A800000000000000000000001000000000000000001"  #signing key path len + path + lock time + expiry + sighash type
]

packets[0] = "e0480000" + hexlify(bytes([int(len(packets[0]) / 2)
                                         ])).decode("utf-8") + packets[0]
result = dongle.exchange(bytearray.fromhex(packets[0]))

for packet in packets[1:]:

    packet = "e0480000" + hexlify(bytes([int(len(packet) / 2)
                                         ])).decode("utf-8") + packet
    result = dongle.exchange(bytearray.fromhex(packet))

# 31450221008a38ca160729e7c381d50126eec970b5c82cb8ba0625498dd414681710da250a0220409b1c2fe867261d1624ad695be464c8a4c37a9e931d8687183697302221c9ce 01 witness signature + sighash flag

expected = "31450221008a38ca160729e7c381d50126eec970b5c82cb8ba0625498dd414681710da250a0220409b1c2fe867261d1624ad695be464c8a4c37a9e931d8687183697302221c9ce01"

if expected not in hexlify(result).decode("utf-8"):
    print("Error:\nExpected:%s\nGot:     %s\n" %
          (hexlify(result).decode("utf-8"), expected))
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
