/**
 * @file test_encoding.c
 * @brief Host-based Unit Tests for ESPSOL
 *
 * This file can be compiled and run on the host machine (Linux/macOS/Windows)
 * to test Base58, Base64, and crypto functions without needing ESP32 hardware.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Include ESPSOL headers (they handle host compatibility) */
#include "espsol_types.h"
#include "espsol_utils.h"
#include "espsol_crypto.h"

/* ============================================================================
 * Test Framework
 * ========================================================================== */

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("✓ PASS: %s\n", message); \
            tests_passed++; \
        } else { \
            printf("✗ FAIL: %s\n", message); \
            tests_failed++; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) == (expected)) { \
            printf("✓ PASS: %s\n", message); \
            tests_passed++; \
        } else { \
            printf("✗ FAIL: %s (expected %d, got %d)\n", message, (int)(expected), (int)(actual)); \
            tests_failed++; \
        } \
    } while (0)

#define TEST_ASSERT_STR_EQ(actual, expected, message) \
    do { \
        if (strcmp((actual), (expected)) == 0) { \
            printf("✓ PASS: %s\n", message); \
            tests_passed++; \
        } else { \
            printf("✗ FAIL: %s\n  expected: '%s'\n  got:      '%s'\n", message, (expected), (actual)); \
            tests_failed++; \
        } \
    } while (0)

/* ============================================================================
 * Base58 Tests
 * ========================================================================== */

