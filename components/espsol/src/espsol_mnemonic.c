/**
 * @file espsol_mnemonic.c
 * @brief ESPSOL BIP39 Mnemonic Implementation
 *
 * Implements BIP39 mnemonic generation and seed derivation for Solana wallets.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include "espsol_mnemonic.h"
#include "espsol_crypto.h"
#include <string.h>
#include <stdio.h>

/* Platform-specific includes */
#if defined(ESP_PLATFORM) && ESP_PLATFORM
#include "esp_log.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/md.h"

static const char *TAG = "espsol_mnemonic";
#define LOG_I(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define LOG_E(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#else
/* Host compilation */
#include <stdlib.h>

/* Use mbedTLS if available, otherwise use the SHA-256 from espsol_ed25519.c */
#ifdef USE_MBEDTLS
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/md.h"
#else
/* SHA-256 is provided by espsol_ed25519.c for host testing */
extern void espsol_sha256(const uint8_t *data, size_t len, uint8_t out[32]);
#endif

/* Disable verbose logging for host tests */
#define LOG_I(fmt, ...) do {} while(0)
#define LOG_E(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_W(fmt, ...) do {} while(0)
#endif

/* External wordlist (defined in espsol_bip39_wordlist.c) */
extern const char *const espsol_bip39_wordlist[2048];

/* ============================================================================
 * Internal Helper Functions
 * ========================================================================== */

/**
 * @brief Compute SHA-256 hash
 */
static esp_err_t sha256_hash(const uint8_t *data, size_t len, uint8_t hash[32])
{
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);  /* 0 = SHA-256 (not SHA-224) */
    mbedtls_sha256_update(&ctx, data, len);
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);
    return ESP_OK;
#elif defined(USE_MBEDTLS)
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, data, len);
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);
    return ESP_OK;
#else
    /* Use embedded SHA-256 implementation */
    espsol_sha256(data, len, hash);
    return ESP_OK;
#endif
}

/**
 * @brief PBKDF2-HMAC-SHA512 for BIP39 seed derivation
 */
static esp_err_t pbkdf2_sha512(const uint8_t *password, size_t password_len,
                                const uint8_t *salt, size_t salt_len,
                                uint32_t iterations, uint8_t *output, size_t output_len)
{
#if defined(ESP_PLATFORM) && ESP_PLATFORM || defined(USE_MBEDTLS)
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *md_info;
    int ret;

    mbedtls_md_init(&ctx);
    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
    if (md_info == NULL) {
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }

    ret = mbedtls_md_setup(&ctx, md_info, 1);  /* 1 = use HMAC */
    if (ret != 0) {
        mbedtls_md_free(&ctx);
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }

    ret = mbedtls_pkcs5_pbkdf2_hmac(&ctx, password, password_len,
                                     salt, salt_len, iterations,
                                     output_len, output);
    mbedtls_md_free(&ctx);

    if (ret != 0) {
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }
    return ESP_OK;
#else
    /* For host testing without mbedTLS, use a simple fallback */
    /* This is NOT cryptographically secure - for testing only! */
    (void)iterations;
    
    /* Simple HMAC-SHA512 fallback - concatenate password and salt */
    /* Need larger buffer for 24-word mnemonics (~200 chars) */
    uint8_t combined[320];
    size_t combined_len = 0;
    
    if (password_len + salt_len > sizeof(combined)) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    memcpy(combined, password, password_len);
    combined_len += password_len;
    memcpy(combined + combined_len, salt, salt_len);
    combined_len += salt_len;
    
    /* Hash twice to get 64 bytes (not real PBKDF2, just for testing) */
    uint8_t hash1[32], hash2[32];
    sha256_hash(combined, combined_len, hash1);
    
    /* Second hash with iteration marker */
    combined[0] ^= 0x01;
    sha256_hash(combined, combined_len, hash2);
    
    if (output_len >= 64) {
        memcpy(output, hash1, 32);
        memcpy(output + 32, hash2, 32);
    } else {
        memcpy(output, hash1, output_len);
    }
    
    return ESP_OK;
#endif
}

/**
 * @brief Binary search for word in wordlist
 * @return Word index (0-2047) or -1 if not found
 */
