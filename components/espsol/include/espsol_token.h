/**
 * @file espsol_token.h
 * @brief ESPSOL SPL Token API
 *
 * This file provides the SPL Token operations for token transfers,
 * account creation, and token balance queries.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#ifndef ESPSOL_TOKEN_H
#define ESPSOL_TOKEN_H

#include "espsol_types.h"
#include "espsol_tx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * SPL Token Constants
 * ========================================================================== */

/** @brief Token account data size (165 bytes) */
#define ESPSOL_TOKEN_ACCOUNT_SIZE   165

/** @brief Mint account data size (82 bytes) */
#define ESPSOL_MINT_ACCOUNT_SIZE    82

/** @brief Minimum lamports for rent exemption (token account) */
#define ESPSOL_TOKEN_ACCOUNT_RENT   2039280

/** @brief Token decimals for native SOL wrapped token */
#define ESPSOL_WSOL_DECIMALS        9

/* ============================================================================
 * SPL Token Instructions
 * ========================================================================== */

/**
 * @brief SPL Token instruction types
 */
typedef enum {
    ESPSOL_TOKEN_INIT_MINT = 0,
    ESPSOL_TOKEN_INIT_ACCOUNT = 1,
    ESPSOL_TOKEN_INIT_MULTISIG = 2,
    ESPSOL_TOKEN_TRANSFER = 3,
    ESPSOL_TOKEN_APPROVE = 4,
    ESPSOL_TOKEN_REVOKE = 5,
    ESPSOL_TOKEN_SET_AUTHORITY = 6,
    ESPSOL_TOKEN_MINT_TO = 7,
    ESPSOL_TOKEN_BURN = 8,
    ESPSOL_TOKEN_CLOSE_ACCOUNT = 9,
    ESPSOL_TOKEN_FREEZE_ACCOUNT = 10,
    ESPSOL_TOKEN_THAW_ACCOUNT = 11,
    ESPSOL_TOKEN_TRANSFER_CHECKED = 12,
    ESPSOL_TOKEN_APPROVE_CHECKED = 13,
    ESPSOL_TOKEN_MINT_TO_CHECKED = 14,
    ESPSOL_TOKEN_BURN_CHECKED = 15,
    ESPSOL_TOKEN_SYNC_NATIVE = 17,
} espsol_token_instruction_t;

/* ============================================================================
 * Associated Token Account (ATA)
 * ========================================================================== */

/**
 * @brief Derive the associated token account (ATA) address
 *
 * Computes the Program Derived Address (PDA) for an associated token account.
 *
 * @param[in]  wallet       Wallet owner public key (32 bytes)
 * @param[in]  mint         Token mint public key (32 bytes)
 * @param[out] ata_address  Output buffer for ATA address (32 bytes)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_CRYPTO_ERROR if PDA derivation fails
 */
esp_err_t espsol_token_get_ata_address(
    const uint8_t wallet[ESPSOL_PUBKEY_SIZE],
    const uint8_t mint[ESPSOL_PUBKEY_SIZE],
    uint8_t ata_address[ESPSOL_PUBKEY_SIZE]);

/**
 * @brief Derive a Program Derived Address (PDA)
 *
 * General-purpose PDA derivation for any program.
 *
 * @param[in]  seeds        Array of seed buffers
 * @param[in]  seed_lens    Array of seed lengths
 * @param[in]  seed_count   Number of seeds
 * @param[in]  program_id   Program ID (32 bytes)
 * @param[out] pda          Output buffer for PDA address (32 bytes)
 * @param[out] bump         Output bump seed (can be NULL if not needed)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if required arguments are NULL
 *     - ESP_ERR_ESPSOL_CRYPTO_ERROR if PDA derivation fails
 */
esp_err_t espsol_token_find_pda(
    const uint8_t **seeds,
    const size_t *seed_lens,
    size_t seed_count,
    const uint8_t program_id[ESPSOL_PUBKEY_SIZE],
    uint8_t pda[ESPSOL_PUBKEY_SIZE],
    uint8_t *bump);

/* ============================================================================
 * Transaction Building - Token Instructions
 * ========================================================================== */

/**
 * @brief Add a Create Associated Token Account instruction
 *
 * Creates an associated token account for the specified wallet and token mint.
 * The ATA address is deterministically derived.
 *
 * @param[in] tx             Transaction handle
 * @param[in] payer          Fee payer (must sign, pays rent)
 * @param[in] wallet         Wallet that will own the token account
 * @param[in] mint           Token mint address
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_MAX_INSTRUCTIONS if instruction limit reached
 */
esp_err_t espsol_tx_add_create_ata(
    espsol_tx_handle_t tx,
    const uint8_t payer[ESPSOL_PUBKEY_SIZE],
    const uint8_t wallet[ESPSOL_PUBKEY_SIZE],
    const uint8_t mint[ESPSOL_PUBKEY_SIZE]);

/**
 * @brief Add a Create Associated Token Account (idempotent) instruction
 *
 * Same as espsol_tx_add_create_ata but will not fail if the account
 * already exists. This is the recommended version.
 *
 * @param[in] tx             Transaction handle
 * @param[in] payer          Fee payer (must sign, pays rent)
 * @param[in] wallet         Wallet that will own the token account
 * @param[in] mint           Token mint address
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_MAX_INSTRUCTIONS if instruction limit reached
 */
esp_err_t espsol_tx_add_create_ata_idempotent(
    espsol_tx_handle_t tx,
    const uint8_t payer[ESPSOL_PUBKEY_SIZE],
    const uint8_t wallet[ESPSOL_PUBKEY_SIZE],
    const uint8_t mint[ESPSOL_PUBKEY_SIZE]);

