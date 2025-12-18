/**
 * @file espsol_crypto.h
 * @brief ESPSOL Cryptographic Operations
 *
 * This file provides Ed25519 cryptographic operations for Solana:
 * - Keypair generation (random and from seed)
 * - Message signing (detached signatures)
 * - Signature verification
 * - Key import/export
 *
 * Uses mbedTLS (ESP-IDF native) on ESP32, or libsodium on host.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#ifndef ESPSOL_CRYPTO_H
#define ESPSOL_CRYPTO_H

#include "espsol_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Keypair Structure
 * ========================================================================== */

/**
 * @brief Solana keypair structure
 *
 * Contains Ed25519 public and private keys.
 * The private key includes the 32-byte seed followed by the 32-byte public key.
 */
typedef struct {
    uint8_t public_key[ESPSOL_PUBKEY_SIZE];     /**< Ed25519 public key (32 bytes) */
    uint8_t private_key[ESPSOL_PRIVKEY_SIZE];   /**< Ed25519 private key (64 bytes) */
    bool initialized;                            /**< Whether keypair is valid */
} espsol_keypair_t;

/* ============================================================================
 * Initialization
 * ========================================================================== */

/**
 * @brief Initialize the crypto subsystem
 *
 * Must be called before any other crypto operations.
 * On ESP32, this initializes the hardware RNG.
 *
 * @return ESP_OK on success
 */
esp_err_t espsol_crypto_init(void);

/* ============================================================================
 * Random Generation
 * ========================================================================== */

/**
 * @brief Generate cryptographically secure random bytes
 *
 * Uses ESP32 hardware RNG on device, or system RNG on host.
 *
 * @param[out] buffer   Output buffer for random bytes
 * @param[in]  len      Number of bytes to generate
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if buffer is NULL
 */
esp_err_t espsol_random_bytes(uint8_t *buffer, size_t len);

/**
 * @brief Generate a random seed for keypair generation
 *
 * @param[out] seed     Output buffer (32 bytes)
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if seed is NULL
 */
esp_err_t espsol_random_seed(uint8_t seed[ESPSOL_SEED_SIZE]);

/* ============================================================================
 * Keypair Generation
 * ========================================================================== */

/**
 * @brief Initialize a keypair structure (zeros all fields)
 *
 * @param[out] keypair  Keypair to initialize
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if keypair is NULL
 */
esp_err_t espsol_keypair_init(espsol_keypair_t *keypair);

/**
 * @brief Generate a new random Ed25519 keypair
 *
 * @param[out] keypair  Keypair structure to populate
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if keypair is NULL
 * @return ESP_ERR_ESPSOL_CRYPTO_ERROR if generation fails
 */
esp_err_t espsol_keypair_generate(espsol_keypair_t *keypair);

/**
 * @brief Generate keypair from a 32-byte seed
 *
 * The seed is deterministically expanded to produce the keypair.
 * Same seed always produces the same keypair.
 *
 * @param[in]  seed     32-byte seed
 * @param[out] keypair  Keypair structure to populate
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_ESPSOL_CRYPTO_ERROR if generation fails
 */
esp_err_t espsol_keypair_from_seed(const uint8_t seed[ESPSOL_SEED_SIZE],
                                    espsol_keypair_t *keypair);

/**
 * @brief Clear a keypair (zero out sensitive data)
 *
 * Should be called when done with a keypair to prevent key leakage.
 *
 * @param[in,out] keypair  Keypair to clear
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if keypair is NULL
 */
esp_err_t espsol_keypair_clear(espsol_keypair_t *keypair);

/**
 * @brief Check if a keypair is initialized
 *
 * @param[in] keypair  Keypair to check
 *
 * @return true if keypair is initialized, false otherwise
 */
static inline bool espsol_keypair_is_initialized(const espsol_keypair_t *keypair)
{
    return keypair != NULL && keypair->initialized;
}

/* ============================================================================
 * Key Import/Export
 * ========================================================================== */

