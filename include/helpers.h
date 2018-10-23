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

#ifndef HELPERS_H

#define HELPERS_H

#define OUTPUT_SCRIPT_REGULAR_PRE_LENGTH 4
#define OUTPUT_SCRIPT_REGULAR_POST_LENGTH 2
#define OUTPUT_SCRIPT_P2SH_PRE_LENGTH 3
#define OUTPUT_SCRIPT_P2SH_POST_LENGTH 1

#define OUTPUT_SCRIPT_NATIVE_WITNESS_PROGRAM_OFFSET 3

unsigned char output_script_is_regular(unsigned char *buffer);
unsigned char output_script_is_p2sh(unsigned char *buffer);
unsigned char output_script_is_op_return(unsigned char *buffer);

unsigned char output_script_is_op_create(unsigned char *buffer);
unsigned char output_script_is_op_call(unsigned char *buffer);

void sleep16(unsigned short delay);
void sleep32(unsigned long int delayEach, unsigned long int delayRepeat);

unsigned long int read_u32(unsigned char WIDE *buffer, unsigned char be,
                                  unsigned char skipSign);

void write_u32_be(unsigned char *buffer, unsigned long int value);
void write_u32_le(unsigned char *buffer, unsigned long int value);

void retrieve_keypair_discard(unsigned char WIDE *privateComponent,
                                     unsigned char derivePublic);

void perform_double_hash(unsigned char WIDE *in, unsigned short inlen,
                                unsigned char *out,
                                unsigned char hash1Algorithm,
                                unsigned char hash2Algorithm);

void public_key_hash160(unsigned char WIDE *in, unsigned short inlen,
                               unsigned char *out);
unsigned short public_key_to_encoded_base58(
    unsigned char WIDE *in, unsigned short inlen, unsigned char *out,
    unsigned short outlen, unsigned short version, unsigned char alreadyHashed);

unsigned short decode_base58_address(unsigned char WIDE *in,
                                            unsigned short inlen,
                                            unsigned char *out,
                                            unsigned short outlen);
void private_derive_keypair(unsigned char WIDE *bip32Path,
                                   unsigned char derivePublic,
                                   unsigned char *out_chainCode);

// void set_check_internal_structure_integrity(unsigned char
// setParameter);
#define set_check_internal_structure_integrity(x)
void swap_bytes(unsigned char *target, unsigned char *source,
                       unsigned char size);

void signverify_finalhash(void WIDE *keyContext, unsigned char sign,
                                 unsigned char WIDE *in, unsigned short inlen,
                                 unsigned char *out, unsigned short outlen,
                                 unsigned char rfc6979);

void transaction_add_output(unsigned char *hash160Address,
                                   unsigned char *amount, unsigned char p2sh);
unsigned char rng_u8_modulo(unsigned char modulo);
unsigned char secure_memcmp(const void WIDE *buf1, const void WIDE *buf2,
                                   unsigned short length);
unsigned char decrease_2fa(void);
void reset_2fa(void);
void reset_token(void);

#endif
