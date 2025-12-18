/**
 * @file espsol_tx.h
 * @brief ESPSOL Transaction Building API
 *
 * This file provides the transaction building, signing, and serialization
 * interface for Solana transactions. Supports System Program transfers
 * and custom instructions.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#ifndef ESPSOL_TX_H
#define ESPSOL_TX_H

#include "espsol_types.h"
#include "espsol_crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Well-Known Program IDs
 * ========================================================================== */

/** @brief System Program ID (11111111111111111111111111111111) */
extern const uint8_t ESPSOL_SYSTEM_PROGRAM_ID[ESPSOL_PUBKEY_SIZE];

/** @brief SPL Token Program ID (TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA) */
extern const uint8_t ESPSOL_TOKEN_PROGRAM_ID[ESPSOL_PUBKEY_SIZE];

/** @brief SPL Associated Token Account Program ID */
extern const uint8_t ESPSOL_ASSOCIATED_TOKEN_PROGRAM_ID[ESPSOL_PUBKEY_SIZE];

/** @brief Memo Program ID (MemoSq4gqABAXKb96qnH8TysNcWxMyWCqXgDLGmfcHr) */
extern const uint8_t ESPSOL_MEMO_PROGRAM_ID[ESPSOL_PUBKEY_SIZE];

/* ============================================================================
 * Transaction Handle
 * ========================================================================== */

/**
 * @brief Opaque handle for a Solana transaction
 */
typedef struct espsol_transaction *espsol_tx_handle_t;

/* ============================================================================
 * Account Meta Structure
 * ========================================================================== */

/**
 * @brief Account metadata for transaction instructions
 */
typedef struct {
    uint8_t pubkey[ESPSOL_PUBKEY_SIZE];  /**< Account public key */
    bool is_signer;                       /**< Whether account must sign */
    bool is_writable;                     /**< Whether account is writable */
} espsol_account_meta_t;

/* ============================================================================
 * Transaction Lifecycle
 * ========================================================================== */

/**
 * @brief Create a new transaction
 *
 * @param[out] tx    Pointer to receive transaction handle
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if tx is NULL
 *     - ESP_ERR_NO_MEM if memory allocation fails
 */
esp_err_t espsol_tx_create(espsol_tx_handle_t *tx);

/**
 * @brief Destroy a transaction and free resources
 *
 * @param[in] tx     Transaction handle
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if tx is NULL
 */
esp_err_t espsol_tx_destroy(espsol_tx_handle_t tx);

/**
 * @brief Reset a transaction for reuse
 *
 * Clears all instructions, accounts, and signatures but keeps the handle valid.
 *
 * @param[in] tx     Transaction handle
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if tx is NULL
 */
esp_err_t espsol_tx_reset(espsol_tx_handle_t tx);

/* ============================================================================
 * Transaction Configuration
 * ========================================================================== */

/**
 * @brief Set the fee payer for the transaction
 *
 * The fee payer must be the first signer. This is typically the sender
 * of a transfer or the wallet paying for the transaction.
 *
 * @param[in] tx         Transaction handle
 * @param[in] pubkey     Fee payer public key (32 bytes)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if tx or pubkey is NULL
 */
esp_err_t espsol_tx_set_fee_payer(espsol_tx_handle_t tx,
                                   const uint8_t pubkey[ESPSOL_PUBKEY_SIZE]);

/**
 * @brief Set the recent blockhash
 *
 * The blockhash must be fetched from the network using espsol_rpc_get_latest_blockhash().
 * Transactions expire after approximately 2 minutes (150 blocks).
 *
 * @param[in] tx             Transaction handle
 * @param[in] blockhash      Recent blockhash (32 bytes)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if tx or blockhash is NULL
 */
esp_err_t espsol_tx_set_recent_blockhash(espsol_tx_handle_t tx,
                                          const uint8_t blockhash[ESPSOL_BLOCKHASH_SIZE]);

/* ============================================================================
 * Built-in Instructions (System Program)
 * ========================================================================== */

