/**
 * @file espsol_rpc.h
 * @brief ESPSOL RPC Client API
 *
 * This file provides the JSON-RPC 2.0 client interface for communicating
 * with Solana RPC nodes. Uses native esp_http_client for HTTP requests
 * and cJSON for JSON parsing.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#ifndef ESPSOL_RPC_H
#define ESPSOL_RPC_H

#include "espsol_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * RPC Client Handle
 * ========================================================================== */

/**
 * @brief Opaque handle for RPC client connection
 */
typedef struct espsol_rpc_client *espsol_rpc_handle_t;

/* ============================================================================
 * Response Structures
 * ========================================================================== */

/**
 * @brief Account information structure
 */
typedef struct {
    char owner[ESPSOL_ADDRESS_MAX_LEN];  /**< Base58 owner program */
    uint64_t lamports;                    /**< Balance in lamports */
    uint8_t *data;                        /**< Account data (caller allocates) */
    size_t data_len;                      /**< Length of account data */
    size_t data_capacity;                 /**< Capacity of data buffer */
    bool executable;                      /**< Is the account executable */
    uint64_t rent_epoch;                  /**< Rent epoch */
} espsol_account_info_t;

/**
 * @brief Transaction response structure
 */
typedef struct {
    char signature[ESPSOL_SIGNATURE_MAX_LEN];  /**< Transaction signature (Base58) */
    uint64_t slot;                              /**< Slot the transaction was processed */
    bool confirmed;                             /**< Whether transaction is confirmed */
    char error[128];                            /**< Error message if any */
} espsol_tx_response_t;

/**
 * @brief Token account information structure
 */
typedef struct {
    char address[ESPSOL_ADDRESS_MAX_LEN];   /**< Token account address */
    char mint[ESPSOL_ADDRESS_MAX_LEN];      /**< Token mint address */
    char owner[ESPSOL_ADDRESS_MAX_LEN];     /**< Token account owner */
    uint64_t amount;                         /**< Token amount (raw) */
    uint8_t decimals;                        /**< Token decimals */
} espsol_token_account_t;

/**
 * @brief RPC client configuration
 */
typedef struct {
    const char *endpoint;             /**< RPC endpoint URL */
    uint32_t timeout_ms;              /**< Request timeout in milliseconds */
    espsol_commitment_t commitment;   /**< Default commitment level */
    size_t buffer_size;               /**< HTTP response buffer size */
    uint8_t max_retries;              /**< Max retry attempts (0 = no retry) */
    uint32_t retry_delay_ms;          /**< Initial retry delay (doubles each attempt) */
} espsol_rpc_config_t;

/**
 * @brief Default RPC configuration initializer
 */
#define ESPSOL_RPC_CONFIG_DEFAULT() { \
    .endpoint = ESPSOL_DEVNET_RPC, \
    .timeout_ms = ESPSOL_DEFAULT_TIMEOUT_MS, \
    .commitment = ESPSOL_COMMITMENT_CONFIRMED, \
    .buffer_size = ESPSOL_DEFAULT_BUFFER_SIZE, \
    .max_retries = 3, \
    .retry_delay_ms = 500 \
}

/* ============================================================================
 * Connection Management
 * ========================================================================== */

/**
 * @brief Initialize RPC client with default configuration
 *
 * @param[out] handle      Pointer to receive RPC client handle
 * @param[in]  endpoint    RPC endpoint URL (e.g., ESPSOL_DEVNET_RPC)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle or endpoint is NULL
 *     - ESP_ERR_NO_MEM if memory allocation fails
 */
esp_err_t espsol_rpc_init(espsol_rpc_handle_t *handle, const char *endpoint);

/**
 * @brief Initialize RPC client with custom configuration
 *
 * @param[out] handle      Pointer to receive RPC client handle
 * @param[in]  config      RPC configuration
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle or config is NULL
 *     - ESP_ERR_NO_MEM if memory allocation fails
 */
esp_err_t espsol_rpc_init_with_config(espsol_rpc_handle_t *handle, 
                                       const espsol_rpc_config_t *config);

/**
 * @brief Deinitialize RPC client and free resources
 *
 * @param[in] handle       RPC client handle
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle is NULL
 */
esp_err_t espsol_rpc_deinit(espsol_rpc_handle_t handle);

/**
 * @brief Set request timeout
 *
 * @param[in] handle       RPC client handle
 * @param[in] timeout_ms   Timeout in milliseconds
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle is NULL
 */
esp_err_t espsol_rpc_set_timeout(espsol_rpc_handle_t handle, uint32_t timeout_ms);

