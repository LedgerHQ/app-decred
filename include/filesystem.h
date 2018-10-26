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

#ifndef FS_H

#define FS_H

#include "os.h"
#include "config.h"
#include "context.h"
#include "filesystem_tx.h"

enum supported_modes_e {
    SUPPORTED_MODE_WALLET = 0x01,
    SUPPORTED_MODE_RELAXED_WALLET = 0x02,
    SUPPORTED_MODE_SERVER = 0x04,
    SUPPORTED_MODE_DEVELOPER = 0x08
};

enum family_e {
    FAMILY_DECRED = 0x01
};

struct config_s {
    unsigned char options;
};
typedef struct config_s config_t;

typedef struct backup_area_s {
    config_t config;
    uint8_t trustedinput_key[32];
} backup_area_t;

typedef struct storage_s {
    unsigned char storageInitialized;

    unsigned char config_valid;
    backup_area_t bkp;

    unsigned char fidoTransport;

    uint8_t pubKeyRequestRestriction;

} storage_t;

// the global nvram memory variable
extern WIDE storage_t N_real;

#define N_btchip (*(WIDE storage_t *)PIC(&N_real))

#endif