/**
 * @brief Add a SOL transfer instruction
 *
 * Creates a System Program transfer instruction to send lamports
 * from one account to another.
 *
 * @param[in] tx         Transaction handle
 * @param[in] from       Sender public key (must sign)
 * @param[in] to         Recipient public key
 * @param[in] lamports   Amount to transfer in lamports
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_MAX_INSTRUCTIONS if instruction limit reached
 */
esp_err_t espsol_tx_add_transfer(espsol_tx_handle_t tx,
                                  const uint8_t from[ESPSOL_PUBKEY_SIZE],
                                  const uint8_t to[ESPSOL_PUBKEY_SIZE],
                                  uint64_t lamports);

/**
 * @brief Add a create account instruction
 *
 * Creates a System Program instruction to create a new account.
 *
 * @param[in] tx         Transaction handle
 * @param[in] from       Funding account (must sign)
 * @param[in] new_account New account to create (must sign)
 * @param[in] lamports   Lamports to transfer to new account
 * @param[in] space      Space to allocate in bytes
 * @param[in] owner      Program owner of the new account
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_MAX_INSTRUCTIONS if instruction limit reached
 */
esp_err_t espsol_tx_add_create_account(espsol_tx_handle_t tx,
                                        const uint8_t from[ESPSOL_PUBKEY_SIZE],
                                        const uint8_t new_account[ESPSOL_PUBKEY_SIZE],
                                        uint64_t lamports,
                                        uint64_t space,
                                        const uint8_t owner[ESPSOL_PUBKEY_SIZE]);

/* ============================================================================
 * Custom Instructions
 * ========================================================================== */

/**
 * @brief Add a custom instruction
 *
 * For advanced users who need to interact with programs beyond System Program.
 *
 * @param[in] tx             Transaction handle
 * @param[in] program_id     Program to invoke
 * @param[in] accounts       Array of account metadata
 * @param[in] account_count  Number of accounts
 * @param[in] data           Instruction data (can be NULL if data_len is 0)
 * @param[in] data_len       Length of instruction data
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if required arguments are NULL
 *     - ESP_ERR_ESPSOL_MAX_INSTRUCTIONS if instruction limit reached
 *     - ESP_ERR_ESPSOL_MAX_ACCOUNTS if account limit reached
 */
esp_err_t espsol_tx_add_instruction(espsol_tx_handle_t tx,
                                     const uint8_t program_id[ESPSOL_PUBKEY_SIZE],
                                     const espsol_account_meta_t *accounts,
                                     size_t account_count,
                                     const uint8_t *data,
                                     size_t data_len);

/**
 * @brief Add a memo instruction
 *
 * Adds an arbitrary memo/message to the transaction using the Memo Program.
 *
 * @param[in] tx         Transaction handle
 * @param[in] memo       Memo string (UTF-8)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if tx or memo is NULL
 *     - ESP_ERR_ESPSOL_MAX_INSTRUCTIONS if instruction limit reached
 */
esp_err_t espsol_tx_add_memo(espsol_tx_handle_t tx, const char *memo);

/* ============================================================================
 * Signing
 * ========================================================================== */

/**
 * @brief Sign the transaction with a keypair
 *
 * Signs the transaction message with the provided keypair. The keypair's
 * public key must match one of the required signers in the transaction.
 *
 * @param[in] tx         Transaction handle
 * @param[in] keypair    Keypair to sign with
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if tx or keypair is NULL
 *     - ESP_ERR_ESPSOL_TX_BUILD_ERROR if transaction not properly configured
 *     - ESP_ERR_ESPSOL_CRYPTO_ERROR if signing fails
 */
esp_err_t espsol_tx_sign(espsol_tx_handle_t tx,
                          const espsol_keypair_t *keypair);

/**
 * @brief Sign the transaction with multiple keypairs
 *
 * For multi-signature transactions where multiple accounts must sign.
 *
 * @param[in] tx         Transaction handle
 * @param[in] keypairs   Array of keypairs
 * @param[in] count      Number of keypairs
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL or count is 0
 *     - ESP_ERR_ESPSOL_TX_BUILD_ERROR if transaction not properly configured
 */