/**
 * @brief Set default commitment level
 *
 * @param[in] handle       RPC client handle
 * @param[in] commitment   Commitment level
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle is NULL
 */
esp_err_t espsol_rpc_set_commitment(espsol_rpc_handle_t handle, 
                                     espsol_commitment_t commitment);

/* ============================================================================
 * Network Information
 * ========================================================================== */

/**
 * @brief Get Solana node version
 *
 * @param[in]  handle      RPC client handle
 * @param[out] version     Buffer to receive version string
 * @param[in]  len         Size of version buffer
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle or version is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer too small
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 *     - ESP_ERR_ESPSOL_RPC_PARSE_ERROR on JSON parse error
 */
esp_err_t espsol_rpc_get_version(espsol_rpc_handle_t handle, 
                                  char *version, size_t len);

/**
 * @brief Get current slot
 *
 * @param[in]  handle      RPC client handle
 * @param[out] slot        Pointer to receive current slot
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle or slot is NULL
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_get_slot(espsol_rpc_handle_t handle, uint64_t *slot);

/**
 * @brief Get current block height
 *
 * @param[in]  handle      RPC client handle
 * @param[out] height      Pointer to receive block height
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle or height is NULL
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_get_block_height(espsol_rpc_handle_t handle, uint64_t *height);

/**
 * @brief Check if node is healthy
 *
 * @param[in]  handle      RPC client handle
 * @param[out] is_healthy  Pointer to receive health status
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle or is_healthy is NULL
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_get_health(espsol_rpc_handle_t handle, bool *is_healthy);

/* ============================================================================
 * Account Operations
 * ========================================================================== */

/**
 * @brief Get account balance in lamports
 *
 * @param[in]  handle      RPC client handle
 * @param[in]  pubkey      Base58-encoded public key
 * @param[out] lamports    Pointer to receive balance
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_get_balance(espsol_rpc_handle_t handle, 
                                  const char *pubkey, 
                                  uint64_t *lamports);

/**
 * @brief Get account information
 *
 * @param[in]  handle      RPC client handle
 * @param[in]  pubkey      Base58-encoded public key
 * @param[out] info        Pointer to account info structure
 *                         (caller must allocate data buffer if data_capacity > 0)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if data buffer too small
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_get_account_info(espsol_rpc_handle_t handle, 
                                       const char *pubkey, 
                                       espsol_account_info_t *info);

/* ============================================================================
 * Blockhash Operations
 * ========================================================================== */

/**
 * @brief Get latest blockhash
 *
 * @param[in]  handle                   RPC client handle
 * @param[out] blockhash                Buffer to receive blockhash (32 bytes)
 * @param[out] last_valid_block_height  Pointer to receive last valid block height (can be NULL)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle or blockhash is NULL
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_get_latest_blockhash(espsol_rpc_handle_t handle, 
                                           uint8_t blockhash[ESPSOL_BLOCKHASH_SIZE], 
                                           uint64_t *last_valid_block_height);

/**
 * @brief Get latest blockhash as Base58 string
 *
 * @param[in]  handle      RPC client handle
 * @param[out] blockhash   Buffer to receive Base58 blockhash
 * @param[in]  len         Size of blockhash buffer
 * @param[out] last_valid_block_height  Pointer to receive last valid block height (can be NULL)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle or blockhash is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer too small
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_get_latest_blockhash_str(espsol_rpc_handle_t handle,
                                               char *blockhash, size_t len,
                                               uint64_t *last_valid_block_height);

/* ============================================================================
 * Transaction Operations
 * ========================================================================== */

/**
 * @brief Send a signed transaction
 *
 * @param[in]  handle      RPC client handle
 * @param[in]  tx_base64   Base64-encoded signed transaction
 * @param[out] signature   Buffer to receive transaction signature (Base58)
 * @param[in]  sig_len     Size of signature buffer
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any required argument is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if signature buffer too small
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network/transaction error
 */
esp_err_t espsol_rpc_send_transaction(espsol_rpc_handle_t handle,
                                       const char *tx_base64,
                                       char *signature, size_t sig_len);

