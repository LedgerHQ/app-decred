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

void autosetup(void);

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
        defaultMode = MODE_SETUP_NEEDED;

        set_operation_mode(defaultMode);
    } else {
        /*
        context_D.payToAddressVersion =
        N_btchip.bkp.config.payToAddressVersion;
        context_D.payToScriptHashVersion =
        N_btchip.bkp.config.payToScriptHashVersion;
            context_D.coinFamily = N_btchip.bkp.config.coinFamily;
        context_D.coinIdLength = N_btchip.bkp.config.coinIdLength;
        os_memmove(context_D.coinId, N_btchip.bkp.config.coinId,
        N_btchip.bkp.config.coinIdLength);
        context_D.shortCoinIdLength =
        N_btchip.bkp.config.shortCoinIdLength;
        os_memmove(context_D.shortCoinId,
        N_btchip.bkp.config.shortCoinId, N_btchip.bkp.config.shortCoinIdLength);
        */
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

        SB_CHECK(N_btchip.bkp.config.operationMode);
    }
    if (!N_btchip.storageInitialized) {
        unsigned char initialized = 1;

        nvm_write((void *)&N_btchip.storageInitialized, &initialized, 1);
    }
}