/**
 * @brief Import keypair from 64-byte private key bytes
 *
 * The private key format is: [32-byte seed | 32-byte public key]
 *
 * @param[in]  private_key  64-byte private key
 * @param[out] keypair      Keypair structure to populate
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 */
esp_err_t espsol_keypair_from_private_key(const uint8_t private_key[ESPSOL_PRIVKEY_SIZE],
                                           espsol_keypair_t *keypair);

/**
 * @brief Import keypair from Base58-encoded private key
 *
 * Supports both 32-byte (seed only) and 64-byte (full) formats.
 *
 * @param[in]  base58_key   Base58-encoded private key string
 * @param[out] keypair      Keypair structure to populate
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_ESPSOL_INVALID_BASE58 if decoding fails
 */
esp_err_t espsol_keypair_from_base58(const char *base58_key,
                                      espsol_keypair_t *keypair);

/**
 * @brief Export private key to Base58 string
 *
 * Exports the full 64-byte private key as Base58.
 *
 * @param[in]  keypair      Keypair to export
 * @param[out] output       Output buffer for Base58 string
 * @param[in]  output_len   Size of output buffer
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT if keypair not initialized
 * @return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer too small
 */
esp_err_t espsol_keypair_to_base58(const espsol_keypair_t *keypair,
                                    char *output, size_t output_len);

/**
 * @brief Get public key address as Base58 string
 *
 * @param[in]  keypair      Keypair to get address from
 * @param[out] address      Output buffer for address string
 * @param[in]  address_len  Size of address buffer
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT if keypair not initialized
 * @return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer too small
 */
esp_err_t espsol_keypair_get_address(const espsol_keypair_t *keypair,
                                      char *address, size_t address_len);

/* ============================================================================
 * Signing Operations
 * ========================================================================== */

/**
 * @brief Sign a message with Ed25519
 *
 * Creates a detached 64-byte signature (signature only, not prepended to message).
 *
 * @param[in]  message      Message to sign
 * @param[in]  message_len  Length of message
 * @param[in]  keypair      Keypair to sign with
 * @param[out] signature    Output buffer for signature (64 bytes)
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT if keypair not initialized
 * @return ESP_ERR_ESPSOL_CRYPTO_ERROR if signing fails
 */
esp_err_t espsol_sign(const uint8_t *message, size_t message_len,
                       const espsol_keypair_t *keypair,
                       uint8_t signature[ESPSOL_SIGNATURE_SIZE]);

/**
 * @brief Sign a message using raw private key
 *
 * @param[in]  message      Message to sign
 * @param[in]  message_len  Length of message
 * @param[in]  private_key  64-byte private key
 * @param[out] signature    Output buffer for signature (64 bytes)
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_ESPSOL_CRYPTO_ERROR if signing fails
 */
esp_err_t espsol_sign_raw(const uint8_t *message, size_t message_len,
                           const uint8_t private_key[ESPSOL_PRIVKEY_SIZE],
                           uint8_t signature[ESPSOL_SIGNATURE_SIZE]);

/**
 * @brief Sign a string message (convenience function)
 *
 * @param[in]  message      Null-terminated string to sign
 * @param[in]  keypair      Keypair to sign with
 * @param[out] signature    Output buffer for signature (64 bytes)
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT if keypair not initialized
 * @return ESP_ERR_ESPSOL_CRYPTO_ERROR if signing fails
 */
esp_err_t espsol_sign_string(const char *message,
                              const espsol_keypair_t *keypair,
                              uint8_t signature[ESPSOL_SIGNATURE_SIZE]);

/* ============================================================================
 * Verification Operations
 * ========================================================================== */

/**
 * @brief Verify an Ed25519 signature
 *
 * @param[in] message      Original message
 * @param[in] message_len  Length of message
 * @param[in] signature    Signature to verify (64 bytes)
 * @param[in] public_key   Public key that signed the message (32 bytes)
 *
 * @return ESP_OK if signature is valid
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_ESPSOL_SIGNATURE_INVALID if signature is invalid
 */
esp_err_t espsol_verify(const uint8_t *message, size_t message_len,
                         const uint8_t signature[ESPSOL_SIGNATURE_SIZE],
                         const uint8_t public_key[ESPSOL_PUBKEY_SIZE]);