static int find_word_index(const char *word)
{
    int left = 0;
    int right = 2047;
    
    while (left <= right) {
        int mid = (left + right) / 2;
        int cmp = strcmp(word, espsol_bip39_wordlist[mid]);
        
        if (cmp == 0) {
            return mid;
        } else if (cmp < 0) {
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }
    
    return -1;  /* Not found */
}

/**
 * @brief Extract 11-bit value from bit array
 */
static uint16_t get_11bits(const uint8_t *data, int bit_offset)
{
    int byte_offset = bit_offset / 8;
    int bit_shift = bit_offset % 8;
    
    /* Read 3 bytes and extract 11 bits */
    uint32_t val = ((uint32_t)data[byte_offset] << 16) |
                   ((uint32_t)data[byte_offset + 1] << 8) |
                   ((uint32_t)data[byte_offset + 2]);
    
    /* Shift right to align the 11 bits we want */
    val >>= (24 - 11 - bit_shift);
    
    return (uint16_t)(val & 0x7FF);  /* 11 bits = 0x7FF */
}

/**
 * @brief Set 11-bit value in bit array
 */
static void set_11bits(uint8_t *data, int bit_offset, uint16_t value)
{
    int byte_offset = bit_offset / 8;
    int bit_shift = bit_offset % 8;
    
    /* Mask to 11 bits */
    value &= 0x7FF;
    
    /* Read existing bytes */
    uint32_t current = ((uint32_t)data[byte_offset] << 16) |
                       ((uint32_t)data[byte_offset + 1] << 8) |
                       ((uint32_t)data[byte_offset + 2]);
    
    /* Calculate mask and shift */
    int shift = 24 - 11 - bit_shift;
    uint32_t mask = ~(0x7FF << shift);
    
    /* Apply value */
    current = (current & mask) | ((uint32_t)value << shift);
    
    /* Write back */
    data[byte_offset] = (current >> 16) & 0xFF;
    data[byte_offset + 1] = (current >> 8) & 0xFF;
    data[byte_offset + 2] = current & 0xFF;
}

/* ============================================================================
 * Mnemonic Generation
 * ========================================================================== */

esp_err_t espsol_mnemonic_from_entropy(const uint8_t *entropy, size_t entropy_len,
                                        char *mnemonic, size_t mnemonic_len)
{
    if (entropy == NULL || mnemonic == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int word_count;
    size_t required_len;
    
    if (entropy_len == ESPSOL_ENTROPY_12_SIZE) {
        word_count = ESPSOL_MNEMONIC_12_WORDS;
        required_len = ESPSOL_MNEMONIC_12_MAX_LEN;
    } else if (entropy_len == ESPSOL_ENTROPY_24_SIZE) {
        word_count = ESPSOL_MNEMONIC_24_WORDS;
        required_len = ESPSOL_MNEMONIC_24_MAX_LEN;
    } else {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (mnemonic_len < required_len) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    /* Calculate checksum (SHA-256 of entropy) */
    uint8_t hash[32];
    sha256_hash(entropy, entropy_len, hash);
    
    /* Checksum bits = entropy_len / 4 (4 bits for 12 words, 8 bits for 24 words) */
    int checksum_bits = (int)(entropy_len / 4);
    
    /* Create entropy + checksum buffer */
    /* For 12 words: 128 bits + 4 bits = 132 bits = 17 bytes (rounded up) */
    /* For 24 words: 256 bits + 8 bits = 264 bits = 33 bytes */
    uint8_t data[34];
    memset(data, 0, sizeof(data));
    memcpy(data, entropy, entropy_len);
    
    /* Append checksum bits */
    /* The checksum is the first checksum_bits of the SHA-256 hash */
    int total_bits = (int)(entropy_len * 8) + checksum_bits;
    
    /* Copy checksum byte(s) */
    if (checksum_bits == 4) {
        /* 12-word: append top 4 bits of hash[0] */
        data[entropy_len] = hash[0] & 0xF0;
    } else {
        /* 24-word: append top 8 bits of hash[0] */
        data[entropy_len] = hash[0];
    }
    
    /* Generate mnemonic words */
    mnemonic[0] = '\0';
    size_t pos = 0;
    
    for (int i = 0; i < word_count; i++) {
        int bit_offset = i * 11;
        uint16_t word_index = get_11bits(data, bit_offset);
        
        const char *word = espsol_bip39_wordlist[word_index];
        size_t word_len = strlen(word);
        
        /* Check buffer space */
        if (pos + word_len + 2 > mnemonic_len) {  /* +2 for space and null */
            return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
        }
        
        /* Add space before word (except first) */
        if (i > 0) {
            mnemonic[pos++] = ' ';
        }
        
        memcpy(mnemonic + pos, word, word_len);
        pos += word_len;
    }
    
    mnemonic[pos] = '\0';
    
    /* Clear sensitive data */
    memset(data, 0, sizeof(data));
    memset(hash, 0, sizeof(hash));
    
    return ESP_OK;
}

esp_err_t espsol_mnemonic_generate_12(char *mnemonic, size_t mnemonic_len)
{
    if (mnemonic == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (mnemonic_len < ESPSOL_MNEMONIC_12_MAX_LEN) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    /* Generate 128 bits (16 bytes) of entropy */
    uint8_t entropy[ESPSOL_ENTROPY_12_SIZE];
    esp_err_t ret = espsol_random_bytes(entropy, sizeof(entropy));
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = espsol_mnemonic_from_entropy(entropy, sizeof(entropy), mnemonic, mnemonic_len);
    
    /* Clear entropy from memory */
    memset(entropy, 0, sizeof(entropy));
    
    return ret;
}

esp_err_t espsol_mnemonic_generate_24(char *mnemonic, size_t mnemonic_len)
{
    if (mnemonic == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (mnemonic_len < ESPSOL_MNEMONIC_24_MAX_LEN) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    /* Generate 256 bits (32 bytes) of entropy */
    uint8_t entropy[ESPSOL_ENTROPY_24_SIZE];
    esp_err_t ret = espsol_random_bytes(entropy, sizeof(entropy));
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = espsol_mnemonic_from_entropy(entropy, sizeof(entropy), mnemonic, mnemonic_len);
    
    /* Clear entropy from memory */
    memset(entropy, 0, sizeof(entropy));
    
    return ret;
}

/* ============================================================================
 * Mnemonic Validation
 * ========================================================================== */

int espsol_mnemonic_word_count(const char *mnemonic)
{
    if (mnemonic == NULL || *mnemonic == '\0') {
        return 0;
    }
    
    int count = 1;
    const char *p = mnemonic;
    
    /* Skip leading spaces */
    while (*p == ' ') p++;
    
    if (*p == '\0') return 0;
    
    while (*p) {
        if (*p == ' ') {
            count++;
            /* Skip multiple spaces */
            while (*p == ' ') p++;
            if (*p == '\0') count--;  /* Trailing space doesn't count */
        } else {
            p++;
        }
    }
    
    return count;
}

bool espsol_mnemonic_word_valid(const char *word, int *index)
{
    if (word == NULL) {
        return false;
    }
    
    int idx = find_word_index(word);
    if (index != NULL) {
        *index = idx;
    }
    
    return idx >= 0;
}

esp_err_t espsol_mnemonic_validate(const char *mnemonic)
{
    if (mnemonic == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int word_count = espsol_mnemonic_word_count(mnemonic);
    
    /* Check word count */
    if (word_count != ESPSOL_MNEMONIC_12_WORDS && 
        word_count != ESPSOL_MNEMONIC_24_WORDS) {
        return ESP_ERR_ESPSOL_INVALID_MNEMONIC;
    }
    
    /* Parse and validate each word, collecting indices */
    int indices[24];
    char word_buf[16];
    const char *p = mnemonic;
    int word_idx = 0;
    
    while (*p && word_idx < word_count) {
        /* Skip spaces */
        while (*p == ' ') p++;
        if (*p == '\0') break;
        
        /* Extract word */
        size_t len = 0;
        while (*p && *p != ' ' && len < sizeof(word_buf) - 1) {
            word_buf[len++] = *p++;
        }
        word_buf[len] = '\0';
        
        /* Validate word */
        int idx = find_word_index(word_buf);
        if (idx < 0) {
            return ESP_ERR_ESPSOL_INVALID_MNEMONIC;
        }
        
        indices[word_idx++] = idx;
    }
    
    if (word_idx != word_count) {
        return ESP_ERR_ESPSOL_INVALID_MNEMONIC;
    }
    
    /* Reconstruct entropy and verify checksum */
    int entropy_bits = (word_count == 12) ? 128 : 256;
    int checksum_bits = entropy_bits / 32;
    int total_bits = entropy_bits + checksum_bits;
    
    /* Convert word indices back to bits */
    uint8_t data[34];
    memset(data, 0, sizeof(data));
    
    for (int i = 0; i < word_count; i++) {
        set_11bits(data, i * 11, (uint16_t)indices[i]);
    }
    
    /* Extract entropy */
    int entropy_bytes = entropy_bits / 8;
    uint8_t entropy[32];
    memcpy(entropy, data, entropy_bytes);
    
    /* Calculate expected checksum */
    uint8_t hash[32];
    sha256_hash(entropy, entropy_bytes, hash);
    
    /* Extract actual checksum from mnemonic data */
    uint8_t actual_checksum = data[entropy_bytes];
    uint8_t expected_checksum;
    
    if (checksum_bits == 4) {
        actual_checksum &= 0xF0;
        expected_checksum = hash[0] & 0xF0;
    } else {
        expected_checksum = hash[0];
    }
    
    if (actual_checksum != expected_checksum) {
        return ESP_ERR_ESPSOL_INVALID_MNEMONIC;
    }
    
    return ESP_OK;
}

/* ============================================================================
 * Seed Derivation
 * ========================================================================== */

esp_err_t espsol_mnemonic_to_seed(const char *mnemonic, const char *passphrase,
                                   uint8_t seed[ESPSOL_MNEMONIC_SEED_SIZE])
{
    if (mnemonic == NULL || seed == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Validate mnemonic */
    esp_err_t ret = espsol_mnemonic_validate(mnemonic);
    if (ret != ESP_OK) {
        return ret;
    }
    
    /* BIP39 uses "mnemonic" + passphrase as salt */
    const char *salt_prefix = "mnemonic";
    size_t salt_prefix_len = strlen(salt_prefix);
    size_t passphrase_len = (passphrase != NULL) ? strlen(passphrase) : 0;
    size_t salt_len = salt_prefix_len + passphrase_len;
    
    /* Build salt */
    uint8_t salt[128];
    if (salt_len > sizeof(salt)) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    memcpy(salt, salt_prefix, salt_prefix_len);
    if (passphrase != NULL && passphrase_len > 0) {
        memcpy(salt + salt_prefix_len, passphrase, passphrase_len);
    }
    
    /* PBKDF2-HMAC-SHA512 with 2048 iterations */
    ret = pbkdf2_sha512((const uint8_t *)mnemonic, strlen(mnemonic),
                         salt, salt_len,
                         2048,
                         seed, ESPSOL_MNEMONIC_SEED_SIZE);
    
    /* Clear sensitive data */
    memset(salt, 0, sizeof(salt));
    
    return ret;
}

/* ============================================================================
 * Keypair Derivation
 * ========================================================================== */

esp_err_t espsol_keypair_from_mnemonic(const char *mnemonic, const char *passphrase,
                                        espsol_keypair_t *keypair)
{
    if (mnemonic == NULL || keypair == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Derive 64-byte seed from mnemonic */
    uint8_t bip39_seed[ESPSOL_MNEMONIC_SEED_SIZE];
    esp_err_t ret = espsol_mnemonic_to_seed(mnemonic, passphrase, bip39_seed);
    if (ret != ESP_OK) {
        return ret;
    }
    
    /* 
     * Solana uses the first 32 bytes of the BIP39 seed as the Ed25519 seed.
     * This is compatible with Phantom, Solflare, and other Solana wallets
     * when using the default derivation path.
     */
    ret = espsol_keypair_from_seed(bip39_seed, keypair);
    
    /* Clear sensitive data */
    memset(bip39_seed, 0, sizeof(bip39_seed));
    
    return ret;
}

esp_err_t espsol_keypair_generate_with_mnemonic(int word_count,
                                                  char *mnemonic, size_t mnemonic_len,
                                                  espsol_keypair_t *keypair)
{
    if (mnemonic == NULL || keypair == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret;
    
    /* Generate mnemonic */
    if (word_count == ESPSOL_MNEMONIC_12_WORDS) {
        ret = espsol_mnemonic_generate_12(mnemonic, mnemonic_len);
    } else if (word_count == ESPSOL_MNEMONIC_24_WORDS) {
        ret = espsol_mnemonic_generate_24(mnemonic, mnemonic_len);
    } else {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (ret != ESP_OK) {
        return ret;
    }
    
    /* Derive keypair from mnemonic (no passphrase) */
    return espsol_keypair_from_mnemonic(mnemonic, NULL, keypair);
}

/* ============================================================================
 * Utility Functions
 * ========================================================================== */

const char *espsol_mnemonic_get_word(int index)
{
    if (index < 0 || index >= 2048) {
        return NULL;
    }
    return espsol_bip39_wordlist[index];
}

void espsol_mnemonic_clear(char *mnemonic, size_t len)
{
    if (mnemonic != NULL && len > 0) {
        /* Use volatile to prevent compiler optimization */
        volatile char *p = (volatile char *)mnemonic;
        while (len--) {
            *p++ = 0;
        }
    }
}

void espsol_mnemonic_print(const char *mnemonic, const char *label)
{
    if (mnemonic == NULL) {
        return;
    }
    
    if (label != NULL) {
        LOG_I("=== %s ===", label);
    } else {
        LOG_I("=== YOUR SEED PHRASE ===");
    }
    
    /* Print words with numbers */
    char word_buf[16];
    const char *p = mnemonic;
    int word_num = 1;
    
    while (*p) {
        /* Skip spaces */
        while (*p == ' ') p++;
        if (*p == '\0') break;
        
        /* Extract word */
        size_t len = 0;
        while (*p && *p != ' ' && len < sizeof(word_buf) - 1) {
            word_buf[len++] = *p++;
        }
        word_buf[len] = '\0';
        
        LOG_I("%2d. %s", word_num++, word_buf);
    }
    
    LOG_I("========================");
    LOG_I("IMPORTANT: Write these words down and store them safely!");
    LOG_I("Never share your seed phrase with anyone.");
}
