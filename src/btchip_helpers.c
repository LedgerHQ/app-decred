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
#include "btchip_apdu_constants.h"
#include "blake256.h"

const unsigned char TRANSACTION_OUTPUT_SCRIPT_PRE[] = {
    0x19,
    0x76,
    0xA9,
    0x14};  // script length, OP_DUP, OP_HASH160, address length
const unsigned char TRANSACTION_OUTPUT_SCRIPT_POST[] = {0x88, 0xAC};  // OP_EQUALVERIFY, OP_CHECKSIG

const unsigned char TRANSACTION_OUTPUT_SCRIPT_P2SH_PRE[] = {
    0x17,
    0xA9,
    0x14};  // script length, OP_HASH160, address length
const unsigned char TRANSACTION_OUTPUT_SCRIPT_P2SH_POST[] = {0x87};  // OP_EQUAL

const unsigned char TRANSACTION_OUTPUT_SCRIPT_P2WPKH_PRE[] = {0x16, 0x00, 0x14};
const unsigned char TRANSACTION_OUTPUT_SCRIPT_P2WSH_PRE[] = {0x22, 0x00, 0x20};

unsigned char btchip_output_script_is_regular(unsigned char *buffer) {
    if ((memcmp(buffer, TRANSACTION_OUTPUT_SCRIPT_PRE, sizeof(TRANSACTION_OUTPUT_SCRIPT_PRE)) ==
         0) &&
        (memcmp(buffer + sizeof(TRANSACTION_OUTPUT_SCRIPT_PRE) + 20,
                TRANSACTION_OUTPUT_SCRIPT_POST,
                sizeof(TRANSACTION_OUTPUT_SCRIPT_POST)) == 0)) {
        return 1;
    }
    PRINTF("Irregular script: %.*H\n", 10, buffer);
    return 0;
}

unsigned char btchip_output_script_is_p2sh(unsigned char *buffer) {
    if ((memcmp(buffer,
                TRANSACTION_OUTPUT_SCRIPT_P2SH_PRE,
                sizeof(TRANSACTION_OUTPUT_SCRIPT_P2SH_PRE)) == 0) &&
        (memcmp(buffer + sizeof(TRANSACTION_OUTPUT_SCRIPT_P2SH_PRE) + 20,
                TRANSACTION_OUTPUT_SCRIPT_P2SH_POST,
                sizeof(TRANSACTION_OUTPUT_SCRIPT_P2SH_POST)) == 0)) {
        return 1;
    }
    return 0;
}

unsigned char btchip_output_script_is_op_return(unsigned char *buffer) {
    return (buffer[1] == 0x6A);
}

unsigned char btchip_output_script_is_op_create(unsigned char *buffer) {
    return (!btchip_output_script_is_regular(buffer) && !btchip_output_script_is_p2sh(buffer) &&
            !btchip_output_script_is_op_return(buffer) && (buffer[0] <= 0xEA) &&
            (buffer[buffer[0]] == 0xC1));
}

unsigned char btchip_output_script_is_op_call(unsigned char *buffer) {
    return (!btchip_output_script_is_regular(buffer) && !btchip_output_script_is_p2sh(buffer) &&
            !btchip_output_script_is_op_return(buffer) && (buffer[0] <= 0xEA) &&
            (buffer[buffer[0]] == 0xC2));
}

unsigned char btchip_rng_u8_modulo(unsigned char modulo) {
    unsigned int rng_max = 256 % modulo;
    unsigned int rng_limit = 256 - rng_max;
    unsigned char candidate;
    while ((candidate = cx_rng_u8()) > rng_limit)
        ;
    return (candidate % modulo);
}

unsigned char btchip_secure_memcmp(const void *buf1, const void *buf2, unsigned short length) {
    unsigned char error = 0;
    while (length--) {
        error |= ((unsigned char *) buf1)[length] ^ ((unsigned char *) buf2)[length];
    }
    if (length != 0xffff) {
        return 1;
    }
    return error;
}

