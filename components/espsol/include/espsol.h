/**
 * @file espsol.h
 * @brief ESPSOL - ESP-IDF Solana Component
 *
 * Main header file for the ESPSOL component. Include this file to access
 * all ESPSOL functionality.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#ifndef ESPSOL_H
#define ESPSOL_H

/* Core types and constants */
#include "espsol_types.h"

/* Utility functions (Base58, Base64, etc.) */
#include "espsol_utils.h"

/* Cryptographic operations (Ed25519 keypairs, signing) */
#include "espsol_crypto.h"

/* RPC client for Solana network communication */
#include "espsol_rpc.h"

/* Transaction building and serialization */
#include "espsol_tx.h"

/* SPL Token operations */
#include "espsol_token.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Core SDK Functions
 * ========================================================================== */

/**
 * @brief Initialize the ESPSOL component
 *
 * This function must be called before using any other ESPSOL functions.
 * It initializes internal state and validates the configuration.
 *
 * @param[in] config Configuration structure (can be NULL for defaults)
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if configuration is invalid
 *     - ESP_ERR_NO_MEM if memory allocation failed
 */
esp_err_t espsol_init(const espsol_config_t *config);

/**
 * @brief Deinitialize the ESPSOL component
 *
 * Frees all resources allocated by espsol_init().
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_ESPSOL_NOT_INITIALIZED if not initialized
 */
esp_err_t espsol_deinit(void);

/**
 * @brief Check if ESPSOL is initialized
 *
 * @return true if initialized, false otherwise
 */
bool espsol_is_initialized(void);

/**
 * @brief Get the ESPSOL version string
 *
 * @return Version string (e.g., "0.1.0")
 */
const char* espsol_get_version(void);

/**
 * @brief Get version components
 *
 * @param[out] major Major version number
 * @param[out] minor Minor version number
 * @param[out] patch Patch version number
 */
void espsol_get_version_numbers(uint8_t *major, uint8_t *minor, uint8_t *patch);

/**
 * @brief Convert an ESPSOL error code to a human-readable string
 *
 * @param[in] err Error code
 *
 * @return Human-readable error description
 */
const char* espsol_err_to_name(esp_err_t err);

/**
 * @brief Get the current configuration
 *
 * Returns a pointer to the current configuration. The pointer is valid
 * until espsol_deinit() is called.
 *
 * @return Pointer to configuration, or NULL if not initialized
 */
const espsol_config_t* espsol_get_config(void);

#ifdef __cplusplus
}
#endif

#endif /* ESPSOL_H */
