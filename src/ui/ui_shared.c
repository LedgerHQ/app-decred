/*******************************************************************************
*   Ledger App - Decred Wallet
*   (c) 2022 Ledger
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
#include "ui_shared.h"
#include "btchip_transaction.h"
#include "btchip_bagl_extensions.h"
#include "btchip_public_ram_variables.h"
#include "btchip_helpers.h"
#include "btchip_bcd.h"
#include "ui_main_menu.h"

unsigned int ux_step;
unsigned int ux_step_count;
uint8_t ux_loop_over_curr_element;
vars_u_t vars;

unsigned int io_seproxyhal_touch_display_cancel(const void* e){
    UNUSED(e);
    // user denied the transaction, tell the USB side
    btchip_bagl_user_action_display(0);
#ifndef HAVE_NBGL
    // redraw ui
    ui_idle();
#endif
    return 0; // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_display_ok(const void* e){
    UNUSED(e);
    // user accepted the transaction, tell the USB side
    btchip_bagl_user_action_display(1);
#ifndef HAVE_NBGL
    // redraw ui
    ui_idle();
#endif
    return 0; // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_verify_cancel(const void *e) {
    UNUSED(e);
    
    // user denied the transaction, tell the USB side
    if (!btchip_bagl_user_action(0)) {
#ifndef HAVE_NBGL
        // redraw ui
        ui_idle();
#endif
    }
    return 0; // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_verify_ok(const void *e) {
    UNUSED(e);    
    // user accepted the transaction, tell the USB side
    if (!btchip_bagl_user_action(1)) {
#ifndef HAVE_NBGL
        // redraw ui
        ui_idle();
#endif
    }
    return 0; // DO NOT REDRAW THE BUTTON
}

unsigned int
io_seproxyhal_touch_message_signature_verify_cancel(const void *e) {
    UNUSED(e);
    // user denied the transaction, tell the USB side
    btchip_bagl_user_action_message_signing(0);
#ifndef HAVE_NBGL
    // redraw ui
    ui_idle();
#endif
    return 0; // DO NOT REDRAW THE BUTTON
}

unsigned int
io_seproxyhal_touch_message_signature_verify_ok(const void *e) {
    UNUSED(e);
    // user accepted the transaction, tell the USB side
    btchip_bagl_user_action_message_signing(1);
#ifndef HAVE_NBGL
    // redraw ui
    ui_idle();
#endif
    return 0; // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_display_token_cancel(const void *e) {
    UNUSED(e);
    // revoke previous valid token if there was one
    btchip_context_D.has_valid_token = false;
    // user denied the token, tell the USB side
    btchip_bagl_user_action_display(0);
    // redraw ui
    ui_idle();
    return 0; // DO NOT REDRAW THE BUTTON
}

unsigned int io_seproxyhal_touch_display_token_ok(const void *e) {
    UNUSED(e);
    // Set the valid token flag
    btchip_context_D.has_valid_token = true;
    // user approved the token, tell the USB side
    btchip_bagl_user_action_display(1);
    // redraw ui
    ui_idle();
    return 0; // DO NOT REDRAW THE BUTTON
}


uint8_t prepare_fees() {
    if (btchip_context_D.transactionContext.relaxed) {
        memmove(vars.tmp.feesAmount, "UNKNOWN", 7);
        vars.tmp.feesAmount[7] = '\0';
    } else {
        unsigned char fees[8];
        unsigned short textSize;
        if (transaction_amount_sub_be(
                fees, btchip_context_D.transactionContext.transactionAmount,
                btchip_context_D.totalOutputAmount)) {
            PRINTF("Fees: %.*H\n", 8, fees);
            PRINTF("transactionAmount:: %.*H\n", 8, btchip_context_D.transactionContext.transactionAmount);
            PRINTF("totalOutputAmount: %.*H\n", 8, btchip_context_D.totalOutputAmount);
            PRINTF("Error : Fees not consistent");
            goto error;
        }
        memmove(vars.tmp.feesAmount, btchip_context_D.shortCoinId,
                   btchip_context_D.shortCoinIdLength);
        vars.tmp.feesAmount[btchip_context_D.shortCoinIdLength] = ' ';
        btchip_context_D.tmp =
            (unsigned char *)(vars.tmp.feesAmount +
                              btchip_context_D.shortCoinIdLength + 1);
        textSize = btchip_convert_hex_amount_to_displayable(fees);
        vars.tmp.feesAmount[textSize + btchip_context_D.shortCoinIdLength + 1] =
            '\0';
    }
    return 1;
error:
    return 0;
}

uint8_t prepare_message_signature() {
    uint8_t buffer[32];

    cx_hash((cx_hash_t *)&btchip_context_D.transactionHashWitness.header, CX_LAST,
            (const unsigned char *) vars.tmp.fullAmount, 0, buffer, 32);

#ifdef HAVE_NBGL
    snprintf(vars.tmp.fullAddress, sizeof(vars.tmp.fullAddress), "%.*H",
             32, buffer);
#else
    snprintf(vars.tmp.fullAddress, sizeof(vars.tmp.fullAddress), "%.*H...%.*H",
             8, buffer, 8, buffer + 32 - 8);
#endif
    return 1;
}

uint8_t prepare_single_output() {
    // TODO : special display for OP_RETURN
    unsigned char amount[8];
    char tmp[80];
    unsigned int offset = 0;
    unsigned char versionSize;
    int addressOffset;
    unsigned char address[22];
    unsigned short version; // addr prefix, or net id
    unsigned short textSize;
    unsigned char script_version[2]; // Decred thing

    vars.tmp.fullAddress[0] = '\0';
    btchip_swap_bytes(amount, btchip_context_D.currentOutput + offset, 8);
    offset += 8;

    btchip_swap_bytes(script_version, btchip_context_D.currentOutput + offset, 2);
    offset += 2;

    PRINTF("amount: %.*H\n", 8, amount);

    if (btchip_output_script_is_op_return(btchip_context_D.currentOutput +
                                          offset)) {
        strcpy(vars.tmp.fullAddress, "OP_RETURN");
    }
    else if (btchip_output_script_is_regular(btchip_context_D.currentOutput +
                                             offset)) {
        addressOffset = offset + 4;
        version = btchip_context_D.payToAddressVersion;
    }
    else {
        addressOffset = offset + 3;
        version = btchip_context_D.payToScriptHashVersion;
    }
    if (vars.tmp.fullAddress[0] == 0) {

        if (version > 255) {
            versionSize = 2;
            address[0] = (version >> 8);
            address[1] = version;
        } else {
            versionSize = 1;
            address[0] = version;
        }
        memmove(address + versionSize,
                   btchip_context_D.currentOutput + addressOffset, 20);

        // Prepare address
        textSize = btchip_public_key_to_encoded_base58(
                    address, 20 + versionSize, (unsigned char *)tmp,
                    sizeof(tmp), version, 1);
        tmp[textSize] = '\0';

        strcpy(vars.tmp.fullAddress, tmp);
    }

    // Prepare amount

    memmove(vars.tmp.fullAmount, btchip_context_D.shortCoinId,
               btchip_context_D.shortCoinIdLength);
    vars.tmp.fullAmount[btchip_context_D.shortCoinIdLength] = ' ';
    btchip_context_D.tmp =
        (unsigned char *)(vars.tmp.fullAmount +
                          btchip_context_D.shortCoinIdLength + 1);
    textSize = btchip_convert_hex_amount_to_displayable(amount);
    vars.tmp.fullAmount[textSize + btchip_context_D.shortCoinIdLength + 1] =
        '\0';

    return 1;
}

uint8_t prepare_full_output(uint8_t checkOnly) {
    PRINTF("prepare full output (check= %d):\n", checkOnly);
    unsigned int offset = 0;
    int numberOutputs;
    int i;
    unsigned int currentPos = 0;
    unsigned char amount[8], totalOutputAmount[8], fees[8];
    char tmp[80];
    unsigned char outputPos = 0, changeFound = 0;
    unsigned char script_version[2]; // Decred thing

    if (btchip_context_D.transactionContext.relaxed &&
        !btchip_context_D.transactionContext.consumeP2SH) {
        if (!checkOnly) {
            PRINTF("Error : Mixed inputs");
        }
        goto error;
    }
    if (btchip_context_D.transactionContext.consumeP2SH) {
        if (checkOnly) {
            goto error;
        }
        vars.tmp.fullAmount[0] = '\0';
        vars.tmp.feesAmount[0] = '\0';
        strcpy(vars.tmp.fullAddress, "P2SH");
        return 1;
    }
    // Parse output, locate the change output location
    explicit_bzero(totalOutputAmount, sizeof(totalOutputAmount));
    numberOutputs = btchip_context_D.currentOutput[offset++];
    PRINTF("%d outputs\n", numberOutputs);
    if (numberOutputs > 3) {
        if (!checkOnly) {
            PRINTF("Error : Too many outputs");
        }
        goto error;
    }
    for (i = 0; i < numberOutputs; i++) {
        unsigned char nullAmount = 1;
        unsigned int j;
        unsigned char isOpReturn, isP2sh;
        unsigned char isOpCreate, isOpCall;

        for (j = 0; j < 8; j++) {
            if (btchip_context_D.currentOutput[offset + j] != 0) {
                nullAmount = 0;
                break;
            }
        }
        btchip_swap_bytes(amount, btchip_context_D.currentOutput + offset, 8);
        transaction_amount_add_be(totalOutputAmount, totalOutputAmount, amount);
        offset += 8; // skip amount

        btchip_swap_bytes(script_version, btchip_context_D.currentOutput + offset, 2);
        offset += 2; // skip script_version

        isOpReturn = btchip_output_script_is_op_return(
            btchip_context_D.currentOutput + offset);
        isP2sh = btchip_output_script_is_p2sh(btchip_context_D.currentOutput +
                                              offset);
        isOpCreate = btchip_output_script_is_op_create(
            btchip_context_D.currentOutput + offset);
        isOpCall = btchip_output_script_is_op_call(
            btchip_context_D.currentOutput + offset);
        PRINTF("REGULAR SCRIPT: %d\n", btchip_output_script_is_regular(btchip_context_D.currentOutput + offset));
        // Always notify OP_RETURN to the user
        if (nullAmount && isOpReturn) {
            if (!checkOnly) {
                PRINTF("Error : Unexpected OP_RETURN");
            }
            goto error;
        }
        if (!btchip_output_script_is_regular(btchip_context_D.currentOutput +
                                             offset) &&
            !isP2sh && !(nullAmount && isOpReturn) &&
             (!isOpCreate && !isOpCall)) {
            if (!checkOnly) {
                PRINTF("Error : Unrecognized input script");
            }
            goto error;
        } else if (!btchip_output_script_is_regular(
                     btchip_context_D.currentOutput + offset) &&
                 !isP2sh && !(nullAmount && isOpReturn)) {
            if (!checkOnly) {
                PRINTF("Error : Unrecognized input script");
            }
            goto error;
        }
        if (btchip_context_D.tmpCtx.output.changeInitialized && !isOpReturn) {
            unsigned char addressOffset =
                (isP2sh ? OUTPUT_SCRIPT_P2SH_PRE_LENGTH
                                         : OUTPUT_SCRIPT_REGULAR_PRE_LENGTH);
            if (memcmp(btchip_context_D.currentOutput + offset +
                              addressOffset,
                          btchip_context_D.tmpCtx.output.changeAddress + 1,
                          20) == 0) {
                if (changeFound) {
                    if (!checkOnly) {
                        PRINTF("Error : Multiple change output found");
                    }
                    goto error;
                }
                changeFound = 1;
            } else {
                // outputPos is the real output pointer (opposed to the change address output)
                outputPos = currentPos;
            }
        }
        offset += 1 + btchip_context_D.currentOutput[offset];
        currentPos++;
    }
    if (btchip_context_D.tmpCtx.output.changeInitialized && !changeFound) {
        if (!checkOnly) {
            PRINTF("Error : change output not found");
        }
        goto error;
    }
    if (transaction_amount_sub_be(
            fees, btchip_context_D.transactionContext.transactionAmount,
            totalOutputAmount)) {
        PRINTF("tx_amount: ", 8, btchip_context_D.transactionContext.transactionAmount);
        PRINTF("total_amount: ", 8, totalOutputAmount);
        if (!checkOnly) {
            PRINTF("Error : Fees not consistent");
        }
        goto error;
    }
    if (!checkOnly) {
        // Format validation message
        currentPos = 0;
        offset = 1;
        btchip_context_D.tmp = (unsigned char *)tmp;
        for (i = 0; i < numberOutputs; i++) {
            if (!btchip_output_script_is_op_return(btchip_context_D.currentOutput + offset + 8 + 2)) {
                unsigned char versionSize;
                int addressOffset;
                unsigned char address[22];
                unsigned short version;

                btchip_swap_bytes(amount, btchip_context_D.currentOutput + offset, 8);
                offset += 8; // skip amount

                btchip_swap_bytes(script_version, btchip_context_D.currentOutput + offset, 2);
                offset += 2; // skip script_version

                if (btchip_output_script_is_regular(
                            btchip_context_D.currentOutput + offset)) {
                    addressOffset = offset + 4;
                    version = btchip_context_D.payToAddressVersion;
                    } else {
                    addressOffset = offset + 3;
                    version = btchip_context_D.payToScriptHashVersion;
                }
                    if (version > 255) {
                    versionSize = 2;
                    address[0] = (version >> 8);
                    address[1] = version;
                    } else {
                    versionSize = 1;
                    address[0] = version;
                }
                memmove(address + versionSize,
                            btchip_context_D.currentOutput + addressOffset,
                            20);

                // if we're processing the real output (not the change one)
                if (currentPos == outputPos) {
                    unsigned short textSize = 0;

                    // Prepare address
                    textSize = btchip_public_key_to_encoded_base58(
                        address, 20 + versionSize, (unsigned char *)tmp,
                        sizeof(tmp), version, 1);
                    tmp[textSize] = '\0';

                    strcpy(vars.tmp.fullAddress, tmp);

                    // Prepare amount
                    PRINTF("prepare amount\n");

                    memmove(vars.tmp.fullAmount,
                               btchip_context_D.shortCoinId,
                               btchip_context_D.shortCoinIdLength);
                    vars.tmp.fullAmount[btchip_context_D.shortCoinIdLength] =
                        ' ';
                    btchip_context_D.tmp =
                        (unsigned char *)(vars.tmp.fullAmount +
                                          btchip_context_D.shortCoinIdLength +
                                          1);
                    textSize = btchip_convert_hex_amount_to_displayable(amount);
                    vars.tmp
                        .fullAmount[textSize +
                                    btchip_context_D.shortCoinIdLength + 1] =
                        '\0';

                    // prepare fee display
                    PRINTF("prepare fee display\n");
                    memmove(vars.tmp.feesAmount,
                               btchip_context_D.shortCoinId,
                               btchip_context_D.shortCoinIdLength);
                    vars.tmp.feesAmount[btchip_context_D.shortCoinIdLength] =
                        ' ';
                    btchip_context_D.tmp =
                        (unsigned char *)(vars.tmp.feesAmount +
                                          btchip_context_D.shortCoinIdLength +
                                          1);
                    textSize = btchip_convert_hex_amount_to_displayable(fees);
                    vars.tmp
                        .feesAmount[textSize +
                                    btchip_context_D.shortCoinIdLength + 1] =
                        '\0';
                    break;
                }
            } else {
                // amount + version
                offset += 8 + 2;
            }
            offset += 1 + btchip_context_D.currentOutput[offset];
            currentPos++;
        }
    }
    return 1;
error:
    return 0;
}
