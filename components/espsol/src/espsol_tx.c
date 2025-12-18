/**
 * @file espsol_tx.c
 * @brief ESPSOL Transaction Building Implementation
 *
 * Implements transaction building, signing, and serialization
 * using Solana's wire format (compact arrays).
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include "espsol_tx.h"
#include "espsol_utils.h"
#include "espsol_crypto.h"

#include <string.h>
#include <stdlib.h>

#if defined(ESP_PLATFORM) && ESP_PLATFORM
#include "esp_log.h"
#else
#define ESP_LOGI(tag, ...)
#define ESP_LOGW(tag, ...)
#define ESP_LOGE(tag, ...)
#define ESP_LOGD(tag, ...)
#endif

static const char *TAG = "espsol_tx";

/* ============================================================================
 * Well-Known Program IDs
 * ========================================================================== */

/* System Program: 11111111111111111111111111111111 (32 zero bytes) */
const uint8_t ESPSOL_SYSTEM_PROGRAM_ID[ESPSOL_PUBKEY_SIZE] = {0};

/* Token Program: TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA */
const uint8_t ESPSOL_TOKEN_PROGRAM_ID[ESPSOL_PUBKEY_SIZE] = {
    0x06, 0xdd, 0xf6, 0xe1, 0xd7, 0x65, 0xa1, 0x93,
    0xd9, 0xcb, 0xe1, 0x46, 0xce, 0xeb, 0x79, 0xac,
    0x1c, 0xb4, 0x85, 0xed, 0x5f, 0x5b, 0x37, 0x91,
    0x3a, 0x8c, 0xf5, 0x85, 0x7e, 0xff, 0x00, 0xa9
};

/* Associated Token Program: ATokenGPvbdGVxr1b2hvZbsiqW5xWH25efTNsLJA8knL */
const uint8_t ESPSOL_ASSOCIATED_TOKEN_PROGRAM_ID[ESPSOL_PUBKEY_SIZE] = {
    0x8c, 0x97, 0x25, 0x8f, 0x4e, 0x24, 0x89, 0xf1,
    0xbb, 0x3d, 0x10, 0x29, 0x14, 0x8e, 0x0d, 0x83,
    0x0b, 0x5a, 0x13, 0x99, 0xda, 0xff, 0x10, 0x84,
    0x04, 0x8e, 0x7b, 0xd8, 0xdb, 0xe9, 0xf8, 0x59
};

/* Memo Program: MemoSq4gqABAXKb96qnH8TysNcWxMyWCqXgDLGmfcHr */
const uint8_t ESPSOL_MEMO_PROGRAM_ID[ESPSOL_PUBKEY_SIZE] = {
    0x05, 0x4a, 0x53, 0x5a, 0x99, 0x29, 0x21, 0x06,
    0x4d, 0x24, 0xe8, 0x71, 0x60, 0xda, 0x38, 0x7c,
    0x7c, 0x35, 0xb5, 0xdd, 0xbc, 0x92, 0xbb, 0x81,
    0xe4, 0x1f, 0xa8, 0x40, 0x41, 0x05, 0x44, 0x8d
};

/* ============================================================================
 * Internal Structures
 * ========================================================================== */

/**
 * @brief Internal instruction structure
 */
typedef struct {
    uint8_t program_id[ESPSOL_PUBKEY_SIZE];
    espsol_account_meta_t accounts[ESPSOL_MAX_ACCOUNTS];
    size_t account_count;
    uint8_t data[ESPSOL_MAX_INSTRUCTION_DATA];
    size_t data_len;
} espsol_instruction_t;

/**
 * @brief Internal account entry for deduplication
 */
typedef struct {
    uint8_t pubkey[ESPSOL_PUBKEY_SIZE];
    bool is_signer;
    bool is_writable;
} espsol_account_entry_t;

/**
 * @brief Internal transaction structure
 */
struct espsol_transaction {
    /* Fee payer (first signer) */
    uint8_t fee_payer[ESPSOL_PUBKEY_SIZE];
    bool has_fee_payer;
    
    /* Recent blockhash */
    uint8_t blockhash[ESPSOL_BLOCKHASH_SIZE];
    bool has_blockhash;
    
