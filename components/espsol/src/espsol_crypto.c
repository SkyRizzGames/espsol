/**
 * @file espsol_crypto.c
 * @brief ESPSOL Cryptographic Operations Implementation
 *
 * Implements Ed25519 cryptographic operations using:
 * - libsodium on ESP-IDF (preferred, well-tested)
 * - Compatible fallback for host testing
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include "espsol_crypto.h"
#include "espsol_utils.h"
#include <string.h>

/* Platform-specific includes */
#if defined(ESP_PLATFORM) && ESP_PLATFORM
#include "esp_log.h"
#include "esp_random.h"
#include "sodium.h"

static const char *TAG = "espsol_crypto";
#define LOG_I(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define LOG_E(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#else
/* Host compilation - use libsodium if available */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef USE_LIBSODIUM
#include <sodium.h>
#else
/* Minimal Ed25519 implementation for host testing */
#include "espsol_ed25519.h"
#endif

#define LOG_I(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_E(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_W(fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#endif

/* ============================================================================
 * Static State
 * ========================================================================== */

static bool s_crypto_initialized = false;

/* ============================================================================
 * Platform-Specific Implementation
 * ========================================================================== */

#if defined(ESP_PLATFORM) && ESP_PLATFORM

/* ESP-IDF implementation using libsodium */

esp_err_t espsol_crypto_init(void)
{
    if (s_crypto_initialized) {
        return ESP_OK;
    }

    if (sodium_init() < 0) {
        LOG_E("Failed to initialize libsodium");
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }

    s_crypto_initialized = true;
    LOG_I("Crypto subsystem initialized");
    return ESP_OK;
}

