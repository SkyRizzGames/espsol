/**
 * @file espsol_utils.h
 * @brief ESPSOL Utility Functions
 *
 * This file contains utility functions for encoding/decoding and
 * common operations used throughout the ESPSOL component.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#ifndef ESPSOL_UTILS_H
#define ESPSOL_UTILS_H

#include "espsol_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Base58 Encoding/Decoding (Solana/Bitcoin format)
 * ========================================================================== */

/**
 * @brief Encode binary data to Base58 string
 *
 * Encodes binary data using the Base58 alphabet used by Solana and Bitcoin.
 *
 * @param[in]  data       Input binary data
 * @param[in]  data_len   Length of input data
 * @param[out] output     Output buffer for Base58 string (null-terminated)
 * @param[in]  output_len Size of output buffer
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if data or output is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if output buffer is too small
 */
esp_err_t espsol_base58_encode(const uint8_t *data, size_t data_len,
                                char *output, size_t output_len);

/**
 * @brief Decode Base58 string to binary data
 *
 * Decodes a Base58 string to binary data.
 *
 * @param[in]  input      Input Base58 string (null-terminated)
 * @param[out] output     Output buffer for decoded data
 * @param[in,out] output_len On input: size of output buffer
 *                           On output: actual decoded length
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if input or output is NULL
 *     - ESP_ERR_ESPSOL_INVALID_BASE58 if input contains invalid characters
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if output buffer is too small
 */
esp_err_t espsol_base58_decode(const char *input,
                                uint8_t *output, size_t *output_len);

/**
 * @brief Calculate the maximum encoded length for Base58
 *
 * @param[in] data_len Length of input data
 *
 * @return Maximum length of Base58 encoded string (including null terminator)
 */
size_t espsol_base58_encoded_len(size_t data_len);

/**
 * @brief Calculate the maximum decoded length for Base58
 *
 * @param[in] encoded_len Length of Base58 encoded string
 *
 * @return Maximum length of decoded binary data
 */
size_t espsol_base58_decoded_len(size_t encoded_len);

/* ============================================================================
 * Base64 Encoding/Decoding
 * ========================================================================== */

/**
 * @brief Encode binary data to Base64 string
 *
 * @param[in]  data       Input binary data
 * @param[in]  data_len   Length of input data
 * @param[out] output     Output buffer for Base64 string (null-terminated)
 * @param[in]  output_len Size of output buffer
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if data or output is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if output buffer is too small
 */
esp_err_t espsol_base64_encode(const uint8_t *data, size_t data_len,
                                char *output, size_t output_len);

/**
 * @brief Decode Base64 string to binary data
 *
 * @param[in]  input      Input Base64 string (null-terminated)
 * @param[out] output     Output buffer for decoded data
 * @param[in,out] output_len On input: size of output buffer
 *                           On output: actual decoded length
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if input or output is NULL
 *     - ESP_ERR_ESPSOL_INVALID_BASE64 if input contains invalid characters
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if output buffer is too small
 */
esp_err_t espsol_base64_decode(const char *input,
                                uint8_t *output, size_t *output_len);

/**
 * @brief Calculate the encoded length for Base64
 *
 * @param[in] data_len Length of input data
 *
 * @return Length of Base64 encoded string (including null terminator)
 */
size_t espsol_base64_encoded_len(size_t data_len);

/**
 * @brief Calculate the maximum decoded length for Base64
 *
 * @param[in] encoded_len Length of Base64 encoded string
 *
 * @return Maximum length of decoded binary data
 */
size_t espsol_base64_decoded_len(size_t encoded_len);

/* ============================================================================
 * Public Key / Address Utilities
 * ========================================================================== */

/**
 * @brief Convert a public key to Base58 address string
 *
 * @param[in]  pubkey  Public key bytes (32 bytes)
 * @param[out] address Output buffer for Base58 address
 * @param[in]  len     Size of output buffer (should be >= ESPSOL_ADDRESS_MAX_LEN)
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if pubkey or address is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer is too small
 */
esp_err_t espsol_pubkey_to_address(const uint8_t pubkey[ESPSOL_PUBKEY_SIZE],
                                    char *address, size_t len);

/**
 * @brief Convert a Base58 address string to public key bytes
 *
 * @param[in]  address Base58 address string
 * @param[out] pubkey  Output buffer for public key (32 bytes)
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if address or pubkey is NULL
 *     - ESP_ERR_ESPSOL_INVALID_BASE58 if address is invalid
 */
esp_err_t espsol_address_to_pubkey(const char *address,
                                    uint8_t pubkey[ESPSOL_PUBKEY_SIZE]);

/**
 * @brief Check if a string is a valid Solana address
 *
 * Validates that the string is a valid Base58 encoded 32-byte public key.
 *
 * @param[in] address Address string to validate
 *
 * @return true if valid, false otherwise
 */
bool espsol_is_valid_address(const char *address);

/* ============================================================================
 * Hex Encoding/Decoding (for debugging)
 * ========================================================================== */

/**
 * @brief Encode binary data to hexadecimal string
 *
 * @param[in]  data       Input binary data
 * @param[in]  data_len   Length of input data
 * @param[out] output     Output buffer for hex string (null-terminated)
 * @param[in]  output_len Size of output buffer (should be >= data_len * 2 + 1)
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if data or output is NULL
 *     - ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if output buffer is too small
 */
esp_err_t espsol_hex_encode(const uint8_t *data, size_t data_len,
                             char *output, size_t output_len);

/**
 * @brief Decode hexadecimal string to binary data
 *
 * @param[in]  input      Input hex string (null-terminated)
 * @param[out] output     Output buffer for decoded data
 * @param[in,out] output_len On input: size of output buffer
 *                           On output: actual decoded length
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if input or output is NULL
 *     - ESP_ERR_ESPSOL_ENCODING_FAILED if input contains invalid characters
 */
esp_err_t espsol_hex_decode(const char *input,
                             uint8_t *output, size_t *output_len);

#ifdef __cplusplus
}
#endif

#endif /* ESPSOL_UTILS_H */