    /* Instructions */
    espsol_instruction_t instructions[ESPSOL_MAX_INSTRUCTIONS];
    size_t instruction_count;
    
    /* Deduplicated accounts (built during serialization) */
    espsol_account_entry_t accounts[ESPSOL_MAX_ACCOUNTS];
    size_t account_count;
    
    /* Signatures */
    uint8_t signatures[ESPSOL_MAX_SIGNERS][ESPSOL_SIGNATURE_SIZE];
    uint8_t signer_pubkeys[ESPSOL_MAX_SIGNERS][ESPSOL_PUBKEY_SIZE];
    size_t signer_count;
    size_t required_signers;
    
    /* State */
    bool is_signed;
    bool accounts_compiled;
};

/* ============================================================================
 * Internal Helper Functions
 * ========================================================================== */

/**
 * @brief Check if two public keys are equal
 */
static bool pubkey_equals(const uint8_t a[ESPSOL_PUBKEY_SIZE],
                          const uint8_t b[ESPSOL_PUBKEY_SIZE])
{
    return memcmp(a, b, ESPSOL_PUBKEY_SIZE) == 0;
}

/**
 * @brief Find or add account to the deduplicated list
 * @return Account index, or -1 if list is full
 */
static int find_or_add_account(struct espsol_transaction *tx,
                               const uint8_t pubkey[ESPSOL_PUBKEY_SIZE],
                               bool is_signer,
                               bool is_writable)
{
    /* Search for existing account */
    for (size_t i = 0; i < tx->account_count; i++) {
        if (pubkey_equals(tx->accounts[i].pubkey, pubkey)) {
            /* Update flags (merge with more permissive) */
            tx->accounts[i].is_signer = tx->accounts[i].is_signer || is_signer;
            tx->accounts[i].is_writable = tx->accounts[i].is_writable || is_writable;
            return (int)i;
        }
    }
    
    /* Add new account */
    if (tx->account_count >= ESPSOL_MAX_ACCOUNTS) {
        return -1;
    }
    
    memcpy(tx->accounts[tx->account_count].pubkey, pubkey, ESPSOL_PUBKEY_SIZE);
    tx->accounts[tx->account_count].is_signer = is_signer;
    tx->accounts[tx->account_count].is_writable = is_writable;
    
    return (int)tx->account_count++;
}

/**
 * @brief Compile accounts from instructions (deduplication and ordering)
 *
 * Solana account ordering:
 * 1. Writable signers
 * 2. Read-only signers
 * 3. Writable non-signers
 * 4. Read-only non-signers
 */
static esp_err_t compile_accounts(struct espsol_transaction *tx)
{
    if (tx->accounts_compiled) {
        return ESP_OK;
    }
    
    tx->account_count = 0;
    tx->required_signers = 0;
    
    /* Add fee payer first (always writable signer) */
    if (tx->has_fee_payer) {
        if (find_or_add_account(tx, tx->fee_payer, true, true) < 0) {
            return ESP_ERR_ESPSOL_MAX_ACCOUNTS;
        }
    }
    
    /* Add all accounts from instructions */
    for (size_t i = 0; i < tx->instruction_count; i++) {
        espsol_instruction_t *ix = &tx->instructions[i];
        
        /* Add program ID (read-only, non-signer) */
        if (find_or_add_account(tx, ix->program_id, false, false) < 0) {
            return ESP_ERR_ESPSOL_MAX_ACCOUNTS;
        }
        
        /* Add instruction accounts */
        for (size_t j = 0; j < ix->account_count; j++) {
            if (find_or_add_account(tx, ix->accounts[j].pubkey,
                                    ix->accounts[j].is_signer,
                                    ix->accounts[j].is_writable) < 0) {
                return ESP_ERR_ESPSOL_MAX_ACCOUNTS;
            }
        }
    }
    
    /* Sort accounts: signers first, then writable, then read-only */
    /* Using simple bubble sort since max accounts is small */
    for (size_t i = 0; i < tx->account_count; i++) {
        for (size_t j = i + 1; j < tx->account_count; j++) {
            bool swap = false;
            
            /* Priority: signer+writable > signer+readonly > writable > readonly */
            int priority_i = (tx->accounts[i].is_signer ? 2 : 0) +
                            (tx->accounts[i].is_writable ? 1 : 0);
            int priority_j = (tx->accounts[j].is_signer ? 2 : 0) +
                            (tx->accounts[j].is_writable ? 1 : 0);
            
            if (priority_j > priority_i) {
                swap = true;
            }
            
            if (swap) {
                espsol_account_entry_t tmp = tx->accounts[i];
                tx->accounts[i] = tx->accounts[j];
                tx->accounts[j] = tmp;
            }
        }
    }
    
    /* Count required signers */
    for (size_t i = 0; i < tx->account_count; i++) {
        if (tx->accounts[i].is_signer) {
            tx->required_signers++;
        }
    }
    
    tx->accounts_compiled = true;
    return ESP_OK;
}