esp_err_t espsol_random_bytes(uint8_t *buffer, size_t len)
{
    if (buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (len == 0) {
        return ESP_OK;
    }

    /* Use ESP32 hardware RNG through libsodium */
    randombytes_buf(buffer, len);
    return ESP_OK;
}

static esp_err_t ed25519_keypair_from_seed(const uint8_t *seed,
                                            uint8_t *public_key,
                                            uint8_t *private_key)
{
    if (crypto_sign_seed_keypair(public_key, private_key, seed) != 0) {
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }
    return ESP_OK;
}

static esp_err_t ed25519_sign(const uint8_t *message, size_t message_len,
                               const uint8_t *private_key,
                               uint8_t *signature)
{
    unsigned long long sig_len;
    if (crypto_sign_detached(signature, &sig_len, message, message_len, private_key) != 0) {
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }
    return ESP_OK;
}

static esp_err_t ed25519_verify(const uint8_t *message, size_t message_len,
                                 const uint8_t *signature,
                                 const uint8_t *public_key)
{
    if (crypto_sign_verify_detached(signature, message, message_len, public_key) != 0) {
        return ESP_ERR_ESPSOL_SIGNATURE_INVALID;
    }
    return ESP_OK;
}

#else /* Host implementation */

esp_err_t espsol_crypto_init(void)
{
    if (s_crypto_initialized) {
        return ESP_OK;
    }

#ifdef USE_LIBSODIUM
    if (sodium_init() < 0) {
        LOG_E("Failed to initialize libsodium");
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }
#else
    /* Seed basic RNG for non-crypto random (test only) */
    srand((unsigned int)time(NULL));
#endif

    s_crypto_initialized = true;
    return ESP_OK;
}

esp_err_t espsol_random_bytes(uint8_t *buffer, size_t len)
{
    if (buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (len == 0) {
        return ESP_OK;
    }

#ifdef USE_LIBSODIUM
    randombytes_buf(buffer, len);
#else
    /* Fallback: use system random (NOT cryptographically secure - for testing only) */
    for (size_t i = 0; i < len; i++) {
        buffer[i] = (uint8_t)(rand() & 0xFF);
    }
#endif

    return ESP_OK;
}

#ifdef USE_LIBSODIUM

static esp_err_t ed25519_keypair_from_seed(const uint8_t *seed,
                                            uint8_t *public_key,
                                            uint8_t *private_key)
{
    if (crypto_sign_seed_keypair(public_key, private_key, seed) != 0) {
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }
    return ESP_OK;
}

static esp_err_t ed25519_sign(const uint8_t *message, size_t message_len,
                               const uint8_t *private_key,
                               uint8_t *signature)
{
    unsigned long long sig_len;
    if (crypto_sign_detached(signature, &sig_len, message, message_len, private_key) != 0) {
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }
    return ESP_OK;
}

static esp_err_t ed25519_verify(const uint8_t *message, size_t message_len,
                                 const uint8_t *signature,
                                 const uint8_t *public_key)
{
    if (crypto_sign_verify_detached(signature, message, message_len, public_key) != 0) {
        return ESP_ERR_ESPSOL_SIGNATURE_INVALID;
    }
    return ESP_OK;
}

#else /* Portable Ed25519 implementation */

/* Use embedded Ed25519 implementation (TweetNaCl-derived) */
static esp_err_t ed25519_keypair_from_seed(const uint8_t *seed,
                                            uint8_t *public_key,
                                            uint8_t *private_key)
{
    return espsol_ed25519_keypair_from_seed(seed, public_key, private_key);
}

static esp_err_t ed25519_sign(const uint8_t *message, size_t message_len,
                               const uint8_t *private_key,
                               uint8_t *signature)
{
    return espsol_ed25519_sign(message, message_len, private_key, signature);
}

static esp_err_t ed25519_verify(const uint8_t *message, size_t message_len,
                                 const uint8_t *signature,
                                 const uint8_t *public_key)
{
    return espsol_ed25519_verify(message, message_len, signature, public_key);
}

#endif /* USE_LIBSODIUM */

#endif /* ESP_PLATFORM */

/* ============================================================================
 * Common Implementation
 * ========================================================================== */

esp_err_t espsol_random_seed(uint8_t seed[ESPSOL_SEED_SIZE])
{
    if (seed == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return espsol_random_bytes(seed, ESPSOL_SEED_SIZE);
}

esp_err_t espsol_keypair_init(espsol_keypair_t *keypair)
{
    if (keypair == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(keypair, 0, sizeof(espsol_keypair_t));
    keypair->initialized = false;
    return ESP_OK;
}

esp_err_t espsol_keypair_generate(espsol_keypair_t *keypair)
{
    if (keypair == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Generate random seed */
    uint8_t seed[ESPSOL_SEED_SIZE];
    esp_err_t err = espsol_random_seed(seed);
    if (err != ESP_OK) {
        return err;
    }

    /* Generate keypair from seed */
    err = espsol_keypair_from_seed(seed, keypair);

    /* Clear seed from memory */
    memset(seed, 0, sizeof(seed));

    return err;
}

esp_err_t espsol_keypair_from_seed(const uint8_t seed[ESPSOL_SEED_SIZE],
                                    espsol_keypair_t *keypair)
{
    if (seed == NULL || keypair == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Clear the keypair first */
    espsol_keypair_init(keypair);

    /* Generate keypair from seed */
    esp_err_t err = ed25519_keypair_from_seed(seed,
                                               keypair->public_key,
                                               keypair->private_key);
    if (err != ESP_OK) {
        espsol_keypair_clear(keypair);
        return err;
    }

    keypair->initialized = true;
    return ESP_OK;
}

esp_err_t espsol_keypair_clear(espsol_keypair_t *keypair)
{
    if (keypair == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Securely clear sensitive data */
    memset(keypair->private_key, 0, sizeof(keypair->private_key));
    memset(keypair->public_key, 0, sizeof(keypair->public_key));
    keypair->initialized = false;

    return ESP_OK;
}

esp_err_t espsol_keypair_from_private_key(const uint8_t private_key[ESPSOL_PRIVKEY_SIZE],
                                           espsol_keypair_t *keypair)
{
    if (private_key == NULL || keypair == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Clear the keypair first */
    espsol_keypair_init(keypair);

    /* Copy private key */
    memcpy(keypair->private_key, private_key, ESPSOL_PRIVKEY_SIZE);

    /* Extract public key from private key (last 32 bytes) */
    memcpy(keypair->public_key, private_key + ESPSOL_SEED_SIZE, ESPSOL_PUBKEY_SIZE);

    keypair->initialized = true;
    return ESP_OK;
}

esp_err_t espsol_keypair_from_base58(const char *base58_key,
                                      espsol_keypair_t *keypair)
{
    if (base58_key == NULL || keypair == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Decode the Base58 key */
    uint8_t decoded[ESPSOL_PRIVKEY_SIZE];
    size_t decoded_len = sizeof(decoded);

    esp_err_t err = espsol_base58_decode(base58_key, decoded, &decoded_len);
    if (err != ESP_OK) {
        return err;
    }

    /* Handle both 32-byte (seed) and 64-byte (full key) formats */
    if (decoded_len == ESPSOL_SEED_SIZE) {
        /* 32-byte seed - generate keypair from it */
        err = espsol_keypair_from_seed(decoded, keypair);
    } else if (decoded_len == ESPSOL_PRIVKEY_SIZE) {
        /* 64-byte full private key */
        err = espsol_keypair_from_private_key(decoded, keypair);
    } else {
        err = ESP_ERR_ESPSOL_INVALID_BASE58;
    }

    /* Clear decoded data */
    memset(decoded, 0, sizeof(decoded));

    return err;
}

esp_err_t espsol_keypair_to_base58(const espsol_keypair_t *keypair,
                                    char *output, size_t output_len)
{
    if (keypair == NULL || output == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!keypair->initialized) {
        return ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT;
    }

    /* Encode the full 64-byte private key as Base58 */
    return espsol_base58_encode(keypair->private_key, ESPSOL_PRIVKEY_SIZE,
                                 output, output_len);
}

esp_err_t espsol_keypair_get_address(const espsol_keypair_t *keypair,
                                      char *address, size_t address_len)
{
    if (keypair == NULL || address == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!keypair->initialized) {
        return ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT;
    }

    return espsol_pubkey_to_address(keypair->public_key, address, address_len);
}

esp_err_t espsol_sign(const uint8_t *message, size_t message_len,
                       const espsol_keypair_t *keypair,
                       uint8_t signature[ESPSOL_SIGNATURE_SIZE])
{
    if (keypair == NULL || signature == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Allow NULL message with len 0 (empty message signing) */
    if (message == NULL && message_len > 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!keypair->initialized) {
        return ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT;
    }

    return ed25519_sign(message, message_len, keypair->private_key, signature);
}

esp_err_t espsol_sign_raw(const uint8_t *message, size_t message_len,
                           const uint8_t private_key[ESPSOL_PRIVKEY_SIZE],
                           uint8_t signature[ESPSOL_SIGNATURE_SIZE])
{
    if (private_key == NULL || signature == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Allow NULL message with len 0 (empty message signing) */
    if (message == NULL && message_len > 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return ed25519_sign(message, message_len, private_key, signature);
}

esp_err_t espsol_sign_string(const char *message,
                              const espsol_keypair_t *keypair,
                              uint8_t signature[ESPSOL_SIGNATURE_SIZE])
{
    if (message == NULL || keypair == NULL || signature == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return espsol_sign((const uint8_t *)message, strlen(message), keypair, signature);
}

esp_err_t espsol_verify(const uint8_t *message, size_t message_len,
                         const uint8_t signature[ESPSOL_SIGNATURE_SIZE],
                         const uint8_t public_key[ESPSOL_PUBKEY_SIZE])
{
    if (signature == NULL || public_key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Allow NULL message with len 0 (empty message verification) */
    if (message == NULL && message_len > 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return ed25519_verify(message, message_len, signature, public_key);
}

esp_err_t espsol_verify_keypair(const uint8_t *message, size_t message_len,
                                 const uint8_t signature[ESPSOL_SIGNATURE_SIZE],
                                 const espsol_keypair_t *keypair)
{
    if (keypair == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!keypair->initialized) {
        return ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT;
    }

    return espsol_verify(message, message_len, signature, keypair->public_key);
}

esp_err_t espsol_public_key_from_private(const uint8_t private_key[ESPSOL_PRIVKEY_SIZE],
                                          uint8_t public_key[ESPSOL_PUBKEY_SIZE])
{
    if (private_key == NULL || public_key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* In Ed25519, the public key is stored in the last 32 bytes of the 64-byte private key */
    memcpy(public_key, private_key + ESPSOL_SEED_SIZE, ESPSOL_PUBKEY_SIZE);
    return ESP_OK;
}

esp_err_t espsol_crypto_self_test(void)
{
    /* RFC 8032 Test Vector 1 */
    static const uint8_t test_seed[32] = {
        0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60,
        0xba, 0x84, 0x4a, 0xf4, 0x92, 0xec, 0x2c, 0xc4,
        0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32, 0x69, 0x19,
        0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60
    };

    static const uint8_t expected_pubkey[32] = {
        0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7,
        0xd5, 0x4b, 0xfe, 0xd3, 0xc9, 0x64, 0x07, 0x3a,
        0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6, 0x23, 0x25,
        0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a
    };

    /* Expected signature for empty message with this key */
    static const uint8_t expected_signature[64] = {
        0xe5, 0x56, 0x43, 0x00, 0xc3, 0x60, 0xac, 0x72,
        0x90, 0x86, 0xe2, 0xcc, 0x80, 0x6e, 0x82, 0x8a,
        0x84, 0x87, 0x7f, 0x1e, 0xb8, 0xe5, 0xd9, 0x74,
        0xd8, 0x73, 0xe0, 0x65, 0x22, 0x49, 0x01, 0x55,
        0x5f, 0xb8, 0x82, 0x15, 0x90, 0xa3, 0x3b, 0xac,
        0xc6, 0x1e, 0x39, 0x70, 0x1c, 0xf9, 0xb4, 0x6b,
        0xd2, 0x5b, 0xf5, 0xf0, 0x59, 0x5b, 0xbe, 0x24,
        0x65, 0x51, 0x41, 0x43, 0x8e, 0x7a, 0x10, 0x0b
    };

    esp_err_t err;
    espsol_keypair_t keypair;

    /* Test 1: Generate keypair from seed */
    err = espsol_keypair_from_seed(test_seed, &keypair);
    if (err != ESP_OK) {
        LOG_E("Self-test: keypair generation failed");
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }

    /* Test 2: Verify public key matches expected */
    if (memcmp(keypair.public_key, expected_pubkey, ESPSOL_PUBKEY_SIZE) != 0) {
        LOG_E("Self-test: public key mismatch");
        espsol_keypair_clear(&keypair);
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }

    /* Test 3: Sign empty message */
    uint8_t signature[ESPSOL_SIGNATURE_SIZE];
    err = espsol_sign(NULL, 0, &keypair, signature);
    if (err != ESP_OK) {
        LOG_E("Self-test: signing failed");
        espsol_keypair_clear(&keypair);
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }

    /* Test 4: Verify signature matches expected */
    if (memcmp(signature, expected_signature, ESPSOL_SIGNATURE_SIZE) != 0) {
        LOG_E("Self-test: signature mismatch");
        espsol_keypair_clear(&keypair);
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }

    /* Test 5: Verify the signature */
    err = espsol_verify(NULL, 0, signature, keypair.public_key);
    if (err != ESP_OK) {
        LOG_E("Self-test: verification failed");
        espsol_keypair_clear(&keypair);
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }

    espsol_keypair_clear(&keypair);
    LOG_I("Crypto self-test passed");
    return ESP_OK;
}

void espsol_keypair_print_address(const espsol_keypair_t *keypair, const char *label)
{
    if (keypair == NULL || !keypair->initialized) {
        LOG_E("Cannot print address: keypair not initialized");
        return;
    }

    char address[ESPSOL_ADDRESS_MAX_LEN];
    if (espsol_keypair_get_address(keypair, address, sizeof(address)) == ESP_OK) {
        if (label != NULL) {
            LOG_I("%s: %s", label, address);
        } else {
            LOG_I("Address: %s", address);
        }
    } else {
        LOG_E("Failed to get keypair address");
    }
}

/* ============================================================================
 * NVS Secure Storage (ESP32 only)
 * ========================================================================== */

#if defined(ESP_PLATFORM) && ESP_PLATFORM
#include "nvs_flash.h"
#include "nvs.h"

/* NVS namespace for ESPSOL keypairs */
#define ESPSOL_NVS_NAMESPACE "espsol_keys"

esp_err_t espsol_keypair_save_to_nvs(const espsol_keypair_t *keypair,
                                      const char *nvs_key)
{
    if (keypair == NULL || nvs_key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!keypair->initialized) {
        return ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT;
    }
    
    nvs_handle_t handle;
    esp_err_t err = nvs_open(ESPSOL_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        LOG_E("Failed to open NVS namespace: %s", esp_err_to_name(err));
        return err;
    }
    
    /* Store private key (64 bytes) - includes public key */
    err = nvs_set_blob(handle, nvs_key, keypair->private_key, ESPSOL_PRIVKEY_SIZE);
    if (err != ESP_OK) {
        LOG_E("Failed to write keypair to NVS: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }
    
    /* Commit changes */
    err = nvs_commit(handle);
    if (err != ESP_OK) {
        LOG_E("Failed to commit NVS: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }
    
    nvs_close(handle);
    LOG_I("Keypair saved to NVS key: %s", nvs_key);
    return ESP_OK;
}

esp_err_t espsol_keypair_load_from_nvs(const char *nvs_key,
                                        espsol_keypair_t *keypair)
{
    if (nvs_key == NULL || keypair == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Initialize the keypair structure */
    espsol_keypair_init(keypair);
    
    nvs_handle_t handle;
    esp_err_t err = nvs_open(ESPSOL_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        LOG_E("Failed to open NVS namespace: %s", esp_err_to_name(err));
        return err;
    }
    
    /* Get the required size */
    size_t required_size = 0;
    err = nvs_get_blob(handle, nvs_key, NULL, &required_size);
    if (err != ESP_OK) {
        nvs_close(handle);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            LOG_W("Keypair not found in NVS: %s", nvs_key);
        } else {
            LOG_E("Failed to get keypair size from NVS: %s", esp_err_to_name(err));
        }
        return err;
    }
    
    if (required_size != ESPSOL_PRIVKEY_SIZE) {
        LOG_E("Invalid keypair size in NVS: %zu (expected %d)", 
              required_size, ESPSOL_PRIVKEY_SIZE);
        nvs_close(handle);
        return ESP_ERR_ESPSOL_CRYPTO_ERROR;
    }
    
    /* Read the private key */
    size_t size = ESPSOL_PRIVKEY_SIZE;
    err = nvs_get_blob(handle, nvs_key, keypair->private_key, &size);
    nvs_close(handle);
    
    if (err != ESP_OK) {
        LOG_E("Failed to read keypair from NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    /* Extract public key from private key (last 32 bytes) */
    memcpy(keypair->public_key, 
           keypair->private_key + ESPSOL_SEED_SIZE, 
           ESPSOL_PUBKEY_SIZE);
    keypair->initialized = true;
    
    LOG_I("Keypair loaded from NVS key: %s", nvs_key);
    return ESP_OK;
}

esp_err_t espsol_keypair_delete_from_nvs(const char *nvs_key)
{
    if (nvs_key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t handle;
    esp_err_t err = nvs_open(ESPSOL_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        LOG_E("Failed to open NVS namespace: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_erase_key(handle, nvs_key);
    if (err != ESP_OK) {
        nvs_close(handle);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            LOG_W("Keypair not found in NVS: %s", nvs_key);
        } else {
            LOG_E("Failed to delete keypair from NVS: %s", esp_err_to_name(err));
        }
        return err;
    }
    
    err = nvs_commit(handle);
    nvs_close(handle);
    
    if (err != ESP_OK) {
        LOG_E("Failed to commit NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    LOG_I("Keypair deleted from NVS key: %s", nvs_key);
    return ESP_OK;
}

esp_err_t espsol_keypair_exists_in_nvs(const char *nvs_key, bool *exists)
{
    if (nvs_key == NULL || exists == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *exists = false;
    
    nvs_handle_t handle;
    esp_err_t err = nvs_open(ESPSOL_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        /* Namespace doesn't exist, key doesn't exist */
        return ESP_OK;
    }
    if (err != ESP_OK) {
        LOG_E("Failed to open NVS namespace: %s", esp_err_to_name(err));
        return err;
    }
    
    size_t required_size = 0;
    err = nvs_get_blob(handle, nvs_key, NULL, &required_size);
    nvs_close(handle);
    
    if (err == ESP_OK && required_size == ESPSOL_PRIVKEY_SIZE) {
        *exists = true;
    }
    
    return ESP_OK;
}

#else /* !ESP_PLATFORM - Host stubs */

/* ESP_ERR_NOT_SUPPORTED is not defined on host */
#ifndef ESP_ERR_NOT_SUPPORTED
#define ESP_ERR_NOT_SUPPORTED 0x106
#endif

esp_err_t espsol_keypair_save_to_nvs(const espsol_keypair_t *keypair,
                                      const char *nvs_key)
{
    (void)keypair;
    (void)nvs_key;
    LOG_W("NVS storage not available on host");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t espsol_keypair_load_from_nvs(const char *nvs_key,
                                        espsol_keypair_t *keypair)
{
    (void)nvs_key;
    (void)keypair;
    LOG_W("NVS storage not available on host");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t espsol_keypair_delete_from_nvs(const char *nvs_key)
{
    (void)nvs_key;
    LOG_W("NVS storage not available on host");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t espsol_keypair_exists_in_nvs(const char *nvs_key, bool *exists)
{
    (void)nvs_key;
    if (exists) *exists = false;
    return ESP_ERR_NOT_SUPPORTED;
}

#endif /* ESP_PLATFORM */