/**
 * @brief Get transaction details by signature
 *
 * @param[in]  handle      RPC client handle
 * @param[in]  signature   Transaction signature (Base58)
 * @param[out] response    Pointer to transaction response structure
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_get_transaction(espsol_rpc_handle_t handle,
                                      const char *signature,
                                      espsol_tx_response_t *response);

/**
 * @brief Wait for transaction confirmation
 *
 * @param[in]  handle      RPC client handle
 * @param[in]  signature   Transaction signature (Base58)
 * @param[in]  timeout_ms  Timeout in milliseconds
 * @param[out] confirmed   Pointer to receive confirmation status
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any required argument is NULL
 *     - ESP_ERR_ESPSOL_TIMEOUT if confirmation times out
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_confirm_transaction(espsol_rpc_handle_t handle,
                                          const char *signature,
                                          uint32_t timeout_ms,
                                          bool *confirmed);

/**
 * @brief Get signature statuses
 *
 * @param[in]  handle      RPC client handle
 * @param[in]  signatures  Array of transaction signatures (Base58)
 * @param[in]  count       Number of signatures
 * @param[out] confirmed   Array to receive confirmation status for each signature
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any argument is NULL or count is 0
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_get_signature_statuses(espsol_rpc_handle_t handle,
                                             const char **signatures,
                                             size_t count,
                                             bool *confirmed);

/* ============================================================================
 * Airdrop (devnet/testnet only)
 * ========================================================================== */

/**
 * @brief Request SOL airdrop (devnet/testnet only)
 *
 * @param[in]  handle      RPC client handle
 * @param[in]  pubkey      Base58-encoded public key to receive airdrop
 * @param[in]  lamports    Amount of lamports to request
 * @param[out] signature   Buffer to receive airdrop transaction signature
 * @param[in]  sig_len     Size of signature buffer
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if any required argument is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if signature buffer too small
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error or airdrop failure
 */
esp_err_t espsol_rpc_request_airdrop(espsol_rpc_handle_t handle,
                                      const char *pubkey,
                                      uint64_t lamports,
                                      char *signature, size_t sig_len);

/* ============================================================================
 * Token Operations (SPL Token)
 * ========================================================================== */

/**
 * @brief Get token accounts owned by a wallet
 *
 * @param[in]  handle      RPC client handle
 * @param[in]  owner       Base58-encoded wallet address
 * @param[in]  mint        Base58-encoded token mint (optional, NULL for all tokens)
 * @param[out] accounts    Array to receive token account info
 * @param[in,out] count    In: capacity of accounts array, Out: number of accounts found
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle, owner, accounts, or count is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if accounts array too small
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_get_token_accounts_by_owner(espsol_rpc_handle_t handle,
                                                   const char *owner,
                                                   const char *mint,
                                                   espsol_token_account_t *accounts,
                                                   size_t *count);

/**
 * @brief Get token account balance
 *
 * @param[in]  handle          RPC client handle
 * @param[in]  token_account   Base58-encoded token account address
 * @param[out] amount          Pointer to receive token amount (raw)
 * @param[out] decimals        Pointer to receive token decimals (can be NULL)
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle, token_account, or amount is NULL
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_get_token_balance(espsol_rpc_handle_t handle,
                                        const char *token_account,
                                        uint64_t *amount,
                                        uint8_t *decimals);

/* ============================================================================
 * Generic RPC Call
 * ========================================================================== */

/**
 * @brief Make a generic JSON-RPC call
 *
 * For advanced users who need to call RPC methods not covered by the API.
 *
 * @param[in]  handle          RPC client handle
 * @param[in]  method          RPC method name (e.g., "getMinimumBalanceForRentExemption")
 * @param[in]  params_json     JSON array of parameters (e.g., "[128]") or NULL
 * @param[out] response        Buffer to receive JSON response
 * @param[in]  response_len    Size of response buffer
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle, method, or response is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if response buffer too small
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_call(espsol_rpc_handle_t handle,
                           const char *method,
                           const char *params_json,
                           char *response, size_t response_len);

/**
 * @brief Get the last RPC error message
 *
 * @param[in]  handle      RPC client handle
 * @return Pointer to error message string, or NULL if no error
 */
const char *espsol_rpc_get_last_error(espsol_rpc_handle_t handle);

/* ============================================================================
 * Utility Functions
 * ========================================================================== */

/**
 * @brief Get commitment level as string
 *
 * @param[in] commitment   Commitment level
 * @return String representation ("processed", "confirmed", or "finalized")
 */
const char *espsol_commitment_to_str(espsol_commitment_t commitment);

/**
 * @brief Get minimum balance for rent exemption
 *
 * @param[in]  handle      RPC client handle
 * @param[in]  data_len    Account data length
 * @param[out] lamports    Pointer to receive minimum balance
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if handle or lamports is NULL
 *     - ESP_ERR_ESPSOL_RPC_FAILED on network error
 */
esp_err_t espsol_rpc_get_minimum_balance_for_rent_exemption(
    espsol_rpc_handle_t handle,
    size_t data_len,
    uint64_t *lamports);

#ifdef __cplusplus
}
#endif

#endif /* ESPSOL_RPC_H */
