/**
 * @file espsol_mnemonic.h
 * @brief ESPSOL BIP39 Mnemonic (Seed Phrase) Support
 *
 * This file provides BIP39 mnemonic seed phrase functionality:
 * - Generate 12/24 word mnemonic phrases
 * - Convert mnemonic to seed bytes
 * - Derive Solana keypairs from mnemonic
 * - Validate mnemonic phrases
 *
 * Solana uses BIP44 derivation path: m/44'/501'/0'/0'
 * with SLIP-0010 Ed25519 derivation.
 *
 * @note Mnemonic phrases are sensitive - treat them like private keys!
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#ifndef ESPSOL_MNEMONIC_H
#define ESPSOL_MNEMONIC_H

#include "espsol_types.h"
#include "espsol_crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Constants
 * ========================================================================== */

/** @brief Number of words in a 12-word mnemonic (128-bit entropy) */
#define ESPSOL_MNEMONIC_12_WORDS    12

/** @brief Number of words in a 24-word mnemonic (256-bit entropy) */
#define ESPSOL_MNEMONIC_24_WORDS    24

/** @brief Maximum length of a 12-word mnemonic string (including spaces and null) */
#define ESPSOL_MNEMONIC_12_MAX_LEN  128

/** @brief Maximum length of a 24-word mnemonic string (including spaces and null) */
#define ESPSOL_MNEMONIC_24_MAX_LEN  256

/** @brief Size of entropy for 12-word mnemonic (128 bits = 16 bytes) */
#define ESPSOL_ENTROPY_12_SIZE      16

/** @brief Size of entropy for 24-word mnemonic (256 bits = 32 bytes) */
#define ESPSOL_ENTROPY_24_SIZE      32

/** @brief Size of seed derived from mnemonic (64 bytes) */
#define ESPSOL_MNEMONIC_SEED_SIZE   64

/** @brief BIP39 wordlist size */
#define ESPSOL_BIP39_WORDLIST_SIZE  2048

/* ============================================================================
 * Mnemonic Generation
 * ========================================================================== */

/**
 * @brief Generate a new 12-word mnemonic phrase
 *
 * Creates a cryptographically random 12-word BIP39 mnemonic.
 * This provides 128 bits of entropy.
 *
 * @param[out] mnemonic     Output buffer for mnemonic string
 * @param[in]  mnemonic_len Size of mnemonic buffer (min ESPSOL_MNEMONIC_12_MAX_LEN)
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer too small
 * @return ESP_ERR_ESPSOL_CRYPTO_ERROR if random generation fails
 *
 * @code
 * char mnemonic[ESPSOL_MNEMONIC_12_MAX_LEN];
 * espsol_mnemonic_generate_12(mnemonic, sizeof(mnemonic));
 * printf("Your seed phrase: %s\n", mnemonic);
 * // Output: "abandon ability able about above absent absorb abstract absurd abuse access accident"
 * @endcode
 */
esp_err_t espsol_mnemonic_generate_12(char *mnemonic, size_t mnemonic_len);

/**
 * @brief Generate a new 24-word mnemonic phrase
 *
 * Creates a cryptographically random 24-word BIP39 mnemonic.
 * This provides 256 bits of entropy (recommended for high security).
 *
 * @param[out] mnemonic     Output buffer for mnemonic string
 * @param[in]  mnemonic_len Size of mnemonic buffer (min ESPSOL_MNEMONIC_24_MAX_LEN)
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL
 * @return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer too small
 * @return ESP_ERR_ESPSOL_CRYPTO_ERROR if random generation fails
 */
esp_err_t espsol_mnemonic_generate_24(char *mnemonic, size_t mnemonic_len);

/**
 * @brief Generate mnemonic from provided entropy
 *
 * Creates a BIP39 mnemonic from the given entropy bytes.
 * Useful for deterministic wallet generation or testing.
 *
 * @param[in]  entropy      Entropy bytes (16 or 32 bytes)
 * @param[in]  entropy_len  Length of entropy (16 for 12 words, 32 for 24 words)
 * @param[out] mnemonic     Output buffer for mnemonic string
 * @param[in]  mnemonic_len Size of mnemonic buffer
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL or invalid entropy length
 * @return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer too small
 */
esp_err_t espsol_mnemonic_from_entropy(const uint8_t *entropy, size_t entropy_len,
                                        char *mnemonic, size_t mnemonic_len);

/* ============================================================================
 * Mnemonic Validation
 * ========================================================================== */

/**
 * @brief Validate a mnemonic phrase
 *
 * Checks that:
 * - All words are in the BIP39 wordlist
 * - Word count is valid (12 or 24)
 * - Checksum is correct
 *
 * @param[in] mnemonic  Mnemonic string to validate
 *
 * @return ESP_OK if mnemonic is valid
 * @return ESP_ERR_INVALID_ARG if mnemonic is NULL
 * @return ESP_ERR_ESPSOL_INVALID_MNEMONIC if mnemonic is invalid
 */
esp_err_t espsol_mnemonic_validate(const char *mnemonic);

/**
 * @brief Check if a word is in the BIP39 wordlist
 *
 * @param[in]  word   Word to check
 * @param[out] index  Optional output for word index (0-2047)
 *
 * @return true if word is valid, false otherwise
 */