/**
 * @brief Find account index in compiled list
 */
static int find_account_index(struct espsol_transaction *tx,
                              const uint8_t pubkey[ESPSOL_PUBKEY_SIZE])
{
    for (size_t i = 0; i < tx->account_count; i++) {
        if (pubkey_equals(tx->accounts[i].pubkey, pubkey)) {
            return (int)i;
        }
    }
    return -1;
}

/**
 * @brief Write compact-u16 encoding
 * @return Number of bytes written
 */
static size_t write_compact_u16(uint8_t *buffer, uint16_t value)
{
    if (value < 0x80) {
        buffer[0] = (uint8_t)value;
        return 1;
    } else if (value < 0x4000) {
        buffer[0] = (uint8_t)((value & 0x7F) | 0x80);
        buffer[1] = (uint8_t)(value >> 7);
        return 2;
    } else {
        buffer[0] = (uint8_t)((value & 0x7F) | 0x80);
        buffer[1] = (uint8_t)(((value >> 7) & 0x7F) | 0x80);
        buffer[2] = (uint8_t)(value >> 14);
        return 3;
    }
}

/**
 * @brief Serialize the transaction message (for signing)
 */
static esp_err_t serialize_message(struct espsol_transaction *tx,
                                    uint8_t *buffer, size_t buffer_len,
                                    size_t *out_len)
{
    esp_err_t err = compile_accounts(tx);
    if (err != ESP_OK) {
        return err;
    }
    
    if (!tx->has_blockhash) {
        ESP_LOGE(TAG, "Transaction missing blockhash");
        return ESP_ERR_ESPSOL_TX_BUILD_ERROR;
    }
    
    size_t offset = 0;
    
    /* Message header (3 bytes) */
    /* num_required_signatures */
    uint8_t num_readonly_signed = 0;
    uint8_t num_readonly_unsigned = 0;
    
    /* Count read-only accounts */
    for (size_t i = 0; i < tx->account_count; i++) {
        if (!tx->accounts[i].is_writable) {
            if (tx->accounts[i].is_signer) {
                num_readonly_signed++;
            } else {
                num_readonly_unsigned++;
            }
        }
    }
    
    if (offset + 3 > buffer_len) return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    buffer[offset++] = (uint8_t)tx->required_signers;
    buffer[offset++] = num_readonly_signed;
    buffer[offset++] = num_readonly_unsigned;
    
    /* Account addresses (compact array) */
    size_t compact_len = write_compact_u16(buffer + offset, (uint16_t)tx->account_count);
    offset += compact_len;
    
    if (offset + tx->account_count * ESPSOL_PUBKEY_SIZE > buffer_len) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    for (size_t i = 0; i < tx->account_count; i++) {
        memcpy(buffer + offset, tx->accounts[i].pubkey, ESPSOL_PUBKEY_SIZE);
        offset += ESPSOL_PUBKEY_SIZE;
    }
    
    /* Recent blockhash */
    if (offset + ESPSOL_BLOCKHASH_SIZE > buffer_len) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    memcpy(buffer + offset, tx->blockhash, ESPSOL_BLOCKHASH_SIZE);
    offset += ESPSOL_BLOCKHASH_SIZE;
    
    /* Instructions (compact array) */
    compact_len = write_compact_u16(buffer + offset, (uint16_t)tx->instruction_count);
    offset += compact_len;
    
    for (size_t i = 0; i < tx->instruction_count; i++) {
        espsol_instruction_t *ix = &tx->instructions[i];
        
        /* Program ID index */
        int prog_idx = find_account_index(tx, ix->program_id);
        if (prog_idx < 0) {
            return ESP_ERR_ESPSOL_TX_BUILD_ERROR;
        }
        if (offset + 1 > buffer_len) return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
        buffer[offset++] = (uint8_t)prog_idx;
        
        /* Account indices (compact array) */
        compact_len = write_compact_u16(buffer + offset, (uint16_t)ix->account_count);
        offset += compact_len;
        
        for (size_t j = 0; j < ix->account_count; j++) {
            int acc_idx = find_account_index(tx, ix->accounts[j].pubkey);
            if (acc_idx < 0) {
                return ESP_ERR_ESPSOL_TX_BUILD_ERROR;
            }
            if (offset + 1 > buffer_len) return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
            buffer[offset++] = (uint8_t)acc_idx;
        }
        
        /* Instruction data (compact array) */
        compact_len = write_compact_u16(buffer + offset, (uint16_t)ix->data_len);
        offset += compact_len;
        
        if (offset + ix->data_len > buffer_len) {
            return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
        }
        memcpy(buffer + offset, ix->data, ix->data_len);
        offset += ix->data_len;
    }
    
    *out_len = offset;
    return ESP_OK;
}