unsigned long int btchip_read_u32(unsigned char *buffer, unsigned char be, unsigned char skipSign) {
    unsigned char i;
    unsigned long int result = 0;
    unsigned char shiftValue = (be ? 24 : 0);
    for (i = 0; i < 4; i++) {
        unsigned char x = (unsigned char) buffer[i];
        if ((i == 0) && skipSign) {
            x &= 0x7f;
        }
        result += ((unsigned long int) x) << shiftValue;
        if (be) {
            shiftValue -= 8;
        } else {
            shiftValue += 8;
        }
    }
    return result;
}

void btchip_write_u32_be(unsigned char *buffer, unsigned long int value) {
    buffer[0] = ((value >> 24) & 0xff);
    buffer[1] = ((value >> 16) & 0xff);
    buffer[2] = ((value >> 8) & 0xff);
    buffer[3] = (value & 0xff);
}

void btchip_write_u32_le(unsigned char *buffer, unsigned long int value) {
    buffer[0] = (value & 0xff);
    buffer[1] = ((value >> 8) & 0xff);
    buffer[2] = ((value >> 16) & 0xff);
    buffer[3] = ((value >> 24) & 0xff);
}

void btchip_retrieve_keypair_discard(unsigned char *privateComponent, unsigned char derivePublic) {
    BEGIN_TRY {
        TRY {
            cx_ecdsa_init_private_key(BTCHIP_CURVE, privateComponent, 32, &btchip_private_key_D);

            PRINTF("Using private component\n%.*H\n", 32, privateComponent);

            if (derivePublic) {
                cx_ecfp_generate_pair(BTCHIP_CURVE, &btchip_public_key_D, &btchip_private_key_D, 1);
            }
        }
        FINALLY {
        }
    }
    END_TRY;
}

void btchip_public_key_hash160(unsigned char *in, unsigned short inlen, unsigned char *out) {
    union {
        BLAKE256_CTX blake;
        cx_ripemd160_t riprip;
    } u;
    unsigned char buffer[32];

    blake256_Init(&u.blake);
    blake256_Update(&u.blake, in, inlen);
    blake256_Final(&u.blake, buffer);
    cx_ripemd160_init(&u.riprip);
    cx_hash(&u.riprip.header, CX_LAST, buffer, 32, out, 32);
}

unsigned short btchip_public_key_to_encoded_base58(unsigned char *in,
                                                   unsigned short inlen,
                                                   unsigned char *out,
                                                   unsigned short outlen,
                                                   unsigned short version,
                                                   unsigned char alreadyHashed) {
    unsigned char tmpBuffer[26];
    unsigned char checksumBuffer[32];
    // cx_blake2b_t hash;
    BLAKE256_CTX hash;
    unsigned char versionSize = (version > 255 ? 2 : 1);
    size_t outputLen;

    if (!alreadyHashed) {
        PRINTF("To hash\n%.*H\n", inlen, in);
        btchip_public_key_hash160(in, inlen, tmpBuffer + versionSize);
        PRINTF("Hash160\n%.*H\n", 20, (tmpBuffer + versionSize));
        if (version > 255) {
            tmpBuffer[0] = (version >> 8);
            tmpBuffer[1] = version;
        } else {
            tmpBuffer[0] = version;
        }
    } else {
        memmove(tmpBuffer, in, 20 + versionSize);
    }

    blake256_Init(&hash);
    blake256_Update(&hash, tmpBuffer, 20 + versionSize);
    blake256_Final(&hash, checksumBuffer);
    blake256_Init(&hash);
    blake256_Update(&hash, checksumBuffer, 32);
    blake256_Final(&hash, checksumBuffer);

    PRINTF("Checksum\n%.*H\n", 4, checksumBuffer);
    memmove(tmpBuffer + 20 + versionSize, checksumBuffer, 4);

    outputLen = outlen;
    if (btchip_encode_base58(tmpBuffer, 24 + versionSize, out, &outputLen) < 0) {
        THROW(EXCEPTION);
    }
    return outputLen;
}

