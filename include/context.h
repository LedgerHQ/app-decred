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

#ifndef CONTEXT_H

#define CONTEXT_H

#include "os.h"
#include "secure_value.h"
#include "filesystem_tx.h"
#include "blake256.h"

#define MAX_OUTPUT_TO_CHECK 200
#define MAX_COIN_ID 13
#define MAX_SHORT_COIN_ID 5 

#define MAGIC_TRUSTED_INPUT 0x32
#define MAGIC_DEV_KEY 0x01

enum modes_e {
    MODE_WALLET = 0x01,
};

enum options_e {
    OPTION_UNCOMPRESSED_KEYS = 0x01,
    OPTION_DETERMINISTIC_SIGNATURE = 0x02,
    OPTION_FREE_SIGHASHTYPE = 0x04,
    OPTION_SKIP_2FA_P2SH = 0x08,
    OPTION_ALLOW_ARBITRARY_CHANGE = 0x10
};

/**
 * Current state of an untrusted transaction hashing
 */
enum transaction_state_e {
    /** No transaction in progress */
    TRANSACTION_NONE = 0x00,
    /** Transaction defined, waiting for an input to be hashed */
    TRANSACTION_DEFINED_WAIT_INPUT = 0x01,
    /** Transaction defined, input hashing in progress, pending input script
       data */
    TRANSACTION_INPUT_HASHING_IN_PROGRESS_INPUT_SCRIPT = 0x02,
    /** Transaction defined, input hashing done, pending output hashing for this
       input */
    TRANSACTION_INPUT_HASHING_DONE = 0x03,
    /** Transaction defined, waiting for an output to be hashed */
    TRANSACTION_DEFINED_WAIT_OUTPUT = 0x04,
    /** Transaction defined, output hashing in progress for a complex script,
       pending output script data */
    TRANSACTION_OUTPUT_HASHING_IN_PROGRESS_OUTPUT_SCRIPT = 0x05,
    /** Transaction defined, output hashing done, pending finalization */
    TRANSACTION_OUTPUT_HASHING_DONE = 0x06,
    /** Extra data present */
    TRANSACTION_PROCESS_EXTRA = 0x07,
    /** Transaction parsed */
    TRANSACTION_PARSED = 0x08,
    /** Transaction parsed, ready to prepare for signature after validating the
       user outputs */
    TRANSACTION_PRESIGN_READY = 0x09,
    /** Transaction fully parsed, ready to be signed */
    TRANSACTION_SIGN_READY = 0x0a,
};
typedef enum transaction_state_e transaction_state_t;

enum output_parsing_state_e {
    OUTPUT_PARSING_NONE = 0x00,
    OUTPUT_PARSING_NUMBER_OUTPUTS = 0x01,
    OUTPUT_PARSING_OUTPUT = 0x02,
    OUTPUT_FINALIZE_TX = 0x03,
    OUTPUT_HANDLE_LEGACY = 0xFF
};
typedef enum output_parsing_state_e output_parsing_state_t;


/**
 * Structure defining an operation on a transaction
 */
struct transaction_context_s {
    /** Transient over signing components */

    /** Remaining number of inputs/outputs to process for this transaction */
    unsigned long int transactionRemainingInputsOutputs;
    /** Index of the currently processed input/output for this transaction */
    unsigned long int transactionCurrentInputOutput;
    /** Remaining script bytes to process for the current input or output */
    unsigned long int scriptRemaining;

    /** Persistent over signing components */

    /** State of the transaction, type transaction_state_t */
    unsigned char transactionState;
    /** Computed sum of transaction inputs or value of the output to convert to
     * a trusted input */
    unsigned char transactionAmount[8];
    /** Flag indicating if this transaction has been processed before */
    unsigned char firstSigned;
    /** If the transaction is relaxed */
    unsigned char relaxed;
    /** If the transaction consumes a P2SH input */
    unsigned char consumeP2SH;
};
typedef struct transaction_context_s transaction_context_t;