/* ============================================================================
 * Transaction Lifecycle
 * ========================================================================== */

esp_err_t espsol_tx_create(espsol_tx_handle_t *tx)
{
    if (!tx) {
        return ESP_ERR_INVALID_ARG;
    }
    
    struct espsol_transaction *t = calloc(1, sizeof(struct espsol_transaction));
    if (!t) {
        ESP_LOGE(TAG, "Failed to allocate transaction");
        return ESP_ERR_NO_MEM;
    }
    
    *tx = t;
    ESP_LOGD(TAG, "Transaction created");
    return ESP_OK;
}

esp_err_t espsol_tx_destroy(espsol_tx_handle_t tx)
{
    if (!tx) {
        return ESP_ERR_INVALID_ARG;
    }
    
    free(tx);
    ESP_LOGD(TAG, "Transaction destroyed");
    return ESP_OK;
}

esp_err_t espsol_tx_reset(espsol_tx_handle_t tx)
{
    if (!tx) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(tx, 0, sizeof(struct espsol_transaction));
    ESP_LOGD(TAG, "Transaction reset");
    return ESP_OK;
}

/* ============================================================================
 * Transaction Configuration
 * ========================================================================== */

esp_err_t espsol_tx_set_fee_payer(espsol_tx_handle_t tx,
                                   const uint8_t pubkey[ESPSOL_PUBKEY_SIZE])
{
    if (!tx || !pubkey) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(tx->fee_payer, pubkey, ESPSOL_PUBKEY_SIZE);
    tx->has_fee_payer = true;
    tx->accounts_compiled = false;  /* Need to recompile */
    tx->is_signed = false;
    
    return ESP_OK;
}

esp_err_t espsol_tx_set_recent_blockhash(espsol_tx_handle_t tx,
                                          const uint8_t blockhash[ESPSOL_BLOCKHASH_SIZE])
{
    if (!tx || !blockhash) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(tx->blockhash, blockhash, ESPSOL_BLOCKHASH_SIZE);
    tx->has_blockhash = true;
    tx->is_signed = false;  /* Blockhash change invalidates signatures */
    
    return ESP_OK;
}

/* ============================================================================
 * Built-in Instructions
 * ========================================================================== */

