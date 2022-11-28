/*******************************************************************************
*   Ledger App - Bitcoin Wallet
*   (c) 2016-2019 Ledger
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

#include "btchip_internal.h"

void autosetup(void){
    btchip_config_t config;
    unsigned char i;
    unsigned char tmp[32];
    explicit_bzero(&config, sizeof(btchip_config_t));
    config.options |= BTCHIP_OPTION_DETERMINISTIC_SIGNATURE;
    config.options |= BTCHIP_OPTION_SKIP_2FA_P2SH; // TODO : remove when
                                                   // supporting multi output

    nvm_write((void *)&N_btchip.bkp.config, &config, sizeof(config));
    cx_rng(tmp, sizeof(tmp));
    nvm_write((void *)&N_btchip.bkp.trustedinput_key, tmp, sizeof(tmp));
    i = 1;
    nvm_write((void *)&N_btchip.config_valid, &i, 1);
}

/**
 * Initialize the application context on boot
 */
void btchip_context_init() {
    PRINTF("Context init\n");
    PRINTF(N_btchip.bkp);
    explicit_bzero(&btchip_context_D, sizeof(btchip_context_D));
    SB_SET(btchip_context_D.halted, 0);
    btchip_context_D.currentOutputOffset = 0;
    btchip_context_D.outputParsingState = BTCHIP_OUTPUT_PARSING_NUMBER_OUTPUTS;
    explicit_bzero(btchip_context_D.totalOutputAmount,
              sizeof(btchip_context_D.totalOutputAmount));
    btchip_context_D.changeOutputFound = 0;


    if (N_btchip.config_valid != 0x01) {
        autosetup();
    }

    if (!N_btchip.config_valid) {
        unsigned char defaultMode;
        PRINTF("No configuration found\n");
        defaultMode = BTCHIP_MODE_WALLET;

    } 
    else {
        btchip_context_D.payToAddressVersion = G_coin_config->p2pkh_version;
        btchip_context_D.payToScriptHashVersion = G_coin_config->p2sh_version;
        btchip_context_D.coinFamily = G_coin_config->family;
        btchip_context_D.coinIdLength = strlen(PIC(G_coin_config->coinid));
        memmove(btchip_context_D.coinId, PIC(G_coin_config->coinid),
                   btchip_context_D.coinIdLength);
        btchip_context_D.shortCoinIdLength =
            strlen(PIC(G_coin_config->name_short));
        memmove(btchip_context_D.shortCoinId, PIC(G_coin_config->name_short),
                   btchip_context_D.shortCoinIdLength);

    }
    if (!N_btchip.storageInitialized) {
        unsigned char initialized = 1, denied=1;

        nvm_write((void *)&N_btchip.pubKeyRequestRestriction, &denied, 1);
        nvm_write((void *)&N_btchip.storageInitialized, &initialized, 1);
    }
}