bool espsol_mnemonic_word_valid(const char *word, int *index);

/**
 * @brief Get word count from mnemonic string
 *
 * @param[in] mnemonic  Mnemonic string
 *
 * @return Number of words (12 or 24 for valid mnemonics, 0 on error)
 */
int espsol_mnemonic_word_count(const char *mnemonic);

/* ============================================================================
 * Seed Derivation
 * ========================================================================== */

/**
 * @brief Derive seed bytes from mnemonic (BIP39)
 *
 * Converts mnemonic to 64-byte seed using PBKDF2-SHA512.
 * Optional passphrase provides additional security.
 *
 * @param[in]  mnemonic     Mnemonic phrase string
 * @param[in]  passphrase   Optional passphrase (can be NULL or empty)
 * @param[out] seed         Output buffer for 64-byte seed
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if mnemonic or seed is NULL
 * @return ESP_ERR_ESPSOL_INVALID_MNEMONIC if mnemonic is invalid
 *
 * @note The passphrase is optional but provides extra security.
 *       Different passphrases produce different wallets from the same mnemonic.
 */
esp_err_t espsol_mnemonic_to_seed(const char *mnemonic, const char *passphrase,
                                   uint8_t seed[ESPSOL_MNEMONIC_SEED_SIZE]);

/* ============================================================================
 * Keypair Derivation
 * ========================================================================== */

/**
 * @brief Generate keypair from mnemonic phrase
 *
 * Derives a Solana Ed25519 keypair from the mnemonic using:
 * - BIP39 seed derivation (mnemonic -> 64-byte seed)
 * - First 32 bytes of seed as Ed25519 seed
 *
 * This is compatible with Phantom, Solflare, and other Solana wallets
 * when using the default derivation path.
 *
 * @param[in]  mnemonic     Mnemonic phrase string (12 or 24 words)
 * @param[in]  passphrase   Optional BIP39 passphrase (can be NULL)
 * @param[out] keypair      Keypair structure to populate
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if mnemonic or keypair is NULL
 * @return ESP_ERR_ESPSOL_INVALID_MNEMONIC if mnemonic is invalid
 * @return ESP_ERR_ESPSOL_CRYPTO_ERROR if key derivation fails
 *
 * @code
 * // Generate new wallet with seed phrase
 * char mnemonic[ESPSOL_MNEMONIC_12_MAX_LEN];
 * espsol_mnemonic_generate_12(mnemonic, sizeof(mnemonic));
 * 
 * printf("=== BACKUP YOUR SEED PHRASE ===\n");
 * printf("%s\n", mnemonic);
 * printf("===============================\n");
 * 
 * espsol_keypair_t wallet;
 * espsol_keypair_from_mnemonic(mnemonic, NULL, &wallet);
 * 
 * char address[ESPSOL_ADDRESS_MAX_LEN];
 * espsol_keypair_get_address(&wallet, address, sizeof(address));
 * printf("Wallet address: %s\n", address);
 * @endcode
 */
esp_err_t espsol_keypair_from_mnemonic(const char *mnemonic, const char *passphrase,
                                        espsol_keypair_t *keypair);

/**
 * @brief Generate keypair and mnemonic together
 *
 * Convenience function that generates a new mnemonic and derives
 * the keypair in one call. Useful for wallet creation flows.
 *
 * @param[in]  word_count   Number of words (12 or 24)
 * @param[out] mnemonic     Output buffer for mnemonic string
 * @param[in]  mnemonic_len Size of mnemonic buffer
 * @param[out] keypair      Keypair structure to populate
 *
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if arguments are NULL or word_count invalid
 * @return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL if buffer too small
 * @return ESP_ERR_ESPSOL_CRYPTO_ERROR if generation fails
 */
esp_err_t espsol_keypair_generate_with_mnemonic(int word_count,
                                                  char *mnemonic, size_t mnemonic_len,
                                                  espsol_keypair_t *keypair);

/* ============================================================================
 * Utility Functions
 * ========================================================================== */

/**
 * @brief Get a word from the BIP39 wordlist by index
 *
 * @param[in] index  Word index (0-2047)
 *
 * @return Pointer to word string, or NULL if index out of range
 */
const char *espsol_mnemonic_get_word(int index);

/**
 * @brief Securely clear mnemonic from memory
 *
 * Overwrites the mnemonic buffer with zeros to prevent
 * sensitive data from remaining in memory.
 *
 * @param[in,out] mnemonic  Mnemonic buffer to clear
 * @param[in]     len       Length of buffer
 */
void espsol_mnemonic_clear(char *mnemonic, size_t len);

/**
 * @brief Print mnemonic words for backup
 *
 * Prints the mnemonic with word numbers for easy backup.
 * Uses ESP_LOGI on ESP32, printf on host.
 *
 * @param[in] mnemonic  Mnemonic phrase to print
 * @param[in] label     Label to print before words (can be NULL)
 *
 * @code
 * // Output:
 * // === YOUR SEED PHRASE ===
 * //  1. abandon
 * //  2. ability
 * //  3. able
 * // ... etc
 * @endcode
 */
void espsol_mnemonic_print(const char *mnemonic, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* ESPSOL_MNEMONIC_H */