esp_err_t espsol_tx_add_transfer(espsol_tx_handle_t tx,
                                  const uint8_t from[ESPSOL_PUBKEY_SIZE],
                                  const uint8_t to[ESPSOL_PUBKEY_SIZE],
                                  uint64_t lamports)
{
    if (!tx || !from || !to) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (tx->instruction_count >= ESPSOL_MAX_INSTRUCTIONS) {
        ESP_LOGE(TAG, "Maximum instructions reached");
        return ESP_ERR_ESPSOL_MAX_INSTRUCTIONS;
    }
    
    espsol_instruction_t *ix = &tx->instructions[tx->instruction_count];
    
    /* System Program ID */
    memcpy(ix->program_id, ESPSOL_SYSTEM_PROGRAM_ID, ESPSOL_PUBKEY_SIZE);
    
    /* Accounts: [from (signer, writable), to (writable)] */
    ix->account_count = 2;
    memcpy(ix->accounts[0].pubkey, from, ESPSOL_PUBKEY_SIZE);
    ix->accounts[0].is_signer = true;
    ix->accounts[0].is_writable = true;
    
    memcpy(ix->accounts[1].pubkey, to, ESPSOL_PUBKEY_SIZE);
    ix->accounts[1].is_signer = false;
    ix->accounts[1].is_writable = true;
    
    /* Instruction data: [u32 instruction_type (2=Transfer), u64 lamports] */
    ix->data_len = 12;
    /* Transfer instruction type = 2 (little-endian u32) */
    ix->data[0] = 2;
    ix->data[1] = 0;
    ix->data[2] = 0;
    ix->data[3] = 0;
    /* Lamports (little-endian u64) */
    for (int i = 0; i < 8; i++) {
        ix->data[4 + i] = (uint8_t)(lamports >> (i * 8));
    }
    
    tx->instruction_count++;
    tx->accounts_compiled = false;
    tx->is_signed = false;
    
    ESP_LOGD(TAG, "Added transfer instruction: %llu lamports", (unsigned long long)lamports);
    return ESP_OK;
}

esp_err_t espsol_tx_add_create_account(espsol_tx_handle_t tx,
                                        const uint8_t from[ESPSOL_PUBKEY_SIZE],
                                        const uint8_t new_account[ESPSOL_PUBKEY_SIZE],
                                        uint64_t lamports,
                                        uint64_t space,
                                        const uint8_t owner[ESPSOL_PUBKEY_SIZE])
{
    if (!tx || !from || !new_account || !owner) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (tx->instruction_count >= ESPSOL_MAX_INSTRUCTIONS) {
        return ESP_ERR_ESPSOL_MAX_INSTRUCTIONS;
    }
    
    espsol_instruction_t *ix = &tx->instructions[tx->instruction_count];
    
    /* System Program ID */
    memcpy(ix->program_id, ESPSOL_SYSTEM_PROGRAM_ID, ESPSOL_PUBKEY_SIZE);
    
    /* Accounts: [from (signer, writable), new_account (signer, writable)] */
    ix->account_count = 2;
    memcpy(ix->accounts[0].pubkey, from, ESPSOL_PUBKEY_SIZE);
    ix->accounts[0].is_signer = true;
    ix->accounts[0].is_writable = true;
    
    memcpy(ix->accounts[1].pubkey, new_account, ESPSOL_PUBKEY_SIZE);
    ix->accounts[1].is_signer = true;
    ix->accounts[1].is_writable = true;
    
    /* Instruction data: [u32 type (0=CreateAccount), u64 lamports, u64 space, pubkey owner] */
    ix->data_len = 4 + 8 + 8 + 32;  /* 52 bytes */
    
    /* CreateAccount instruction type = 0 */
    ix->data[0] = 0;
    ix->data[1] = 0;
    ix->data[2] = 0;
    ix->data[3] = 0;
    
    /* Lamports */
    for (int i = 0; i < 8; i++) {
        ix->data[4 + i] = (uint8_t)(lamports >> (i * 8));
    }
    
    /* Space */
    for (int i = 0; i < 8; i++) {
        ix->data[12 + i] = (uint8_t)(space >> (i * 8));
    }
    
    /* Owner */
    memcpy(ix->data + 20, owner, ESPSOL_PUBKEY_SIZE);
    
    tx->instruction_count++;
    tx->accounts_compiled = false;
    tx->is_signed = false;
    
    return ESP_OK;
}

/* ============================================================================
 * Custom Instructions
 * ========================================================================== */