esp_err_t espsol_tx_sign_multiple(espsol_tx_handle_t tx,
                                   const espsol_keypair_t **keypairs,
                                   size_t count);

/* ============================================================================
 * Serialization
 * ========================================================================== */

/**
 * @brief Serialize the transaction to binary format
 *
 * Serializes the signed transaction to Solana wire format.
 *
 * @param[in]  tx          Transaction handle
 * @param[out] buffer      Output buffer
 * @param[in]  buffer_len  Size of output buffer
 * @param[out] out_len     Actual serialized length
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer too small
 *     - ESP_ERR_ESPSOL_TX_NOT_SIGNED if transaction not signed
 */
esp_err_t espsol_tx_serialize(espsol_tx_handle_t tx,
                               uint8_t *buffer, size_t buffer_len,
                               size_t *out_len);

/**
 * @brief Serialize the transaction to Base64 for RPC submission
 *
 * @param[in]  tx          Transaction handle
 * @param[out] output      Output buffer for Base64 string
 * @param[in]  output_len  Size of output buffer
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer too small
 *     - ESP_ERR_ESPSOL_TX_NOT_SIGNED if transaction not signed
 */
esp_err_t espsol_tx_to_base64(espsol_tx_handle_t tx,
                               char *output, size_t output_len);

/**
 * @brief Serialize the transaction to Base58
 *
 * @param[in]  tx          Transaction handle
 * @param[out] output      Output buffer for Base58 string
 * @param[in]  output_len  Size of output buffer
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer too small
 *     - ESP_ERR_ESPSOL_TX_NOT_SIGNED if transaction not signed
 */
esp_err_t espsol_tx_to_base58(espsol_tx_handle_t tx,
                               char *output, size_t output_len);

/* ============================================================================
 * Transaction Inspection
 * ========================================================================== */

/**
 * @brief Check if the transaction is fully signed
 *
 * @param[in] tx     Transaction handle
 * @return true if all required signers have signed, false otherwise
 */
bool espsol_tx_is_signed(espsol_tx_handle_t tx);

/**
 * @brief Get the number of signatures
 *
 * @param[in]  tx     Transaction handle
 * @param[out] count  Pointer to receive signature count
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if tx or count is NULL
 */
esp_err_t espsol_tx_get_signature_count(espsol_tx_handle_t tx, size_t *count);

/**
 * @brief Get a signature from the transaction
 *
 * @param[in]  tx         Transaction handle
 * @param[in]  index      Signature index (0 = primary/fee payer signature)
 * @param[out] signature  Buffer to receive signature (64 bytes)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if tx or signature is NULL
 *     - ESP_ERR_INVALID_ARG if index is out of range
 */
esp_err_t espsol_tx_get_signature(espsol_tx_handle_t tx,
                                   size_t index,
                                   uint8_t signature[ESPSOL_SIGNATURE_SIZE]);

/**
 * @brief Get the primary signature as Base58 string
 *
 * The primary signature is the transaction ID on the network.
 *
 * @param[in]  tx         Transaction handle
 * @param[out] output     Buffer for Base58 signature
 * @param[in]  output_len Size of output buffer
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer too small
 *     - ESP_ERR_ESPSOL_TX_NOT_SIGNED if not signed
 */
esp_err_t espsol_tx_get_signature_base58(espsol_tx_handle_t tx,
                                          char *output, size_t output_len);

/**
 * @brief Get the number of instructions
 *
 * @param[in]  tx     Transaction handle
 * @param[out] count  Pointer to receive instruction count
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if tx or count is NULL
 */
esp_err_t espsol_tx_get_instruction_count(espsol_tx_handle_t tx, size_t *count);

/**
 * @brief Get the number of accounts
 *
 * @param[in]  tx     Transaction handle
 * @param[out] count  Pointer to receive account count
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if tx or count is NULL
 */
esp_err_t espsol_tx_get_account_count(espsol_tx_handle_t tx, size_t *count);

#ifdef __cplusplus
}
#endif

#endif /* ESPSOL_TX_H */
