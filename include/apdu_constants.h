/*******************************************************************************
*   Ledger Blue - Bitcoin Wallet
*   (c) 2016 Ledger
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

#ifndef APDU_CONSTANTS_H

#define APDU_CONSTANTS_H

#define CLA 0xE0

#define INS_GET_WALLET_PUBLIC_KEY 0x40
#define INS_GET_TRUSTED_INPUT 0x42
#define INS_HASH_INPUT_START 0x44
#define INS_HASH_INPUT_FINALIZE 0x46
#define INS_HASH_SIGN 0x48
#define INS_HASH_INPUT_FINALIZE_FULL 0x4A
#define INS_SIGN_MESSAGE 0x4E
#define INS_GET_PUBLIC_KEY 0xB2
#define INS_GET_FIRMWARE_VERSION 0xC4
#define INS_GET_COIN_VER 0x16


#define SW_PIN_REMAINING_ATTEMPTS 0x63C0
#define SW_INCORRECT_LENGTH 0x6700
#define SW_COMMAND_INCOMPATIBLE_FILE_STRUCTURE 0x6981
#define SW_SECURITY_STATUS_NOT_SATISFIED 0x6982
#define SW_CONDITIONS_OF_USE_NOT_SATISFIED 0x6985
#define SW_INCORRECT_DATA 0x6A80
#define SW_NOT_ENOUGH_MEMORY_SPACE 0x6A84
#define SW_REFERENCED_DATA_NOT_FOUND 0x6A88
#define SW_FILE_ALREADY_EXISTS 0x6A89
#define SW_INCORRECT_P1_P2 0x6B00
#define SW_INS_NOT_SUPPORTED 0x6D00
#define SW_CLA_NOT_SUPPORTED 0x6E00
#define SW_TECHNICAL_PROBLEM 0x6F00
#define SW_OK 0x9000
#define SW_MEMORY_PROBLEM 0x9240
#define SW_NO_EF_SELECTED 0x9400
#define SW_INVALID_OFFSET 0x9402
#define SW_FILE_NOT_FOUND 0x9404
#define SW_INCONSISTENT_FILE 0x9408
#define SW_ALGORITHM_NOT_SUPPORTED 0x9484
#define SW_INVALID_KCV 0x9485
#define SW_CODE_NOT_INITIALIZED 0x9802
#define SW_ACCESS_CONDITION_NOT_FULFILLED 0x9804
#define SW_CONTRADICTION_SECRET_CODE_STATUS 0x9808
#define SW_CONTRADICTION_INVALIDATION 0x9810
#define SW_CODE_BLOCKED 0x9840
#define SW_MAX_VALUE_REACHED 0x9850
#define SW_GP_AUTH_FAILED 0x6300
#define SW_LICENSING 0x6F42
#define SW_HALTED 0x6FAA
#define SW_APP_HALTED SW_CONDITIONS_OF_USE_NOT_SATISFIED

#define ISO_OFFSET_CLA 0x00
#define ISO_OFFSET_INS 0x01
#define ISO_OFFSET_P1 0x02
#define ISO_OFFSET_P2 0x03
#define ISO_OFFSET_LC 0x04
#define ISO_OFFSET_CDATA 0x05

#define NONE_2FA 0x00
#define KEYBOARD_2FA 0x01
#define SECURE_SCREEN_2FA 0x02

#define BITID_DERIVE 0xB11D
#define BITID_DERIVE_MULTIPLE 0xB11E

#include "os.h"
#include "secure_value.h"

void commit_operation_mode(secu8 operationMode);

unsigned short apdu_get_wallet_public_key(void);
unsigned short apdu_get_trusted_input(void);
unsigned short apdu_hash_input_start(void);
unsigned short apdu_hash_input_finalize(void);
unsigned short apdu_hash_sign(void);
unsigned short apdu_hash_input_finalize_full(void);
unsigned short apdu_get_public_key(void);
unsigned short apdu_sign_message(void);

unsigned short apdu_get_firmware_version(void);

unsigned short apdu_get_coin_version(void);

#endif