esp_err_t espsol_tx_add_instruction(espsol_tx_handle_t tx,
                                     const uint8_t program_id[ESPSOL_PUBKEY_SIZE],
                                     const espsol_account_meta_t *accounts,
                                     size_t account_count,
                                     const uint8_t *data,
                                     size_t data_len)
{
    if (!tx || !program_id) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (account_count > 0 && !accounts) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (data_len > 0 && !data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (tx->instruction_count >= ESPSOL_MAX_INSTRUCTIONS) {
        return ESP_ERR_ESPSOL_MAX_INSTRUCTIONS;
    }
    
    if (account_count > ESPSOL_MAX_ACCOUNTS) {
        return ESP_ERR_ESPSOL_MAX_ACCOUNTS;
    }
    
    if (data_len > ESPSOL_MAX_INSTRUCTION_DATA) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    espsol_instruction_t *ix = &tx->instructions[tx->instruction_count];
    
    memcpy(ix->program_id, program_id, ESPSOL_PUBKEY_SIZE);
    
    ix->account_count = account_count;
    for (size_t i = 0; i < account_count; i++) {
        memcpy(ix->accounts[i].pubkey, accounts[i].pubkey, ESPSOL_PUBKEY_SIZE);
        ix->accounts[i].is_signer = accounts[i].is_signer;
        ix->accounts[i].is_writable = accounts[i].is_writable;
    }
    
    ix->data_len = data_len;
    if (data_len > 0) {
        memcpy(ix->data, data, data_len);
    }
    
    tx->instruction_count++;
    tx->accounts_compiled = false;
    tx->is_signed = false;
    
    return ESP_OK;
}

esp_err_t espsol_tx_add_memo(espsol_tx_handle_t tx, const char *memo)
{
    if (!tx || !memo) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t memo_len = strlen(memo);
    if (memo_len > ESPSOL_MAX_INSTRUCTION_DATA) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    /* Memo program takes no accounts, just data */
    return espsol_tx_add_instruction(tx, ESPSOL_MEMO_PROGRAM_ID,
                                      NULL, 0,
                                      (const uint8_t *)memo, memo_len);
}

/* ============================================================================
 * Signing
 * ========================================================================== */

esp_err_t espsol_tx_sign(espsol_tx_handle_t tx, const espsol_keypair_t *keypair)
{
    if (!tx || !keypair) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Compile accounts if needed */
    esp_err_t err = compile_accounts(tx);
    if (err != ESP_OK) {
        return err;
    }
    
    if (!tx->has_blockhash) {
        ESP_LOGE(TAG, "Cannot sign: missing blockhash");
        return ESP_ERR_ESPSOL_TX_BUILD_ERROR;
    }
    
    /* Serialize the message */
    uint8_t message[ESPSOL_MAX_TX_SIZE];
    size_t message_len = 0;
    err = serialize_message(tx, message, sizeof(message), &message_len);
    if (err != ESP_OK) {
        return err;
    }
    
    /* Find which signer this keypair corresponds to */
    int signer_idx = -1;
    for (size_t i = 0; i < tx->required_signers && i < tx->account_count; i++) {
        if (pubkey_equals(tx->accounts[i].pubkey, keypair->public_key)) {
            signer_idx = (int)i;
            break;
        }
    }
    
    if (signer_idx < 0) {
        ESP_LOGE(TAG, "Keypair public key not found in required signers");
        return ESP_ERR_ESPSOL_TX_BUILD_ERROR;
    }
    
    if ((size_t)signer_idx >= ESPSOL_MAX_SIGNERS) {
        ESP_LOGE(TAG, "Too many signers");
        return ESP_ERR_ESPSOL_MAX_ACCOUNTS;
    }
    
    /* Sign the message */
    err = espsol_sign(message, message_len, keypair, tx->signatures[signer_idx]);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to sign transaction");
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }
    
    /* Store the signer pubkey */
    memcpy(tx->signer_pubkeys[signer_idx], keypair->public_key, ESPSOL_PUBKEY_SIZE);
    
    /* Update signer count */
    if ((size_t)signer_idx >= tx->signer_count) {
        tx->signer_count = (size_t)signer_idx + 1;
    }
    
    /* Check if fully signed */
    tx->is_signed = (tx->signer_count >= tx->required_signers);
    
    ESP_LOGD(TAG, "Transaction signed by signer %d", signer_idx);
    return ESP_OK;
}

