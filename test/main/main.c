/**
 * @file main.c
 * @brief ESPSOL Complete Test Suite
 *
 * Tests for Base58, Base64 encoding/decoding, crypto, RPC, and transactions.
 * 
 * Set RUN_INTEGRATION_TEST to 1 to run devnet integration test instead of
 * unit tests. Make sure to update WiFi credentials in integration_test.c
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "espsol.h"
#include "integration_test.h"

/* ============================================================================
 * Test Mode Selection
 * Set RUN_INTEGRATION_TEST to 1 to run the devnet integration test
 * Set RUN_INTEGRATION_TEST to 0 to run unit tests only
 * ========================================================================== */
#define RUN_INTEGRATION_TEST 1

static const char *TAG = "espsol_test";

/* ============================================================================
 * Test Counters
 * ========================================================================== */

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            ESP_LOGI(TAG, "✓ PASS: %s", message); \
            tests_passed++; \
        } else { \
            ESP_LOGE(TAG, "✗ FAIL: %s", message); \
            tests_failed++; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) == (expected)) { \
            ESP_LOGI(TAG, "✓ PASS: %s", message); \
            tests_passed++; \
        } else { \
            ESP_LOGE(TAG, "✗ FAIL: %s (expected %d, got %d)", message, (int)(expected), (int)(actual)); \
            tests_failed++; \
        } \
    } while (0)

#define TEST_ASSERT_STR_EQ(actual, expected, message) \
    do { \
        if (strcmp((actual), (expected)) == 0) { \
            ESP_LOGI(TAG, "✓ PASS: %s", message); \
            tests_passed++; \
        } else { \
            ESP_LOGE(TAG, "✗ FAIL: %s (expected '%s', got '%s')", message, (expected), (actual)); \
            tests_failed++; \
        } \
    } while (0)

/* ============================================================================
 * Test: Core SDK
 * ========================================================================== */

static void test_core_sdk(void)
{
    ESP_LOGI(TAG, "\n========== Core SDK Tests ==========");

    /* Test version */
    const char *version = espsol_get_version();
    TEST_ASSERT(version != NULL, "espsol_get_version returns non-NULL");
    TEST_ASSERT_STR_EQ(version, "0.1.0", "Version is 0.1.0");

    uint8_t major, minor, patch;
    espsol_get_version_numbers(&major, &minor, &patch);
    TEST_ASSERT_EQ(major, 0, "Major version is 0");
    TEST_ASSERT_EQ(minor, 1, "Minor version is 1");
    TEST_ASSERT_EQ(patch, 0, "Patch version is 0");

    /* Test initialization */
    TEST_ASSERT(!espsol_is_initialized(), "Not initialized before init");

    esp_err_t err = espsol_init(NULL);  /* Use defaults */
    TEST_ASSERT_EQ(err, ESP_OK, "espsol_init with defaults succeeds");
    TEST_ASSERT(espsol_is_initialized(), "Is initialized after init");

    /* Test config */
    const espsol_config_t *config = espsol_get_config();
    TEST_ASSERT(config != NULL, "espsol_get_config returns non-NULL");
    TEST_ASSERT_STR_EQ(config->rpc_url, ESPSOL_DEVNET_RPC, "Default RPC is devnet");

    /* Test double init */
    err = espsol_init(NULL);
    TEST_ASSERT_EQ(err, ESP_OK, "Double init returns OK (idempotent)");

    /* Test deinit */
    err = espsol_deinit();
    TEST_ASSERT_EQ(err, ESP_OK, "espsol_deinit succeeds");
    TEST_ASSERT(!espsol_is_initialized(), "Not initialized after deinit");

    /* Test deinit when not initialized */
    err = espsol_deinit();
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_NOT_INITIALIZED, "Deinit when not initialized returns error");

    /* Test custom config */
    espsol_config_t custom_config = {
        .rpc_url = "http://localhost:8899",
        .timeout_ms = 10000,
        .commitment = ESPSOL_COMMITMENT_FINALIZED,
        .use_tls = false,
        .log_level = ESPSOL_LOG_DEBUG
    };
    err = espsol_init(&custom_config);
    TEST_ASSERT_EQ(err, ESP_OK, "espsol_init with custom config succeeds");
    
    config = espsol_get_config();
    TEST_ASSERT_STR_EQ(config->rpc_url, "http://localhost:8899", "Custom RPC URL is set");
    TEST_ASSERT_EQ(config->timeout_ms, 10000, "Custom timeout is set");
    
    espsol_deinit();

    /* Test error name */
    const char *err_name = espsol_err_to_name(ESP_ERR_ESPSOL_INVALID_BASE58);
    TEST_ASSERT(err_name != NULL, "Error name is not NULL");
    ESP_LOGI(TAG, "Error name for INVALID_BASE58: %s", err_name);
}