struct tmp_output_s {
    /** Change address if initialized */
    unsigned char changeAddress[21];
    /** Flag set if the change address was initialized */
    unsigned char changeInitialized;
    /** Flag set if the change address was checked */
    unsigned char changeChecked;
    /** Flag set if the change address can be submitted */
    unsigned char changeAccepted;
    /** Flag set if the outputs have been fragmented */
    unsigned char multipleOutput;
};
typedef struct tmp_output_s tmp_output_t;

struct context_s {
    /** Flag if dongle has been halted */
    secu8 halted;
    /** Index of the output to convert into a trusted input in a transaction */
    unsigned long int trustedInputIndex;
    /** (Integrity protected) transaction context */
    transaction_context_t transactionContext;

    /** Current Pay To Address version */
    unsigned short payToAddressVersion;
    /** Current Pay To Script Hash version */
    unsigned short payToScriptHashVersion;
    /** Current coin family */
    unsigned char coinFamily;
    /** Current Coin ID */
    unsigned char coinId[MAX_COIN_ID];
    /** Current short Coin ID */
    unsigned char shortCoinId[MAX_SHORT_COIN_ID];
    /** Current Coin ID length */
    unsigned char coinIdLength;    
    /** Current short Coin ID length */
    unsigned char shortCoinIdLength;        

    /** Non protected transaction context */

    /** Transaction Prefix hash context */
    BLAKE256_CTX transactionHashPrefix;
    /** Transaction Witness hash context */
    BLAKE256_CTX transactionHashWitness; 
    /** Authorization hash context */
    BLAKE256_CTX transactionHashAuthorization;
    /** Current hash to perform (TRANSACTION_HASH_) */
    unsigned char transactionHashOption;

    unsigned char transactionVersion[4];
    unsigned char inputValue[8];


    /** Size currently available to the transaction parser */
    unsigned char transactionDataRemaining;
    /** Current pointer to the transaction buffer for the transaction parser */
    unsigned char *transactionBufferPointer;
    /** Trusted Input index processed */
    unsigned char trustedInputProcessed;
    /** Transaction input to catch for a Trusted Input lookup */
    unsigned long int transactionTargetInput;

    /** Length of the incoming command */
    unsigned short inLength;
    /** Length of the outgoing command */
    unsigned short outLength;

    /** IO flags to reply with at the end of an APDU handler */
    unsigned char io_flags;

    /** Status Word of the response */
    unsigned short sw;

    /** Current scratch buffer */
    unsigned char *tmp;

    // was previously in NVRAM
    transaction_summary_t transactionSummary;


    unsigned short hashedMessageLength;

    union {
        tmp_output_t output;
    } tmpCtx;

    unsigned char currentOutput[MAX_OUTPUT_TO_CHECK];
    unsigned short currentOutputOffset;
    unsigned int remainingOutputs;
    unsigned int totalOutputs;
    unsigned int discardSize;
    unsigned char outputParsingState;
    unsigned char totalOutputAmount[8];
    unsigned char changeOutputFound;    
};
typedef struct context_s context_t;


/**
 * Structure to configure the bitcoin application for a given altcoin
 * 
 */
typedef enum coin_flags_e {
    DUMMY_FLAG=1
} coin_flags_t;


typedef enum coin_kind_e {
    COIN_KIND_DECRED,
    COIN_KIND_DECRED_TESTNET,
} coin_kind_t;

typedef struct altcoin_config_s {
    unsigned short p2pkh_version;
    unsigned short p2sh_version;
    unsigned char family;
    //unsigned char* iconsuffix;// will use the icon provided on the stack (maybe)
    #ifdef TARGET_BLUE
        const char* header_text;
        unsigned int color_header;
        unsigned int color_dashboard;
    #endif // TARGET_BLUE
    
    const char* coinid; // used coind id for message signature prefix
    const char* name; // for ux displays
    const char* name_short; // for unit in ux displays
    unsigned int forkid;
    coin_kind_t kind;
    unsigned int flags;
} altcoin_config_t;

void context_init(void);

#endif