esp_err_t espsol_tx_sign_multiple(espsol_tx_handle_t tx,
                                   const espsol_keypair_t **keypairs,
                                   size_t count)
{
    if (!tx || !keypairs || count == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    for (size_t i = 0; i < count; i++) {
        esp_err_t err = espsol_tx_sign(tx, keypairs[i]);
        if (err != ESP_OK) {
            return err;
        }
    }
    
    return ESP_OK;
}

/* ============================================================================
 * Serialization
 * ========================================================================== */

esp_err_t espsol_tx_serialize(espsol_tx_handle_t tx,
                               uint8_t *buffer, size_t buffer_len,
                               size_t *out_len)
{
    if (!tx || !buffer || !out_len) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!tx->is_signed) {
        ESP_LOGE(TAG, "Transaction not fully signed");
        return ESP_ERR_ESPSOL_TX_NOT_SIGNED;
    }
    
    size_t offset = 0;
    
    /* Signatures (compact array) */
    size_t compact_len = write_compact_u16(buffer + offset, (uint16_t)tx->required_signers);
    offset += compact_len;
    
    /* Write signatures in order */
    for (size_t i = 0; i < tx->required_signers; i++) {
        if (offset + ESPSOL_SIGNATURE_SIZE > buffer_len) {
            return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
        }
        memcpy(buffer + offset, tx->signatures[i], ESPSOL_SIGNATURE_SIZE);
        offset += ESPSOL_SIGNATURE_SIZE;
    }
    
    /* Message */
    size_t message_len = 0;
    esp_err_t err = serialize_message(tx, buffer + offset, buffer_len - offset, &message_len);
    if (err != ESP_OK) {
        return err;
    }
    offset += message_len;
    
    *out_len = offset;
    return ESP_OK;
}

esp_err_t espsol_tx_to_base64(espsol_tx_handle_t tx,
                               char *output, size_t output_len)
{
    if (!tx || !output || output_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t buffer[ESPSOL_MAX_TX_SIZE];
    size_t tx_len = 0;
    
    esp_err_t err = espsol_tx_serialize(tx, buffer, sizeof(buffer), &tx_len);
    if (err != ESP_OK) {
        return err;
    }
    
    return espsol_base64_encode(buffer, tx_len, output, output_len);
}

esp_err_t espsol_tx_to_base58(espsol_tx_handle_t tx,
                               char *output, size_t output_len)
{
    if (!tx || !output || output_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t buffer[ESPSOL_MAX_TX_SIZE];
    size_t tx_len = 0;
    
    esp_err_t err = espsol_tx_serialize(tx, buffer, sizeof(buffer), &tx_len);
    if (err != ESP_OK) {
        return err;
    }
    
    return espsol_base58_encode(buffer, tx_len, output, output_len);
}

/* ============================================================================
 * Transaction Inspection
 * ========================================================================== */

bool espsol_tx_is_signed(espsol_tx_handle_t tx)
{
    if (!tx) {
        return false;
    }
    return tx->is_signed;
}

esp_err_t espsol_tx_get_signature_count(espsol_tx_handle_t tx, size_t *count)
{
    if (!tx || !count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *count = tx->signer_count;
    return ESP_OK;
}

esp_err_t espsol_tx_get_signature(espsol_tx_handle_t tx,
                                   size_t index,
                                   uint8_t signature[ESPSOL_SIGNATURE_SIZE])
{
    if (!tx || !signature) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (index >= tx->signer_count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(signature, tx->signatures[index], ESPSOL_SIGNATURE_SIZE);
    return ESP_OK;
}

esp_err_t espsol_tx_get_signature_base58(espsol_tx_handle_t tx,
                                          char *output, size_t output_len)
{
    if (!tx || !output || output_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!tx->is_signed || tx->signer_count == 0) {
        return ESP_ERR_ESPSOL_TX_NOT_SIGNED;
    }
    
    return espsol_base58_encode(tx->signatures[0], ESPSOL_SIGNATURE_SIZE,
                                 output, output_len);
}

esp_err_t espsol_tx_get_instruction_count(espsol_tx_handle_t tx, size_t *count)
{
    if (!tx || !count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *count = tx->instruction_count;
    return ESP_OK;
}

esp_err_t espsol_tx_get_account_count(espsol_tx_handle_t tx, size_t *count)
{
    if (!tx || !count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Compile if needed to get accurate count */
    esp_err_t err = compile_accounts(tx);
    if (err != ESP_OK) {
        return err;
    }
    
    *count = tx->account_count;
    return ESP_OK;
}