void btchip_swap_bytes(unsigned char *target, unsigned char *source, unsigned char size) {
    unsigned char i;
    for (i = 0; i < size; i++) {
        target[i] = source[size - 1 - i];
    }
}

unsigned short btchip_decode_base58_address(unsigned char *in,
                                            unsigned short inlen,
                                            unsigned char *out,
                                            unsigned short outlen) {
    unsigned char hashBuffer[32];
    cx_sha256_t hash;
    size_t outputLen = outlen;
    if (btchip_decode_base58((char *) in, inlen, out, &outputLen) < 0) {
        THROW(EXCEPTION);
    }
    outlen = outputLen;

    // Compute hash to verify address
    cx_sha256_init(&hash);
    cx_hash(&hash.header, CX_LAST, out, outlen - 4, hashBuffer, 32);
    cx_sha256_init(&hash);
    cx_hash(&hash.header, CX_LAST, hashBuffer, 32, hashBuffer, 32);

    if (memcmp(out + outlen - 4, hashBuffer, 4)) {
        PRINTF("Hash checksum mismatch\n%.*H\n", sizeof(hashBuffer), hashBuffer);
        THROW(INVALID_CHECKSUM);
    }

    return outlen;
}

void btchip_private_derive_keypair(unsigned char *bip32Path,
                                   unsigned char derivePublic,
                                   unsigned char *out_chainCode) {
    unsigned char bip32PathLength;
    unsigned char i;
    unsigned int bip32PathInt[MAX_BIP32_PATH];
    unsigned char privateComponent[32];

    bip32PathLength = bip32Path[0];
    if (bip32PathLength > MAX_BIP32_PATH) {
        THROW(INVALID_PARAMETER);
    }
    bip32Path++;
    for (i = 0; i < bip32PathLength; i++) {
        bip32PathInt[i] = btchip_read_u32(bip32Path, 1, 0);
        bip32Path += 4;
    }
    os_perso_derive_node_bip32(CX_CURVE_256K1,
                               bip32PathInt,
                               bip32PathLength,
                               privateComponent,
                               out_chainCode);
    btchip_retrieve_keypair_discard(privateComponent, derivePublic);
    explicit_bzero(privateComponent, sizeof(privateComponent));
}

/*
Checks if the values of a derivation path are within "normal" (arbitrary) ranges:
Account < 100, change == 1 or 0, address index < 50000
Returns 1 if the path is unusual, or not compliant with BIP44*/
unsigned char bip44_derivation_guard(unsigned char *bip32Path, bool is_change_path) {
    unsigned char i, path_len;
    unsigned int bip32PathInt[MAX_BIP32_PATH];

    path_len = bip32Path[0];
    bip32Path++;
    if (path_len > MAX_BIP32_PATH) {
        THROW(INVALID_PARAMETER);
    }

    for (i = 0; i < path_len; i++) {
        bip32PathInt[i] = btchip_read_u32(bip32Path, 1, 0);
        bip32Path += 4;
    }

    // If the path length is not compliant with BIP44 or if the purpose/coin type don't match
    // regular usage
    if (path_len != BIP44_PATH_LEN || ((bip32PathInt[BIP44_PURPOSE_OFFSET] ^ 0x80000000) != 44 &&
                                       (bip32PathInt[BIP44_PURPOSE_OFFSET] ^ 0x80000000) != 49 &&
                                       (bip32PathInt[BIP44_PURPOSE_OFFSET] ^ 0x80000000) != 84)) {
        return 1;
    }

    // If the account or address index is very high or if the change isn't 1, return a warning
    if ((bip32PathInt[BIP44_ACCOUNT_OFFSET] ^ 0x80000000) > MAX_BIP44_ACCOUNT_RECOMMENDED ||
                bip32PathInt[BIP44_CHANGE_OFFSET] != is_change_path
            ? 1
            : 0 || bip32PathInt[BIP44_ADDRESS_INDEX_OFFSET] > MAX_BIP44_ADDRESS_INDEX_RECOMMENDED) {
        return 1;
    }

    return 0;
}