/**
 * @brief Verify signature using keypair's public key
 *
 * @param[in] message      Original message
 * @param[in] message_len  Length of message
 * @param[in] signature    Signature to verify (64 bytes)
 * @param[in] keypair      Keypair with public key
 *
 * @return ESP_OK if signature is valid
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT if keypair not initialized
 * @return ESP_ERR_ESPSOL_SIGNATURE_INVALID if signature is invalid
 */
esp_err_t espsol_verify_keypair(const uint8_t *message, size_t message_len,
                                 const uint8_t signature[ESPSOL_SIGNATURE_SIZE],
                                 const espsol_keypair_t *keypair);

/* ============================================================================
 * Utility Functions
 * ========================================================================== */

/**
 * @brief Extract public key from private key
 *
 * The last 32 bytes of the 64-byte private key are the public key.
 *
 * @param[in]  private_key  64-byte private key
 * @param[out] public_key   Output buffer for public key (32 bytes)
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 */
esp_err_t espsol_public_key_from_private(const uint8_t private_key[ESPSOL_PRIVKEY_SIZE],
                                          uint8_t public_key[ESPSOL_PUBKEY_SIZE]);

/**
 * @brief Test Ed25519 implementation with known test vectors
 *
 * Runs RFC 8032 test vectors to verify crypto implementation.
 *
 * @return ESP_OK if all tests pass
 * @return ESP_ERR_ESPSOL_CRYPTO_ERROR if any test fails
 */
esp_err_t espsol_crypto_self_test(void);

/**
 * @brief Print keypair address to log (for debugging)
 *
 * Logs the public key address using ESP_LOGI (on ESP32) or printf (on host).
 * This is a convenience function for debugging.
 *
 * @param[in] keypair  Keypair to print
 * @param[in] label    Label to prefix the address (can be NULL)
 */
void espsol_keypair_print_address(const espsol_keypair_t *keypair, const char *label);

/* ============================================================================
 * NVS Secure Storage (ESP32 only)
 * ========================================================================== */

/**
 * @brief Save keypair to NVS (Non-Volatile Storage)
 *
 * Stores the keypair securely in NVS. The private key is stored encrypted
 * if NVS encryption is enabled.
 *
 * @note This function is only available on ESP32 (ESP_PLATFORM).
 * @note NVS must be initialized before calling this function.
 *
 * @param[in] keypair    Keypair to save
 * @param[in] nvs_key    NVS key name (max 15 characters)
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT if keypair not initialized
 * @return ESP_ERR_NVS_* on NVS errors
 */
esp_err_t espsol_keypair_save_to_nvs(const espsol_keypair_t *keypair,
                                      const char *nvs_key);

/**
 * @brief Load keypair from NVS
 *
 * Loads a previously saved keypair from NVS.
 *
 * @note This function is only available on ESP32 (ESP_PLATFORM).
 * @note NVS must be initialized before calling this function.
 *
 * @param[in]  nvs_key   NVS key name
 * @param[out] keypair   Keypair structure to populate
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_NVS_NOT_FOUND if key doesn't exist
 * @return ESP_ERR_NVS_* on other NVS errors
 */
esp_err_t espsol_keypair_load_from_nvs(const char *nvs_key,
                                        espsol_keypair_t *keypair);

/**
 * @brief Delete keypair from NVS
 *
 * Removes a stored keypair from NVS.
 *
 * @note This function is only available on ESP32 (ESP_PLATFORM).
 *
 * @param[in] nvs_key    NVS key name to delete
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if nvs_key is NULL
 * @return ESP_ERR_NVS_NOT_FOUND if key doesn't exist
 * @return ESP_ERR_NVS_* on other NVS errors
 */
esp_err_t espsol_keypair_delete_from_nvs(const char *nvs_key);

/**
 * @brief Check if a keypair exists in NVS
 *
 * @param[in]  nvs_key   NVS key name to check
 * @param[out] exists    Set to true if keypair exists
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 */
esp_err_t espsol_keypair_exists_in_nvs(const char *nvs_key, bool *exists);

#ifdef __cplusplus
}
#endif

#endif /* ESPSOL_CRYPTO_H */