/**
 * @brief Add a SPL Token Transfer instruction
 *
 * Transfers tokens from one token account to another.
 *
 * @param[in] tx             Transaction handle
 * @param[in] source         Source token account
 * @param[in] dest           Destination token account
 * @param[in] owner          Owner of the source account (must sign)
 * @param[in] amount         Amount of tokens to transfer (in smallest units)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_MAX_INSTRUCTIONS if instruction limit reached
 */
esp_err_t espsol_tx_add_token_transfer(
    espsol_tx_handle_t tx,
    const uint8_t source[ESPSOL_PUBKEY_SIZE],
    const uint8_t dest[ESPSOL_PUBKEY_SIZE],
    const uint8_t owner[ESPSOL_PUBKEY_SIZE],
    uint64_t amount);

/**
 * @brief Add a SPL Token TransferChecked instruction
 *
 * Transfers tokens with decimal verification. This is the recommended
 * transfer method as it validates the expected decimals.
 *
 * @param[in] tx             Transaction handle
 * @param[in] source         Source token account
 * @param[in] mint           Token mint (for decimal validation)
 * @param[in] dest           Destination token account
 * @param[in] owner          Owner of the source account (must sign)
 * @param[in] amount         Amount of tokens to transfer (in smallest units)
 * @param[in] decimals       Expected token decimals
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_MAX_INSTRUCTIONS if instruction limit reached
 */
esp_err_t espsol_tx_add_token_transfer_checked(
    espsol_tx_handle_t tx,
    const uint8_t source[ESPSOL_PUBKEY_SIZE],
    const uint8_t mint[ESPSOL_PUBKEY_SIZE],
    const uint8_t dest[ESPSOL_PUBKEY_SIZE],
    const uint8_t owner[ESPSOL_PUBKEY_SIZE],
    uint64_t amount,
    uint8_t decimals);

/**
 * @brief Add a SPL Token MintTo instruction
 *
 * Mints new tokens to a token account.
 *
 * @param[in] tx             Transaction handle
 * @param[in] mint           Token mint
 * @param[in] dest           Destination token account
 * @param[in] mint_authority Mint authority (must sign)
 * @param[in] amount         Amount of tokens to mint
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_MAX_INSTRUCTIONS if instruction limit reached
 */
esp_err_t espsol_tx_add_token_mint_to(
    espsol_tx_handle_t tx,
    const uint8_t mint[ESPSOL_PUBKEY_SIZE],
    const uint8_t dest[ESPSOL_PUBKEY_SIZE],
    const uint8_t mint_authority[ESPSOL_PUBKEY_SIZE],
    uint64_t amount);

/**
 * @brief Add a SPL Token Burn instruction
 *
 * Burns tokens from a token account.
 *
 * @param[in] tx             Transaction handle
 * @param[in] account        Token account to burn from
 * @param[in] mint           Token mint
 * @param[in] owner          Owner of the token account (must sign)
 * @param[in] amount         Amount of tokens to burn
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_MAX_INSTRUCTIONS if instruction limit reached
 */
esp_err_t espsol_tx_add_token_burn(
    espsol_tx_handle_t tx,
    const uint8_t account[ESPSOL_PUBKEY_SIZE],
    const uint8_t mint[ESPSOL_PUBKEY_SIZE],
    const uint8_t owner[ESPSOL_PUBKEY_SIZE],
    uint64_t amount);

/**
 * @brief Add a SPL Token CloseAccount instruction
 *
 * Closes a token account and returns the rent to the destination.
 *
 * @param[in] tx             Transaction handle
 * @param[in] account        Token account to close
 * @param[in] dest           Destination for remaining lamports
 * @param[in] owner          Owner of the token account (must sign)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_MAX_INSTRUCTIONS if instruction limit reached
 */
esp_err_t espsol_tx_add_token_close_account(
    espsol_tx_handle_t tx,
    const uint8_t account[ESPSOL_PUBKEY_SIZE],
    const uint8_t dest[ESPSOL_PUBKEY_SIZE],
    const uint8_t owner[ESPSOL_PUBKEY_SIZE]);

/**
 * @brief Add a SyncNative instruction
 *
 * Synchronizes a wrapped SOL account to reflect the underlying lamports.
 *
 * @param[in] tx             Transaction handle
 * @param[in] account        Native SOL token account
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_MAX_INSTRUCTIONS if instruction limit reached
 */
esp_err_t espsol_tx_add_token_sync_native(
    espsol_tx_handle_t tx,
    const uint8_t account[ESPSOL_PUBKEY_SIZE]);

/* ============================================================================
 * Utility Functions
 * ========================================================================== */

/**
 * @brief Check if a public key is on the Ed25519 curve
 *
 * PDAs are not on the curve. This can be used to verify if an address
 * is a regular pubkey or a PDA.
 *
 * @param[in] pubkey     Public key to check (32 bytes)
 * @return true if on curve (regular pubkey), false if off curve (PDA)
 */
bool espsol_is_on_curve(const uint8_t pubkey[ESPSOL_PUBKEY_SIZE]);

/**
 * @brief Get Wrapped SOL mint address
 *
 * Returns the well-known address for wrapped SOL (native mint).
 *
 * @param[out] mint      Output buffer for mint address (32 bytes)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if mint is NULL
 */
esp_err_t espsol_get_native_mint(uint8_t mint[ESPSOL_PUBKEY_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* ESPSOL_TOKEN_H */
