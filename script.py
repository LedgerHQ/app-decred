#!/usr/bin/env python
#*******************************************************************************
#*   Ledger Blue
#*   (c) 2016 Ledger
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
textToSign = ""


dongle = getDongle(True)


'''

################# GET PUBKEY #########################

packets = [
"058000002c80000001800000000000000000000001"
]


packets[0] = "e0400100" + hexlify(bytes([int(len(packets[0])/2)])).decode("utf-8") + packets[0] 
result = dongle.exchange(bytearray.fromhex(packets[0]))


'''
################# GET TRUSTED INPUT #########################


packets = [
"000000010100000001",
"b8e584d35a37e3ebe8cef426da518fe028d7b83ec7745404dd9ade9e7e6760df01000000006b",
"4830450221008e63cadcbc42147d7e5051fbd7a45a2a6af3911a49198717ff6d2ce31fbd2e3e022065cc04bb30df1591e421b0b5bc2f2f69739882161f39a90af3e69a27f04ed9d901210243e17f0dc1ada2ceb8caaa44ad0fe5a2632699b612608f2ff9932d84679dc56affffffff",
"02",
"80f0fa020000000000001976a914fe64e574c5ae9f06904c52b7ff59947643a1f47c88ac",
"40c7e0080000000000001976a914b0cb136330c40870bef02c743ff134b06ad5a18b88ac",
"0000000000000000"
]


packets[0] = "e0420000" + hexlify(bytes([int(len(packets[0])/2)])).decode("utf-8") + packets[0] 
result = dongle.exchange(bytearray.fromhex(packets[0]))

for packet in packets[1:]:

	packet = "e0428000" + hexlify(bytes([int(len(packet)/2)])).decode("utf-8") + packet 
	result = dongle.exchange(bytearray.fromhex(packet))

trusted_input = result

'''
################# HASH INPUT START #########################

packets = [
"0100000001",
#"0138320006a640c65057afdd582f4f086c6e6e8c160092e4c0d32b9faa9fa91b8feb1048379c020000002ac503f20100000085bdec7eae8ace3a01",
"01" + "%0.2X" % len(trusted_input) + hexlify(trusted_input).decode("utf-8") + "01" + "",
"ffffffff"
]


packets[0] = "e0440000" + hexlify(bytes([int(len(packets[0])/2)])).decode("utf-8") + packets[0] 
result = dongle.exchange(bytearray.fromhex(packets[0]))

for packet in packets[1:]:

	packet = "e0448000" + hexlify(bytes([int(len(packet)/2)])).decode("utf-8") + packet 
	result = dongle.exchange(bytearray.fromhex(packet))



################# HASH INPUT FINALIZE WITHOUT CHANGE #########################

packets = [
"01c4ba3a5e0100000000001976a914d483aea01347a133fd0de34dc10908d7a6c3b43788ac"
]


packets[0] = "e04a8000" + hexlify(bytes([int(len(packets[0])/2)])).decode("utf-8") + packets[0] 
result = dongle.exchange(bytearray.fromhex(packets[0]))

for packet in packets[1:]:

	packet = "e0448000" + hexlify(bytes([int(len(packet)/2)])).decode("utf-8") + packet 
	result = dongle.exchange(bytearray.fromhex(packet))






'''