static void test_base58_encode_decode(void)
{
    printf("\n========== Base58 Tests ==========\n\n");
    
    esp_err_t err;
    char encoded[128];
    uint8_t decoded[64];
    size_t decoded_len;

    /* Test 1: System Program (32 zeros) */
    {
        const uint8_t system_program[32] = { 0 };
        err = espsol_base58_encode(system_program, 32, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Encode System Program");
        TEST_ASSERT_STR_EQ(encoded, "11111111111111111111111111111111", "System Program = 32 ones");
        
        decoded_len = sizeof(decoded);
        err = espsol_base58_decode(encoded, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Decode System Program");
        TEST_ASSERT_EQ(decoded_len, 32, "Decoded length is 32");
        TEST_ASSERT(memcmp(system_program, decoded, 32) == 0, "Roundtrip matches");
    }

    /* Test 2: Simple bytes */
    {
        const uint8_t data[] = { 0x00, 0x01, 0x02, 0x03 };
        err = espsol_base58_encode(data, sizeof(data), encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Encode simple bytes");
        printf("  Encoded [00,01,02,03]: %s\n", encoded);
        
        decoded_len = sizeof(decoded);
        err = espsol_base58_decode(encoded, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Decode simple bytes");
        TEST_ASSERT(memcmp(data, decoded, sizeof(data)) == 0, "Roundtrip simple bytes");
    }

    /* Test 3: Known test vector */
    {
        /* "Hello World!" in hex: 48656c6c6f20576f726c6421 */
        const uint8_t hello[] = { 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 
                                   0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21 };
        err = espsol_base58_encode(hello, sizeof(hello), encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Encode 'Hello World!'");
        printf("  'Hello World!' in Base58: %s\n", encoded);
        /* Known answer: 2NEpo7TZRRrLZSi2U */
        TEST_ASSERT_STR_EQ(encoded, "2NEpo7TZRRrLZSi2U", "Known test vector");
    }

    /* Test 4: Random 32-byte key roundtrip */
    {
        const uint8_t key[32] = {
            0x0e, 0xc7, 0x8f, 0x5e, 0x2b, 0x85, 0xd0, 0x9c,
            0x4a, 0x1b, 0x3f, 0x6d, 0x2e, 0x7c, 0x8a, 0x5b,
            0x9d, 0x4e, 0x1f, 0x0a, 0x3c, 0x7b, 0x6d, 0x2e,
            0x8f, 0x5c, 0x9a, 0x1b, 0x4d, 0x0e, 0x7f, 0x3a
        };
        
        err = espsol_base58_encode(key, 32, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Encode 32-byte key");
        printf("  32-byte key address: %s\n", encoded);
        
        decoded_len = sizeof(decoded);
        err = espsol_base58_decode(encoded, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Decode 32-byte key");
        TEST_ASSERT_EQ(decoded_len, 32, "Decoded length is 32");
        TEST_ASSERT(memcmp(key, decoded, 32) == 0, "32-byte key roundtrip");
    }

    /* Test 5: Empty input */
    {
        err = espsol_base58_encode((const uint8_t *)"", 0, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Encode empty");
        TEST_ASSERT_STR_EQ(encoded, "", "Empty encodes to empty");
    }

    /* Test 6: Invalid characters */
    {
        decoded_len = sizeof(decoded);
        err = espsol_base58_decode("0invalid", decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE58, "Detect invalid '0'");
        
        err = espsol_base58_decode("Oinvalid", decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE58, "Detect invalid 'O'");
        
        err = espsol_base58_decode("Iinvalid", decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE58, "Detect invalid 'I'");
        
        err = espsol_base58_decode("linvalid", decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE58, "Detect invalid 'l'");
    }

    /* Test 7: Buffer too small */
    {
        const uint8_t data[] = { 0x01, 0x02, 0x03 };
        err = espsol_base58_encode(data, sizeof(data), encoded, 2);
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_BUFFER_TOO_SMALL, "Detect buffer too small");
    }
}

/* ============================================================================
 * Base64 Tests
 * ========================================================================== */

static void test_base64_encode_decode(void)
{
    printf("\n========== Base64 Tests ==========\n\n");
    
    esp_err_t err;
    char encoded[512];    /* Enough for 256 bytes -> ~344 chars */
    uint8_t decoded[256]; /* Enough to decode 256 bytes */
    size_t decoded_len;

    /* Test 1: Standard test vectors from RFC 4648 */
    {
        struct {
            const char *input;
            const char *expected;
        } vectors[] = {
            { "", "" },
            { "f", "Zg==" },
            { "fo", "Zm8=" },
            { "foo", "Zm9v" },
            { "foob", "Zm9vYg==" },
            { "fooba", "Zm9vYmE=" },
            { "foobar", "Zm9vYmFy" },
        };
        
        for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
            err = espsol_base64_encode((const uint8_t *)vectors[i].input, 
                                        strlen(vectors[i].input), 
                                        encoded, sizeof(encoded));
            TEST_ASSERT_EQ(err, ESP_OK, "Encode RFC 4648 vector");
            
            char msg[64];
            snprintf(msg, sizeof(msg), "'%s' -> '%s'", vectors[i].input, vectors[i].expected);
            TEST_ASSERT_STR_EQ(encoded, vectors[i].expected, msg);
        }
    }

    /* Test 2: Decode roundtrip */
    {
        const char *test_str = "Hello, Solana!";
        err = espsol_base64_encode((const uint8_t *)test_str, strlen(test_str), 
                                    encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Encode 'Hello, Solana!'");
        printf("  'Hello, Solana!' -> %s\n", encoded);
        
        decoded_len = sizeof(decoded);
        err = espsol_base64_decode(encoded, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Decode 'Hello, Solana!'");
        decoded[decoded_len] = '\0';
        TEST_ASSERT_STR_EQ((char *)decoded, test_str, "Roundtrip matches");
    }

    /* Test 3: Binary data */
    {
        const uint8_t binary[] = { 0x00, 0xFF, 0x80, 0x7F, 0x01, 0xFE };
        err = espsol_base64_encode(binary, sizeof(binary), encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Encode binary data");
        printf("  Binary [00,FF,80,7F,01,FE] -> %s\n", encoded);
        
        decoded_len = sizeof(decoded);
        err = espsol_base64_decode(encoded, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Decode binary data");
        TEST_ASSERT_EQ(decoded_len, sizeof(binary), "Binary length matches");
        TEST_ASSERT(memcmp(binary, decoded, sizeof(binary)) == 0, "Binary roundtrip");
    }

    /* Test 4: Large data (simulating transaction) */
    {
        uint8_t tx_data[256];
        for (int i = 0; i < 256; i++) {
            tx_data[i] = (uint8_t)i;
        }
        
        err = espsol_base64_encode(tx_data, 256, encoded, sizeof(encoded));
        TEST_ASSERT_EQ(err, ESP_OK, "Encode 256 bytes");
        
        decoded_len = sizeof(decoded);
        err = espsol_base64_decode(encoded, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Decode 256 bytes");
        TEST_ASSERT_EQ(decoded_len, 256, "Decoded 256 bytes");
        TEST_ASSERT(memcmp(tx_data, decoded, 256) == 0, "256-byte roundtrip");
    }

    /* Test 5: Invalid Base64 */
    {
        decoded_len = sizeof(decoded);
        err = espsol_base64_decode("abc", decoded, &decoded_len);  /* Not multiple of 4 */
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE64, "Detect invalid length");
        
        err = espsol_base64_decode("ab!d", decoded, &decoded_len);  /* Invalid char */
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE64, "Detect invalid char");
    }
}

/* ============================================================================
 * Address Utility Tests
 * ========================================================================== */

static void test_address_utils(void)
{
    printf("\n========== Address Utility Tests ==========\n\n");
    
    esp_err_t err;
    char address[ESPSOL_ADDRESS_MAX_LEN];
    uint8_t pubkey[ESPSOL_PUBKEY_SIZE];

    /* Test 1: System Program */
    {
        const uint8_t system_pubkey[32] = { 0 };
        err = espsol_pubkey_to_address(system_pubkey, address, sizeof(address));
        TEST_ASSERT_EQ(err, ESP_OK, "Convert System Program pubkey");
        TEST_ASSERT_STR_EQ(address, "11111111111111111111111111111111", "System Program address");
    }

    /* Test 2: Roundtrip */
    {
        const uint8_t test_pubkey[32] = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
            0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
            0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
        };
        
        err = espsol_pubkey_to_address(test_pubkey, address, sizeof(address));
        TEST_ASSERT_EQ(err, ESP_OK, "Convert test pubkey to address");
        printf("  Test address: %s\n", address);
        
        err = espsol_address_to_pubkey(address, pubkey);
        TEST_ASSERT_EQ(err, ESP_OK, "Convert address to pubkey");
        TEST_ASSERT(memcmp(test_pubkey, pubkey, 32) == 0, "Pubkey roundtrip");
    }

    /* Test 3: Validity checks */
    {
        TEST_ASSERT(espsol_is_valid_address("11111111111111111111111111111111") == true, 
                    "System Program is valid");
        TEST_ASSERT(espsol_is_valid_address("short") == false, 
                    "Short string is invalid");
        TEST_ASSERT(espsol_is_valid_address("0000000000000000000000000000000") == false, 
                    "String with '0' is invalid");
        TEST_ASSERT(espsol_is_valid_address(NULL) == false, 
                    "NULL is invalid");
    }
}

/* ============================================================================
 * Hex Utility Tests
 * ========================================================================== */

static void test_hex_utils(void)
{
    printf("\n========== Hex Utility Tests ==========\n\n");
    
    esp_err_t err;
    char hex[128];
    uint8_t decoded[64];
    size_t decoded_len;

    /* Test 1: Basic encoding */
    {
        const uint8_t data[] = { 0xDE, 0xAD, 0xBE, 0xEF };
        err = espsol_hex_encode(data, sizeof(data), hex, sizeof(hex));
        TEST_ASSERT_EQ(err, ESP_OK, "Encode deadbeef");
        TEST_ASSERT_STR_EQ(hex, "deadbeef", "Hex output correct");
    }

    /* Test 2: Roundtrip */
    {
        const uint8_t data[] = { 0x00, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xFF };
        err = espsol_hex_encode(data, sizeof(data), hex, sizeof(hex));
        TEST_ASSERT_EQ(err, ESP_OK, "Encode mixed bytes");
        
        decoded_len = sizeof(decoded);
        err = espsol_hex_decode(hex, decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Decode hex");
        TEST_ASSERT_EQ(decoded_len, sizeof(data), "Decoded length matches");
        TEST_ASSERT(memcmp(data, decoded, sizeof(data)) == 0, "Hex roundtrip");
    }

    /* Test 3: Uppercase decode */
    {
        decoded_len = sizeof(decoded);
        err = espsol_hex_decode("DEADBEEF", decoded, &decoded_len);
        TEST_ASSERT_EQ(err, ESP_OK, "Decode uppercase hex");
        TEST_ASSERT_EQ(decoded[0], 0xDE, "First byte correct");
        TEST_ASSERT_EQ(decoded[3], 0xEF, "Last byte correct");
    }
}

/* ============================================================================
 * Crypto Tests
 * ========================================================================== */

static void test_crypto_keypair(void)
{
    printf("\n========== Crypto Keypair Tests ==========\n\n");
    
    esp_err_t err;

    /* Initialize crypto subsystem */
    err = espsol_crypto_init();
    TEST_ASSERT_EQ(err, ESP_OK, "Crypto init");

    /* Test 1: Generate keypair from known seed (RFC 8032 test vector) */
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
        
        /* Get address */
        char address[ESPSOL_ADDRESS_MAX_LEN];
        err = espsol_keypair_get_address(&keypair, address, sizeof(address));
        TEST_ASSERT_EQ(err, ESP_OK, "Get keypair address");
        printf("  Test keypair address: %s\n", address);
        
        espsol_keypair_clear(&keypair);
        TEST_ASSERT(!keypair.initialized, "Keypair cleared");
    }

    /* Test 2: Generate random keypair */
    {
        espsol_keypair_t keypair;
        err = espsol_keypair_generate(&keypair);
        TEST_ASSERT_EQ(err, ESP_OK, "Generate random keypair");
        TEST_ASSERT(keypair.initialized, "Random keypair initialized");
        
        char address[ESPSOL_ADDRESS_MAX_LEN];
        err = espsol_keypair_get_address(&keypair, address, sizeof(address));
        TEST_ASSERT_EQ(err, ESP_OK, "Get random keypair address");
        printf("  Random keypair address: %s\n", address);
        
        espsol_keypair_clear(&keypair);
    }

    /* Test 3: Export/import keypair via Base58 */
    {
        espsol_keypair_t keypair1, keypair2;
        char base58_key[ESPSOL_PRIVKEY_BASE58_LEN];
        
        err = espsol_keypair_generate(&keypair1);
        TEST_ASSERT_EQ(err, ESP_OK, "Generate keypair for export");
        
        err = espsol_keypair_to_base58(&keypair1, base58_key, sizeof(base58_key));
        TEST_ASSERT_EQ(err, ESP_OK, "Export keypair to Base58");
        printf("  Exported key length: %zu chars\n", strlen(base58_key));
        
        err = espsol_keypair_from_base58(base58_key, &keypair2);
        TEST_ASSERT_EQ(err, ESP_OK, "Import keypair from Base58");
        
        TEST_ASSERT(memcmp(keypair1.public_key, keypair2.public_key, 32) == 0, "Imported public key matches");
        TEST_ASSERT(memcmp(keypair1.private_key, keypair2.private_key, 64) == 0, "Imported private key matches");
        
        espsol_keypair_clear(&keypair1);
        espsol_keypair_clear(&keypair2);
    }
}

static void test_crypto_signing(void)
{
    printf("\n========== Crypto Signing Tests ==========\n\n");
    
    esp_err_t err;

    /* Test 1: Sign and verify with RFC 8032 test vector */
    {
        const uint8_t test_seed[32] = {
            0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60,
            0xba, 0x84, 0x4a, 0xf4, 0x92, 0xec, 0x2c, 0xc4,
            0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32, 0x69, 0x19,
            0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60
        };
        
        /* Expected signature for empty message from RFC 8032 */
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
        
        /* Sign empty message */
        uint8_t signature[64];
        err = espsol_sign(NULL, 0, &keypair, signature);
        TEST_ASSERT_EQ(err, ESP_OK, "Sign empty message");
        TEST_ASSERT(memcmp(signature, expected_sig, 64) == 0, "Signature matches RFC 8032");
        
        /* Verify the signature */
        err = espsol_verify(NULL, 0, signature, keypair.public_key);
        TEST_ASSERT_EQ(err, ESP_OK, "Verify empty message signature");
        
        espsol_keypair_clear(&keypair);
    }

    /* Test 2: Sign and verify arbitrary message */
    {
        espsol_keypair_t keypair;
        err = espsol_keypair_generate(&keypair);
        TEST_ASSERT_EQ(err, ESP_OK, "Generate keypair for message signing");
        
        const char *message = "Hello, Solana! This is a test message.";
        uint8_t signature[64];
        
        err = espsol_sign((const uint8_t *)message, strlen(message), &keypair, signature);
        TEST_ASSERT_EQ(err, ESP_OK, "Sign test message");
        
        err = espsol_verify((const uint8_t *)message, strlen(message), signature, keypair.public_key);
        TEST_ASSERT_EQ(err, ESP_OK, "Verify test message signature");
        
        /* Verify with wrong message should fail */
        const char *wrong_message = "Wrong message";
        err = espsol_verify((const uint8_t *)wrong_message, strlen(wrong_message), signature, keypair.public_key);
        TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_SIGNATURE_INVALID, "Reject wrong message");
        
        espsol_keypair_clear(&keypair);
    }

    /* Test 3: Sign binary data */
    {
        espsol_keypair_t keypair;
        err = espsol_keypair_generate(&keypair);
        TEST_ASSERT_EQ(err, ESP_OK, "Generate keypair for binary signing");
        
        uint8_t binary_data[256];
        for (int i = 0; i < 256; i++) {
            binary_data[i] = (uint8_t)i;
        }
        
        uint8_t signature[64];
        err = espsol_sign(binary_data, sizeof(binary_data), &keypair, signature);
        TEST_ASSERT_EQ(err, ESP_OK, "Sign 256 bytes");
        
        err = espsol_verify(binary_data, sizeof(binary_data), signature, keypair.public_key);
        TEST_ASSERT_EQ(err, ESP_OK, "Verify 256-byte signature");
        
        espsol_keypair_clear(&keypair);
    }
}

static void test_crypto_self_test(void)
{
    printf("\n========== Crypto Self-Test ==========\n\n");
    
    esp_err_t err = espsol_crypto_self_test();
    TEST_ASSERT_EQ(err, ESP_OK, "Crypto self-test passes");
}

/* ============================================================================
 * Main
 * ========================================================================== */

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════╗\n");
    printf("║     ESPSOL Host Unit Tests                 ║\n");
    printf("║     Encoding & Crypto Functions            ║\n");
    printf("╚════════════════════════════════════════════╝\n");

    test_base58_encode_decode();
    test_base64_encode_decode();
    test_address_utils();
    test_hex_utils();
    test_crypto_keypair();
    test_crypto_signing();
    test_crypto_self_test();

    printf("\n");
    printf("╔════════════════════════════════════════════╗\n");
    printf("║            TEST SUMMARY                    ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║  Passed: %-3d                               ║\n", tests_passed);
    printf("║  Failed: %-3d                               ║\n", tests_failed);
    printf("║  Total:  %-3d                               ║\n", tests_passed + tests_failed);
    printf("╚════════════════════════════════════════════╝\n");

    if (tests_failed == 0) {
        printf("\n🎉 ALL TESTS PASSED! 🎉\n\n");
        return 0;
    } else {
        printf("\n❌ SOME TESTS FAILED!\n\n");
        return 1;
    }
}