/* ============================================================================
 * Test: Base58 Encoding/Decoding
 * ========================================================================== */

static void test_base58(void)
{
    ESP_LOGI(TAG, "\n========== Base58 Tests ==========");

    esp_err_t err;
    char encoded[128];
    uint8_t decoded[64];
    size_t decoded_len;

    /* Test vector 1: Simple bytes */
    {
        const uint8_t data[] = { 0x00, 0x01, 0x02, 0x03 };
        err = espsol_base58_encode(data, sizeof(data), encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Base58 encode simple bytes");
        ESP_LOGI(TAG, "Encoded: %s", encoded);
        
        decoded_len = sizeof(decoded);
        err = espsol_base58_decode(encoded, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Base58 decode roundtrip");
        TEST_ASSERT_EQ(decoded_len, sizeof(data), "Decoded length matches");
        TEST_ASSERT(memcmp(data, decoded, sizeof(data)) == 0, "Decoded data matches");
    }

    /* Test vector 2: Known Solana public key (System Program) */
    {
        /* 11111111111111111111111111111111 = 32 bytes of zeros */
        const uint8_t system_program[32] = { 0 };
        err = espsol_base58_encode(system_program, 32, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Base58 encode System Program");
        TEST_ASSERT_STR_EQ(encoded, "11111111111111111111111111111111", "System Program address");
    }

    /* Test vector 3: Another known address */
    {
        /* Test with a random 32-byte key */
        const uint8_t test_key[32] = {
            0x0e, 0xc7, 0x8f, 0x5e, 0x2b, 0x85, 0xd0, 0x9c,
            0x4a, 0x1b, 0x3f, 0x6d, 0x2e, 0x7c, 0x8a, 0x5b,
            0x9d, 0x4e, 0x1f, 0x0a, 0x3c, 0x7b, 0x6d, 0x2e,
            0x8f, 0x5c, 0x9a, 0x1b, 0x4d, 0x0e, 0x7f, 0x3a
        };
        
        err = espsol_base58_encode(test_key, 32, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Base58 encode 32-byte key");
        ESP_LOGI(TAG, "Test key address: %s", encoded);
        
        /* Verify roundtrip */
        decoded_len = sizeof(decoded);
        err = espsol_base58_decode(encoded, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Base58 decode 32-byte key");
        TEST_ASSERT_EQ(decoded_len, 32, "Decoded length is 32");
        TEST_ASSERT(memcmp(test_key, decoded, 32) == 0, "32-byte key roundtrip");
    }

    /* Test vector 4: Empty input */
    {
        err = espsol_base58_encode((const uint8_t *)"", 0, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Base58 encode empty");
        TEST_ASSERT_STR_EQ(encoded, "", "Empty encodes to empty");
        
        decoded_len = sizeof(decoded);
        err = espsol_base58_decode("", decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Base58 decode empty");
        TEST_ASSERT_EQ(decoded_len, 0, "Empty decodes to zero length");
    }

    /* Test: Invalid Base58 character */
    {
        decoded_len = sizeof(decoded);
        err = espsol_base58_decode("Invalid0Character", decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE58, "Invalid char '0' detected");
        
        decoded_len = sizeof(decoded);
        err = espsol_base58_decode("InvalidOCharacter", decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE58, "Invalid char 'O' detected");
        
        decoded_len = sizeof(decoded);
        err = espsol_base58_decode("InvalidICharacter", decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE58, "Invalid char 'I' detected");
        
        decoded_len = sizeof(decoded);
        err = espsol_base58_decode("Invalidlharacter", decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE58, "Invalid char 'l' detected");
    }

    /* Test: Buffer too small */
    {
        const uint8_t data[] = { 0x01, 0x02, 0x03 };
        err = espsol_base58_encode(data, sizeof(data), encoded, 2);  /* Too small */
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_BUFFER_TOO_SMALL, "Buffer too small detected");
    }

    /* Test: NULL pointers */
    {
        err = espsol_base58_encode(NULL, 1, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "NULL input data detected");
        
        const uint8_t data[] = { 0x01 };
        err = espsol_base58_encode(data, 1, NULL, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "NULL output buffer detected");
    }
}

/* ============================================================================
 * Test: Base64 Encoding/Decoding
 * ========================================================================== */

static void test_base64(void)
{
    ESP_LOGI(TAG, "\n========== Base64 Tests ==========");

    esp_err_t err;
    char encoded[256];
    uint8_t decoded[128];
    size_t decoded_len;

    /* Test vector 1: "Hello" */
    {
        const uint8_t data[] = "Hello";
        err = espsol_base64_encode(data, 5, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Base64 encode 'Hello'");
        TEST_ASSERT_STR_EQ(encoded, "SGVsbG8=", "Hello -> SGVsbG8=");
        
        decoded_len = sizeof(decoded);
        err = espsol_base64_decode(encoded, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Base64 decode 'Hello'");
        TEST_ASSERT_EQ(decoded_len, 5, "Decoded length is 5");
        TEST_ASSERT(memcmp(data, decoded, 5) == 0, "Decoded matches 'Hello'");
    }

    /* Test vector 2: "Hello!" (6 bytes, no padding) */
    {
        const uint8_t data[] = "Hello!";
        err = espsol_base64_encode(data, 6, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Base64 encode 'Hello!'");
        TEST_ASSERT_STR_EQ(encoded, "SGVsbG8h", "Hello! -> SGVsbG8h");
    }

    /* Test vector 3: "Hi" (2 bytes, double padding) */
    {
        const uint8_t data[] = "Hi";
        err = espsol_base64_encode(data, 2, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Base64 encode 'Hi'");
        TEST_ASSERT_STR_EQ(encoded, "SGk=", "Hi -> SGk=");
    }

    /* Test vector 4: "H" (1 byte, double padding) */
    {
        const uint8_t data[] = "H";
        err = espsol_base64_encode(data, 1, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Base64 encode 'H'");
        TEST_ASSERT_STR_EQ(encoded, "SA==", "H -> SA==");
    }

    /* Test vector 5: Binary data */
    {
        const uint8_t data[] = { 0x00, 0xFF, 0x80, 0x7F, 0x01 };
        err = espsol_base64_encode(data, sizeof(data), encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Base64 encode binary");
        ESP_LOGI(TAG, "Binary encoded: %s", encoded);
        
        decoded_len = sizeof(decoded);
        err = espsol_base64_decode(encoded, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Base64 decode binary roundtrip");
        TEST_ASSERT_EQ(decoded_len, sizeof(data), "Decoded length matches");
        TEST_ASSERT(memcmp(data, decoded, sizeof(data)) == 0, "Binary roundtrip");
    }

    /* Test vector 6: Empty */
    {
        err = espsol_base64_encode((const uint8_t *)"", 0, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Base64 encode empty");
        TEST_ASSERT_STR_EQ(encoded, "", "Empty encodes to empty");
    }

    /* Test: Longer data (simulating transaction) */
    {
        uint8_t tx_data[100];
        for (int i = 0; i < 100; i++) {
            tx_data[i] = (uint8_t)i;
        }
        
        err = espsol_base64_encode(tx_data, 100, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Base64 encode 100 bytes");
        ESP_LOGI(TAG, "100-byte encoded length: %d", (int)strlen(encoded));
        
        decoded_len = sizeof(decoded);
        err = espsol_base64_decode(encoded, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Base64 decode 100 bytes");
        TEST_ASSERT_EQ(decoded_len, 100, "Decoded length is 100");
        TEST_ASSERT(memcmp(tx_data, decoded, 100) == 0, "100-byte roundtrip");
    }

    /* Test: Invalid Base64 */
    {
        decoded_len = sizeof(decoded);
        err = espsol_base64_decode("SGVsbG8", decoded, &decoded_len);  /* Missing padding */
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE64, "Invalid length detected");
        
        decoded_len = sizeof(decoded);
        err = espsol_base64_decode("SGVs!G8=", decoded, &decoded_len);  /* Invalid char */
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE64, "Invalid char detected");
    }
}

/* ============================================================================
 * Test: Address Utilities
 * ========================================================================== */

static void test_address_utils(void)
{
    ESP_LOGI(TAG, "\n========== Address Utility Tests ==========");

    esp_err_t err;
    char address[ESPSOL_ADDRESS_MAX_LEN];
    uint8_t pubkey[ESPSOL_PUBKEY_SIZE];

    /* Test: System Program address */
    {
        const uint8_t system_pubkey[32] = { 0 };
        err = espsol_pubkey_to_address(system_pubkey, address, sizeof(address));
        TEST_ASSERT_EQ(err, ESP_OK, "pubkey_to_address for System Program");
        TEST_ASSERT_STR_EQ(address, "11111111111111111111111111111111", "System Program address");
    }

    /* Test: Roundtrip */
    {
        const uint8_t test_pubkey[32] = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
            0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
            0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
        };
        
        err = espsol_pubkey_to_address(test_pubkey, address, sizeof(address));
        TEST_ASSERT_EQ(err, ESP_OK, "pubkey_to_address roundtrip");
        ESP_LOGI(TAG, "Test address: %s", address);
        
        err = espsol_address_to_pubkey(address, pubkey);
        TEST_ASSERT_EQ(err, ESP_OK, "address_to_pubkey roundtrip");
        TEST_ASSERT(memcmp(test_pubkey, pubkey, 32) == 0, "Pubkey roundtrip match");
    }

    /* Test: Valid address check */
    {
        TEST_ASSERT(espsol_is_valid_address("11111111111111111111111111111111"), 
                    "System Program is valid");
        TEST_ASSERT(!espsol_is_valid_address("invalid"), 
                    "Short string is invalid");
        TEST_ASSERT(!espsol_is_valid_address("0000000000000000000000000000000000000000"), 
                    "String with '0' is invalid");
        TEST_ASSERT(!espsol_is_valid_address(NULL), 
                    "NULL is invalid");
    }
}

/* ============================================================================
 * Test: Hex Encoding (for debugging)
 * ========================================================================== */

static void test_hex(void)
{
    ESP_LOGI(TAG, "\n========== Hex Utility Tests ==========");

    esp_err_t err;
    char hex[128];
    uint8_t decoded[64];
    size_t decoded_len;

    /* Test: Basic encoding */
    {
        const uint8_t data[] = { 0x00, 0x01, 0xAB, 0xCD, 0xEF, 0xFF };
        err = espsol_hex_encode(data, sizeof(data), hex, sizeof(hex));
        TEST_ASSERT_EQ(err, ESP_OK, "Hex encode");
        TEST_ASSERT_STR_EQ(hex, "0001abcdefff", "Hex output matches");
        ESP_LOGI(TAG, "Hex: %s", hex);
    }

    /* Test: Roundtrip */
    {
        const uint8_t data[] = { 0xDE, 0xAD, 0xBE, 0xEF };
        err = espsol_hex_encode(data, sizeof(data), hex, sizeof(hex));
        TEST_ASSERT_EQ(err, ESP_OK, "Hex encode deadbeef");
        TEST_ASSERT_STR_EQ(hex, "deadbeef", "Hex is deadbeef");
        
        decoded_len = sizeof(decoded);
        err = espsol_hex_decode(hex, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Hex decode deadbeef");
        TEST_ASSERT_EQ(decoded_len, 4, "Decoded length is 4");
        TEST_ASSERT(memcmp(data, decoded, 4) == 0, "Hex roundtrip");
    }
}

/* ============================================================================
 * Test: Crypto Keypair
 * ========================================================================== */

static void test_crypto_keypair(void)
{
    ESP_LOGI(TAG, "\n========== Crypto Keypair Tests ==========");

    esp_err_t err;

    /* Initialize crypto subsystem */
    err = espsol_crypto_init();
    TEST_ASSERT_EQ(err, ESP_OK, "Crypto init");

    /* Test: Generate keypair from known seed (RFC 8032 test vector) */
    {
        const uint8_t test_seed[32] = {
            0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60,
            0xba, 0x84, 0x4a, 0xf4, 0x92, 0xec, 0x2c, 0xc4,
            0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32, 0x69, 0x19,
            0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60
        };
        
        const uint8_t expected_pubkey[32] = {
            0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7,
            0xd5, 0x4b, 0xfe, 0xd3, 0xc9, 0x64, 0x07, 0x3a,
            0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6, 0x23, 0x25,
            0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a
        };
        
        espsol_keypair_t keypair;
        err = espsol_keypair_from_seed(test_seed, &keypair);
        TEST_ASSERT_EQ(err, ESP_OK, "Keypair from seed");
        TEST_ASSERT(keypair.initialized, "Keypair initialized");
        TEST_ASSERT(memcmp(keypair.public_key, expected_pubkey, 32) == 0, "Public key matches RFC 8032");
        
        char address[ESPSOL_ADDRESS_MAX_LEN];
        err = espsol_keypair_get_address(&keypair, address, sizeof(address));
        TEST_ASSERT_EQ(err, ESP_OK, "Get keypair address");
        ESP_LOGI(TAG, "  Test keypair address: %s", address);
        
        espsol_keypair_clear(&keypair);
        TEST_ASSERT(!keypair.initialized, "Keypair cleared");
    }

    /* Test: Generate random keypair */
    {
        espsol_keypair_t keypair;
        err = espsol_keypair_generate(&keypair);
        TEST_ASSERT_EQ(err, ESP_OK, "Generate random keypair");
        TEST_ASSERT(keypair.initialized, "Random keypair initialized");
        
        char address[ESPSOL_ADDRESS_MAX_LEN];
        err = espsol_keypair_get_address(&keypair, address, sizeof(address));
        TEST_ASSERT_EQ(err, ESP_OK, "Get random keypair address");
        ESP_LOGI(TAG, "  Random keypair address: %s", address);
        
        espsol_keypair_clear(&keypair);
    }

    /* Test: Export/import keypair via Base58 */
    {
        espsol_keypair_t keypair1, keypair2;
        char base58_key[ESPSOL_PRIVKEY_BASE58_LEN];
        
        err = espsol_keypair_generate(&keypair1);
        TEST_ASSERT_EQ(err, ESP_OK, "Generate keypair for export");
        
        err = espsol_keypair_to_base58(&keypair1, base58_key, sizeof(base58_key));
        TEST_ASSERT_EQ(err, ESP_OK, "Export keypair to Base58");
        
        err = espsol_keypair_from_base58(base58_key, &keypair2);
        TEST_ASSERT_EQ(err, ESP_OK, "Import keypair from Base58");
        
        TEST_ASSERT(memcmp(keypair1.public_key, keypair2.public_key, 32) == 0, "Imported public key matches");
        TEST_ASSERT(memcmp(keypair1.private_key, keypair2.private_key, 64) == 0, "Imported private key matches");
        
        espsol_keypair_clear(&keypair1);
        espsol_keypair_clear(&keypair2);
    }
}

/* ============================================================================
 * Test: Crypto Signing
 * ========================================================================== */

static void test_crypto_signing(void)
{
    ESP_LOGI(TAG, "\n========== Crypto Signing Tests ==========");

    esp_err_t err;

    /* Test: Sign and verify with RFC 8032 test vector */
    {
        const uint8_t test_seed[32] = {
            0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60,
            0xba, 0x84, 0x4a, 0xf4, 0x92, 0xec, 0x2c, 0xc4,
            0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32, 0x69, 0x19,
            0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60
        };
        
        const uint8_t expected_sig[64] = {
            0xe5, 0x56, 0x43, 0x00, 0xc3, 0x60, 0xac, 0x72,
            0x90, 0x86, 0xe2, 0xcc, 0x80, 0x6e, 0x82, 0x8a,
            0x84, 0x87, 0x7f, 0x1e, 0xb8, 0xe5, 0xd9, 0x74,
            0xd8, 0x73, 0xe0, 0x65, 0x22, 0x49, 0x01, 0x55,
            0x5f, 0xb8, 0x82, 0x15, 0x90, 0xa3, 0x3b, 0xac,
            0xc6, 0x1e, 0x39, 0x70, 0x1c, 0xf9, 0xb4, 0x6b,
            0xd2, 0x5b, 0xf5, 0xf0, 0x59, 0x5b, 0xbe, 0x24,
            0x65, 0x51, 0x41, 0x43, 0x8e, 0x7a, 0x10, 0x0b
        };
        
        espsol_keypair_t keypair;
        err = espsol_keypair_from_seed(test_seed, &keypair);
        TEST_ASSERT_EQ(err, ESP_OK, "Create keypair for signing");
        
        uint8_t signature[64];
        err = espsol_sign(NULL, 0, &keypair, signature);
        TEST_ASSERT_EQ(err, ESP_OK, "Sign empty message");
        TEST_ASSERT(memcmp(signature, expected_sig, 64) == 0, "Signature matches RFC 8032");
        
        err = espsol_verify(NULL, 0, signature, keypair.public_key);
        TEST_ASSERT_EQ(err, ESP_OK, "Verify empty message signature");
        
        espsol_keypair_clear(&keypair);
    }

    /* Test: Sign and verify arbitrary message */
    {
        espsol_keypair_t keypair;
        err = espsol_keypair_generate(&keypair);
        TEST_ASSERT_EQ(err, ESP_OK, "Generate keypair for message signing");
        
        const char *message = "Hello, Solana!";
        uint8_t signature[64];
        
        err = espsol_sign((const uint8_t *)message, strlen(message), &keypair, signature);
        TEST_ASSERT_EQ(err, ESP_OK, "Sign test message");
        
        err = espsol_verify((const uint8_t *)message, strlen(message), signature, keypair.public_key);
        TEST_ASSERT_EQ(err, ESP_OK, "Verify test message signature");
        
        const char *wrong_message = "Wrong message";
        err = espsol_verify((const uint8_t *)wrong_message, strlen(wrong_message), signature, keypair.public_key);
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_SIGNATURE_INVALID, "Reject wrong message");
        
        espsol_keypair_clear(&keypair);
    }
}

/* ============================================================================
 * Test: Crypto Self-Test
 * ========================================================================== */

static void test_crypto_self_test(void)
{
    ESP_LOGI(TAG, "\n========== Crypto Self-Test ==========");
    
    esp_err_t err = espsol_crypto_self_test();
    TEST_ASSERT_EQ(err, ESP_OK, "Crypto self-test passes");
}

/* ============================================================================
 * Test: Transaction Building
 * ========================================================================== */

static void test_transaction_building(void)
{
    ESP_LOGI(TAG, "\n========== Transaction Building Tests ==========");
    
    esp_err_t err;
    espsol_tx_handle_t tx;
    
    /* Create transaction */
    err = espsol_tx_create(&tx);
    TEST_ASSERT_EQ(err, ESP_OK, "Create transaction");
    TEST_ASSERT(tx != NULL, "Transaction handle not NULL");
    
    /* Generate keypair */
    espsol_keypair_t keypair;
    err = espsol_keypair_generate(&keypair);
    TEST_ASSERT_EQ(err, ESP_OK, "Generate keypair");
    
    /* Get keypair address */
    char address[ESPSOL_ADDRESS_MAX_LEN];
    err = espsol_keypair_get_address(&keypair, address, sizeof(address));
    TEST_ASSERT_EQ(err, ESP_OK, "Get keypair address");
    ESP_LOGI(TAG, "  Sender address: %s", address);
    
    /* Set fee payer */
    err = espsol_tx_set_fee_payer(tx, keypair.public_key);
    TEST_ASSERT_EQ(err, ESP_OK, "Set fee payer");
    
    /* Set blockhash (dummy for testing) */
    uint8_t blockhash[32] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    err = espsol_tx_set_recent_blockhash(tx, blockhash);
    TEST_ASSERT_EQ(err, ESP_OK, "Set blockhash");
    
    /* Create recipient */
    uint8_t recipient[32] = {0xAA, 0xBB, 0xCC, 0xDD};
    
    /* Add transfer instruction */
    err = espsol_tx_add_transfer(tx, keypair.public_key, recipient, 1000000000ULL);
    TEST_ASSERT_EQ(err, ESP_OK, "Add SOL transfer instruction");
    
    /* Check instruction count */
    size_t count;
    err = espsol_tx_get_instruction_count(tx, &count);
    TEST_ASSERT_EQ(err, ESP_OK, "Get instruction count");
    TEST_ASSERT_EQ(count, 1, "Has 1 instruction");
    
    /* Add memo */
    err = espsol_tx_add_memo(tx, "ESPSOL Test Transaction");
    TEST_ASSERT_EQ(err, ESP_OK, "Add memo instruction");
    
    err = espsol_tx_get_instruction_count(tx, &count);
    TEST_ASSERT_EQ(count, 2, "Has 2 instructions");
    
    /* Transaction not signed yet */
    TEST_ASSERT(!espsol_tx_is_signed(tx), "Not signed yet");
    
    /* Sign transaction */
    err = espsol_tx_sign(tx, &keypair);
    TEST_ASSERT_EQ(err, ESP_OK, "Sign transaction");
    TEST_ASSERT(espsol_tx_is_signed(tx), "Transaction is signed");
    
    /* Serialize to Base64 */
    char base64[2048];
    err = espsol_tx_to_base64(tx, base64, sizeof(base64));
    TEST_ASSERT_EQ(err, ESP_OK, "Serialize to Base64");
    ESP_LOGI(TAG, "  Base64 length: %d bytes", strlen(base64));
    
    /* Get signature */
    char sig_b58[ESPSOL_SIGNATURE_MAX_LEN];
    err = espsol_tx_get_signature_base58(tx, sig_b58, sizeof(sig_b58));
    TEST_ASSERT_EQ(err, ESP_OK, "Get signature Base58");
    ESP_LOGI(TAG, "  Signature: %.32s...", sig_b58);
    
    /* Clean up */
    espsol_tx_destroy(tx);
    espsol_keypair_clear(&keypair);
}

/* ============================================================================
 * Test: SPL Token Operations
 * ========================================================================== */

static void test_spl_token(void)
{
    ESP_LOGI(TAG, "\n========== SPL Token Tests ==========");
    
    esp_err_t err;
    espsol_tx_handle_t tx;
    
    /* Get native mint address */
    uint8_t native_mint[32];
    err = espsol_get_native_mint(native_mint);
    TEST_ASSERT_EQ(err, ESP_OK, "Get native mint");
    
    /* Create transaction */
    err = espsol_tx_create(&tx);
    TEST_ASSERT_EQ(err, ESP_OK, "Create transaction");
    
    /* Generate keypair */
    espsol_keypair_t keypair;
    err = espsol_keypair_generate(&keypair);
    TEST_ASSERT_EQ(err, ESP_OK, "Generate keypair");
    
    /* Set transaction config */
    err = espsol_tx_set_fee_payer(tx, keypair.public_key);
    TEST_ASSERT_EQ(err, ESP_OK, "Set fee payer");
    
    uint8_t blockhash[32] = {0x99, 0x88, 0x77, 0x66};
    err = espsol_tx_set_recent_blockhash(tx, blockhash);
    TEST_ASSERT_EQ(err, ESP_OK, "Set blockhash");
    
    /* Add token transfer instruction */
    uint8_t source[32] = {0x01};
    uint8_t dest[32] = {0x02};
    err = espsol_tx_add_token_transfer(tx, source, dest, keypair.public_key, 1000000);
    TEST_ASSERT_EQ(err, ESP_OK, "Add token transfer");
    
    /* Add token transfer checked instruction */
    uint8_t mint[32] = {0x03};
    err = espsol_tx_add_token_transfer_checked(tx, source, mint, dest, keypair.public_key, 500000, 6);
    TEST_ASSERT_EQ(err, ESP_OK, "Add token transfer checked");
    
    /* Check instruction count */
    size_t count;
    err = espsol_tx_get_instruction_count(tx, &count);
    TEST_ASSERT_EQ(count, 2, "Has 2 token instructions");
    
    /* Sign and serialize */
    err = espsol_tx_sign(tx, &keypair);
    TEST_ASSERT_EQ(err, ESP_OK, "Sign token transaction");
    
    char base64[2048];
    err = espsol_tx_to_base64(tx, base64, sizeof(base64));
    TEST_ASSERT_EQ(err, ESP_OK, "Serialize token tx to Base64");
    ESP_LOGI(TAG, "  Token tx Base64 length: %d bytes", strlen(base64));
    
    /* Clean up */
    espsol_tx_destroy(tx);
    espsol_keypair_clear(&keypair);
}

/* ============================================================================
 * Test: Lamport Conversions
 * ========================================================================== */

static void test_lamport_conversion(void)
{
    ESP_LOGI(TAG, "\n========== Lamport Conversion Tests ==========");

    /* Test: SOL to lamports */
    {
        uint64_t lamports = espsol_sol_to_lamports(1.0);
        TEST_ASSERT_EQ(lamports, 1000000000ULL, "1 SOL = 1B lamports");
        
        lamports = espsol_sol_to_lamports(0.001);
        TEST_ASSERT_EQ(lamports, 1000000ULL, "0.001 SOL = 1M lamports");
    }

    /* Test: Lamports to SOL */
    {
        double sol = espsol_lamports_to_sol(1000000000ULL);
        TEST_ASSERT(sol > 0.999 && sol < 1.001, "1B lamports = 1 SOL");
        
        sol = espsol_lamports_to_sol(500000000ULL);
        TEST_ASSERT(sol > 0.499 && sol < 0.501, "500M lamports = 0.5 SOL");
    }
}

/* ============================================================================
 * Main Entry Point
 * ========================================================================== */

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║     ESPSOL Complete Test Suite             ║");
    ESP_LOGI(TAG, "║     Version: %s                          ║", ESPSOL_VERSION_STRING);
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

#if RUN_INTEGRATION_TEST
    /* Run the devnet integration test */
    ESP_LOGI(TAG, "Mode: INTEGRATION TEST (Solana Devnet)");
    ESP_LOGI(TAG, "");
    run_integration_test();
    
    ESP_LOGI(TAG, "Integration test complete. Entering idle loop...");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
#else
    /* Run all unit tests */
    ESP_LOGI(TAG, "Mode: UNIT TESTS (offline)");
    ESP_LOGI(TAG, "");
    
    test_core_sdk();
    test_base58();
    test_base64();
    test_address_utils();
    test_hex();
    test_crypto_keypair();
    test_crypto_signing();
    test_crypto_self_test();
    test_transaction_building();
    test_spl_token();
    test_lamport_conversion();

    /* Print summary */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║            TEST SUMMARY                    ║");
    ESP_LOGI(TAG, "╠════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║  Passed: %-3d                               ║", tests_passed);
    ESP_LOGI(TAG, "║  Failed: %-3d                               ║", tests_failed);
    ESP_LOGI(TAG, "║  Total:  %-3d                               ║", tests_passed + tests_failed);
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");

    if (tests_failed == 0) {
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "🎉 ALL TESTS PASSED! 🎉");
    } else {
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "❌ SOME TESTS FAILED!");
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Test complete. Entering idle loop...");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
#endif /* RUN_INTEGRATION_TEST */
}
