/**
 * @file espsol_ed25519.h
 * @brief Portable Ed25519 Implementation (TweetNaCl-derived)
 *
 * Minimal Ed25519 implementation for host testing when libsodium is unavailable.
 * Based on TweetNaCl, a compact, audited cryptographic library.
 *
 * This should only be used for testing. On ESP32, use libsodium.
 *
 * @copyright Public domain (TweetNaCl)
 */

#ifndef ESPSOL_ED25519_H
#define ESPSOL_ED25519_H

#include "espsol_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generate Ed25519 keypair from seed
 */
esp_err_t espsol_ed25519_keypair_from_seed(const uint8_t seed[32],
                                            uint8_t public_key[32],
                                            uint8_t private_key[64]);

/**
 * @brief Sign a message with Ed25519 (detached signature)
 */
esp_err_t espsol_ed25519_sign(const uint8_t *message, size_t message_len,
                               const uint8_t private_key[64],
                               uint8_t signature[64]);

/**
 * @brief Verify an Ed25519 signature
 */
esp_err_t espsol_ed25519_verify(const uint8_t *message, size_t message_len,
                                 const uint8_t signature[64],
                                 const uint8_t public_key[32]);

#ifdef __cplusplus
}
#endif

#endif /* ESPSOL_ED25519_H */
