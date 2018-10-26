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

#include "internal.h"

void autosetup(void){
    config_t config;
    unsigned char i;
    unsigned char tmp[32];
    os_memset(&config, 0, sizeof(config_t));
    config.options |= OPTION_DETERMINISTIC_SIGNATURE;
    config.options |= OPTION_SKIP_2FA_P2SH; // TODO : remove when
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
void context_init() {
    L_DEBUG_APP(("Context init\n"));
    L_DEBUG_APP((N_btchip.bkp));
    os_memset(&context_D, 0, sizeof(context_D));
    SB_SET(context_D.halted, 0);
    context_D.currentOutputOffset = 0;
    context_D.outputParsingState = OUTPUT_PARSING_NUMBER_OUTPUTS;
    os_memset(context_D.totalOutputAmount, 0,
              sizeof(context_D.totalOutputAmount));
    context_D.changeOutputFound = 0;


    if (N_btchip.config_valid != 0x01) {
        autosetup();
    }

    if (!N_btchip.config_valid) {
        unsigned char defaultMode;
        L_DEBUG_APP(("No configuration found\n"));
        defaultMode = MODE_WALLET;

    } 
    else {
        context_D.payToAddressVersion = G_coin_config->p2pkh_version;
        context_D.payToScriptHashVersion = G_coin_config->p2sh_version;
        context_D.coinFamily = G_coin_config->family;
        context_D.coinIdLength = strlen(PIC(G_coin_config->coinid));
        os_memmove(context_D.coinId, PIC(G_coin_config->coinid),
                   context_D.coinIdLength);
        context_D.shortCoinIdLength =
            strlen(PIC(G_coin_config->name_short));
        os_memmove(context_D.shortCoinId, PIC(G_coin_config->name_short),
                   context_D.shortCoinIdLength);

    }
    if (!N_btchip.storageInitialized) {
        unsigned char initialized = 1, denied=0;

        nvm_write((void *)&N_btchip.pubKeyRequestRestriction, &denied, 1);
        nvm_write((void *)&N_btchip.storageInitialized, &initialized, 1);
    }
}