// Print a BIP32 path as an ascii string to display on the device screen
unsigned char bip32_print_path(unsigned char *bip32Path, char *out, unsigned char max_out_len) {
    unsigned char bip32PathLength;
    unsigned char i, offset;
    unsigned int current_level;
    bool hardened;

    bip32PathLength = bip32Path[0];
    if (bip32PathLength > MAX_BIP32_PATH) {
        THROW(INVALID_PARAMETER);
    }
    bip32Path++;
    out[0] = ' ';
    offset = 1;
    for (i = 0; i < bip32PathLength; i++) {
        current_level = btchip_read_u32(bip32Path, 1, 0);
        hardened = (bool) (current_level & 0x80000000);
        if (hardened) {
            // remove hardening flag
            current_level ^= 0x80000000;
        }
        bip32Path += 4;
        snprintf(out + offset, max_out_len - offset, "%u", current_level);
        offset = strnlen(out, max_out_len);
        if (offset >= max_out_len - 2) THROW(EXCEPTION_OVERFLOW);
        if (hardened) out[offset++] = '\'';

        out[offset++] = '/';
        out[offset] = '\0';
    }
    // remove last '/'
    out[offset - 1] = '\0';

    return offset - 1;
}

void btchip_transaction_add_output(unsigned char *hash160Address,
                                   unsigned char *amount,
                                   unsigned char p2sh) {
    const unsigned char *pre =
        (p2sh ? TRANSACTION_OUTPUT_SCRIPT_P2SH_PRE : TRANSACTION_OUTPUT_SCRIPT_PRE);
    const unsigned char *post =
        (p2sh ? TRANSACTION_OUTPUT_SCRIPT_P2SH_POST : TRANSACTION_OUTPUT_SCRIPT_POST);
    unsigned char sizePre =
        (p2sh ? sizeof(TRANSACTION_OUTPUT_SCRIPT_P2SH_PRE) : sizeof(TRANSACTION_OUTPUT_SCRIPT_PRE));
    unsigned char sizePost = (p2sh ? sizeof(TRANSACTION_OUTPUT_SCRIPT_P2SH_POST)
                                   : sizeof(TRANSACTION_OUTPUT_SCRIPT_POST));
    if (amount != NULL) {
        btchip_swap_bytes(btchip_context_D.tmp, amount, 8);
        btchip_context_D.tmp += 8;
    }
    memmove(btchip_context_D.tmp, (void *) pre, sizePre);
    btchip_context_D.tmp += sizePre;
    memmove(btchip_context_D.tmp, hash160Address, 20);
    btchip_context_D.tmp += 20;
    memmove(btchip_context_D.tmp, (void *) post, sizePost);
    btchip_context_D.tmp += sizePost;
}

void btchip_signverify_finalhash(void *keyContext,
                                 unsigned char sign,
                                 unsigned char *in,
                                 unsigned short inlen,
                                 unsigned char *out,
                                 unsigned short outlen,
                                 unsigned char rfc6979) {
    if (sign) {
        unsigned int info = 0;
        cx_ecdsa_sign((cx_ecfp_private_key_t *) keyContext,
                      CX_LAST | (rfc6979 ? CX_RND_RFC6979 : CX_RND_TRNG),
                      CX_SHA256,
                      in,
                      inlen,
                      out,
                      outlen,
                      &info);
        if (info & CX_ECCINFO_PARITY_ODD) {
            out[0] |= 0x01;
        }
    } else {
        cx_ecdsa_verify((cx_ecfp_public_key_t *) keyContext,
                        CX_LAST,
                        CX_SHA256,
                        in,
                        inlen,
                        out,
                        outlen);
    }
}
