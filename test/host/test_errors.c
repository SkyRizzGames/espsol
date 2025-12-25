/**
 * @file test_errors.c
 * @brief ESPSOL Error Handling Unit Tests
 *
 * Tests for error code coverage, error path validation, and
 * error-to-name conversion.
 *
 * @copyright Copyright (c) 2025 Rizky Ariestiyansyah
 * @license Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Include ESPSOL headers */
#include "../../components/espsol/include/espsol_types.h"
#include "../../components/espsol/include/espsol_utils.h"
#include "../../components/espsol/include/espsol_crypto.h"
#include "../../components/espsol/include/espsol_tx.h"

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test macros */
#define TEST_ASSERT(condition, msg) do { \
    if (condition) { \
        tests_passed++; \
        printf("✓ PASS: %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("✗ FAIL: %s\n", msg); \
    } \
} while(0)

#define TEST_ASSERT_EQ(a, b, msg) TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_NE(a, b, msg) TEST_ASSERT((a) != (b), msg)

/* ============================================================================
 * Error Code Validation Tests
 * ========================================================================== */

static void test_error_code_values(void)
{
    printf("\n========== Error Code Value Tests ==========\n\n");
    
    /* Verify base value */
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_BASE, 0x50000, "Error base is 0x50000");
    
    /* Verify all error codes are sequential and unique */
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_INVALID_ARG, 0x50001, "INVALID_ARG = 0x50001");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_BUFFER_TOO_SMALL, 0x50002, "BUFFER_TOO_SMALL = 0x50002");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_ENCODING_FAILED, 0x50003, "ENCODING_FAILED = 0x50003");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_INVALID_BASE58, 0x50004, "INVALID_BASE58 = 0x50004");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_INVALID_BASE64, 0x50005, "INVALID_BASE64 = 0x50005");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT, 0x50006, "KEYPAIR_NOT_INIT = 0x50006");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_SIGNATURE_INVALID, 0x50007, "SIGNATURE_INVALID = 0x50007");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_RPC_FAILED, 0x50008, "RPC_FAILED = 0x50008");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_RPC_PARSE_ERROR, 0x50009, "RPC_PARSE_ERROR = 0x50009");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_TX_BUILD_ERROR, 0x5000A, "TX_BUILD_ERROR = 0x5000A");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_TX_NOT_SIGNED, 0x5000B, "TX_NOT_SIGNED = 0x5000B");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_MAX_ACCOUNTS, 0x5000C, "MAX_ACCOUNTS = 0x5000C");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_MAX_INSTRUCTIONS, 0x5000D, "MAX_INSTRUCTIONS = 0x5000D");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_NVS_ERROR, 0x5000E, "NVS_ERROR = 0x5000E");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_CRYPTO_ERROR, 0x5000F, "CRYPTO_ERROR = 0x5000F");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_NETWORK_ERROR, 0x50010, "NETWORK_ERROR = 0x50010");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_TIMEOUT, 0x50011, "TIMEOUT = 0x50011");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_NOT_INITIALIZED, 0x50012, "NOT_INITIALIZED = 0x50012");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_RATE_LIMITED, 0x50013, "RATE_LIMITED = 0x50013");
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_INVALID_MNEMONIC, 0x50014, "INVALID_MNEMONIC = 0x50014");
    
    /* Verify MAX is set correctly */
    TEST_ASSERT_EQ(ESP_ERR_ESPSOL_MAX, ESP_ERR_ESPSOL_INVALID_MNEMONIC, "MAX error equals INVALID_MNEMONIC");
}

static void test_espsol_is_err_macro(void)
{
    printf("\n========== ESPSOL_IS_ERR Macro Tests ==========\n\n");
    
    /* Test valid ESPSOL errors */
    TEST_ASSERT(ESPSOL_IS_ERR(ESP_ERR_ESPSOL_INVALID_ARG), "INVALID_ARG is ESPSOL error");
    TEST_ASSERT(ESPSOL_IS_ERR(ESP_ERR_ESPSOL_BUFFER_TOO_SMALL), "BUFFER_TOO_SMALL is ESPSOL error");
    TEST_ASSERT(ESPSOL_IS_ERR(ESP_ERR_ESPSOL_RATE_LIMITED), "RATE_LIMITED is ESPSOL error");
    TEST_ASSERT(ESPSOL_IS_ERR(ESP_ERR_ESPSOL_CRYPTO_ERROR), "CRYPTO_ERROR is ESPSOL error");
    TEST_ASSERT(ESPSOL_IS_ERR(ESP_ERR_ESPSOL_INVALID_MNEMONIC), "INVALID_MNEMONIC is ESPSOL error");
    
    /* Test non-ESPSOL errors */
    TEST_ASSERT(!ESPSOL_IS_ERR(ESP_OK), "ESP_OK is not ESPSOL error");
    TEST_ASSERT(!ESPSOL_IS_ERR(ESP_FAIL), "ESP_FAIL is not ESPSOL error");
    TEST_ASSERT(!ESPSOL_IS_ERR(ESP_ERR_INVALID_ARG), "ESP_ERR_INVALID_ARG is not ESPSOL error");
    TEST_ASSERT(!ESPSOL_IS_ERR(ESP_ERR_NO_MEM), "ESP_ERR_NO_MEM is not ESPSOL error");
    
    /* Test edge cases */
    TEST_ASSERT(!ESPSOL_IS_ERR(ESP_ERR_ESPSOL_BASE), "BASE alone is not ESPSOL error");
    TEST_ASSERT(!ESPSOL_IS_ERR(ESP_ERR_ESPSOL_BASE + 0x15), "Beyond MAX is not ESPSOL error");
    TEST_ASSERT(!ESPSOL_IS_ERR(0), "Zero is not ESPSOL error");
}

/* ============================================================================
 * NULL Argument Error Tests (Utils)
 * ========================================================================== */

static void test_base58_null_errors(void)
{
    printf("\n========== Base58 NULL Argument Tests ==========\n\n");
    
    uint8_t data[32] = {0};
    char output[64];
    size_t out_len;
    esp_err_t err;
    
    /* Encode NULL tests */
    err = espsol_base58_encode(NULL, 32, output, sizeof(output));
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Encode NULL data returns INVALID_ARG");
    
    err = espsol_base58_encode(data, 32, NULL, sizeof(output));
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Encode NULL output returns INVALID_ARG");
    
    /* Decode NULL tests */
    err = espsol_base58_decode(NULL, data, &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Decode NULL input returns INVALID_ARG");
    
    err = espsol_base58_decode("test", NULL, &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Decode NULL output returns INVALID_ARG");
    
    err = espsol_base58_decode("test", data, NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Decode NULL len returns INVALID_ARG");
}

static void test_base64_null_errors(void)
{
    printf("\n========== Base64 NULL Argument Tests ==========\n\n");
    
    uint8_t data[32] = {0};
    char output[64];
    size_t out_len;
    esp_err_t err;
    
    /* Encode NULL tests */
    err = espsol_base64_encode(NULL, 32, output, sizeof(output));
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Encode NULL data returns INVALID_ARG");
    
    err = espsol_base64_encode(data, 32, NULL, sizeof(output));
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Encode NULL output returns INVALID_ARG");
    
    /* Decode NULL tests */
    err = espsol_base64_decode(NULL, data, &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Decode NULL input returns INVALID_ARG");
    
    err = espsol_base64_decode("dGVzdA==", NULL, &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Decode NULL output returns INVALID_ARG");
    
    err = espsol_base64_decode("dGVzdA==", data, NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Decode NULL len returns INVALID_ARG");
}

static void test_address_null_errors(void)
{
    printf("\n========== Address Utils NULL Argument Tests ==========\n\n");
    
    uint8_t pubkey[32] = {0};
    char address[64];
    esp_err_t err;
    
    /* pubkey_to_address NULL tests */
    err = espsol_pubkey_to_address(NULL, address, sizeof(address));
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "pubkey_to_address NULL pubkey returns INVALID_ARG");
    
    err = espsol_pubkey_to_address(pubkey, NULL, sizeof(address));
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "pubkey_to_address NULL address returns INVALID_ARG");
    
    /* address_to_pubkey NULL tests */
    err = espsol_address_to_pubkey(NULL, pubkey);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "address_to_pubkey NULL address returns INVALID_ARG");
    
    err = espsol_address_to_pubkey("11111111111111111111111111111111", NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "address_to_pubkey NULL pubkey returns INVALID_ARG");
    
    /* is_valid_address NULL test */
    bool valid = espsol_is_valid_address(NULL);
    TEST_ASSERT(!valid, "is_valid_address NULL returns false");
}

/* ============================================================================
 * NULL Argument Error Tests (Crypto)
 * ========================================================================== */

static void test_crypto_null_errors(void)
{
    printf("\n========== Crypto NULL Argument Tests ==========\n\n");
    
    espsol_keypair_t keypair;
    uint8_t seed[32] = {0};
    uint8_t signature[64];
    uint8_t message[] = "test";
    char address[64];
    esp_err_t err;
    
    /* Initialize crypto */
    espsol_crypto_init();
    
    /* keypair_init NULL */
    err = espsol_keypair_init(NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "keypair_init NULL returns INVALID_ARG");
    
    /* keypair_generate NULL */
    err = espsol_keypair_generate(NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "keypair_generate NULL returns INVALID_ARG");
    
    /* keypair_from_seed NULL tests */
    err = espsol_keypair_from_seed(NULL, &keypair);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "keypair_from_seed NULL seed returns INVALID_ARG");
    
    err = espsol_keypair_from_seed(seed, NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "keypair_from_seed NULL keypair returns INVALID_ARG");
    
    /* keypair_clear NULL */
    err = espsol_keypair_clear(NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "keypair_clear NULL returns INVALID_ARG");
    
    /* Generate a valid keypair for remaining tests */
    espsol_keypair_from_seed(seed, &keypair);
    
    /* keypair_get_address NULL tests */
    err = espsol_keypair_get_address(NULL, address, sizeof(address));
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "keypair_get_address NULL keypair returns INVALID_ARG");
    
    err = espsol_keypair_get_address(&keypair, NULL, sizeof(address));
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "keypair_get_address NULL address returns INVALID_ARG");
    
    /* espsol_sign NULL tests */
    err = espsol_sign(NULL, sizeof(message), &keypair, signature);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "sign NULL message returns INVALID_ARG");
    
    err = espsol_sign(message, sizeof(message), NULL, signature);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "sign NULL keypair returns INVALID_ARG");
    
    err = espsol_sign(message, sizeof(message), &keypair, NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "sign NULL signature returns INVALID_ARG");
    
    /* random_bytes NULL */
    err = espsol_random_bytes(NULL, 32);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "random_bytes NULL returns INVALID_ARG");
}

static void test_crypto_uninitialized_keypair(void)
{
    printf("\n========== Uninitialized Keypair Tests ==========\n\n");
    
    espsol_keypair_t keypair;
    char address[64];
    uint8_t signature[64];
    uint8_t message[] = "test";
    esp_err_t err;
    
    /* Initialize but don't generate */
    espsol_keypair_init(&keypair);
    
    /* Verify initialized flag is false */
    TEST_ASSERT(!espsol_keypair_is_initialized(&keypair), "Initialized keypair shows not initialized");
    
    /* Try to get address from uninitialized keypair */
    err = espsol_keypair_get_address(&keypair, address, sizeof(address));
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT, "get_address uninitialized returns KEYPAIR_NOT_INIT");
    
    /* Try to sign with uninitialized keypair */
    err = espsol_sign(message, sizeof(message), &keypair, signature);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT, "sign uninitialized returns KEYPAIR_NOT_INIT");
}

/* ============================================================================
 * NULL Argument Error Tests (Transaction)
 * ========================================================================== */

static void test_transaction_null_errors(void)
{
    printf("\n========== Transaction NULL Argument Tests ==========\n\n");
    
    espsol_tx_handle_t tx = NULL;
    uint8_t pubkey[32] = {0};
    uint8_t blockhash[32] = {0};
    esp_err_t err;
    
    /* espsol_tx_create NULL */
    err = espsol_tx_create(NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "tx_create NULL returns INVALID_ARG");
    
    /* espsol_tx_destroy NULL */
    err = espsol_tx_destroy(NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "tx_destroy NULL returns INVALID_ARG");
    
    /* Create a valid transaction for remaining tests */
    espsol_tx_create(&tx);
    
    /* espsol_tx_set_fee_payer NULL tests */
    err = espsol_tx_set_fee_payer(NULL, pubkey);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "set_fee_payer NULL tx returns INVALID_ARG");
    
    err = espsol_tx_set_fee_payer(tx, NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "set_fee_payer NULL pubkey returns INVALID_ARG");
    
    /* espsol_tx_set_recent_blockhash NULL tests */
    err = espsol_tx_set_recent_blockhash(NULL, blockhash);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "set_blockhash NULL tx returns INVALID_ARG");
    
    err = espsol_tx_set_recent_blockhash(tx, NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "set_blockhash NULL blockhash returns INVALID_ARG");
    
    /* espsol_tx_add_transfer NULL tests */
    err = espsol_tx_add_transfer(NULL, pubkey, pubkey, 1000);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "add_transfer NULL tx returns INVALID_ARG");
    
    err = espsol_tx_add_transfer(tx, NULL, pubkey, 1000);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "add_transfer NULL from returns INVALID_ARG");
    
    err = espsol_tx_add_transfer(tx, pubkey, NULL, 1000);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "add_transfer NULL to returns INVALID_ARG");
    
    /* espsol_tx_add_memo NULL tests */
    err = espsol_tx_add_memo(NULL, "test");
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "add_memo NULL tx returns INVALID_ARG");
    
    err = espsol_tx_add_memo(tx, NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "add_memo NULL memo returns INVALID_ARG");
    
    /* Cleanup */
    espsol_tx_destroy(tx);
}

static void test_transaction_unsigned_errors(void)
{
    printf("\n========== Unsigned Transaction Error Tests ==========\n\n");
    
    espsol_tx_handle_t tx = NULL;
    uint8_t buffer[1024];
    char base64[2048];
    size_t out_len;
    esp_err_t err;
    
    /* Create transaction but don't sign */
    espsol_tx_create(&tx);
    
    /* Check is_signed returns false */
    TEST_ASSERT(!espsol_tx_is_signed(tx), "New transaction is not signed");
    
    /* Try to serialize unsigned transaction */
    err = espsol_tx_serialize(tx, buffer, sizeof(buffer), &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_TX_NOT_SIGNED, "serialize unsigned returns TX_NOT_SIGNED");
    
    /* Try to convert to base64 unsigned */
    err = espsol_tx_to_base64(tx, base64, sizeof(base64));
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_TX_NOT_SIGNED, "to_base64 unsigned returns TX_NOT_SIGNED");
    
    /* Cleanup */
    espsol_tx_destroy(tx);
}

/* ============================================================================
 * Buffer Too Small Tests
 * ========================================================================== */

static void test_buffer_too_small_errors(void)
{
    printf("\n========== Buffer Too Small Tests ==========\n\n");
    
    uint8_t data[32] = {1, 2, 3, 4, 5};
    char small_output[5];
    uint8_t small_decoded[2];
    size_t out_len;
    esp_err_t err;
    
    /* Base58 encode with small buffer */
    err = espsol_base58_encode(data, 32, small_output, sizeof(small_output));
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_BUFFER_TOO_SMALL, "Base58 encode small buffer returns BUFFER_TOO_SMALL");
    
    /* Base64 encode with small buffer */
    err = espsol_base64_encode(data, 32, small_output, sizeof(small_output));
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_BUFFER_TOO_SMALL, "Base64 encode small buffer returns BUFFER_TOO_SMALL");
    
    /* Base58 decode with small buffer */
    out_len = sizeof(small_decoded);
    err = espsol_base58_decode("11111111111111111111111111111111", small_decoded, &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_BUFFER_TOO_SMALL, "Base58 decode small buffer returns BUFFER_TOO_SMALL");
    
    /* pubkey_to_address with small buffer */
    err = espsol_pubkey_to_address(data, small_output, sizeof(small_output));
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_BUFFER_TOO_SMALL, "pubkey_to_address small buffer returns BUFFER_TOO_SMALL");
}

/* ============================================================================
 * Invalid Input Tests
 * ========================================================================== */

static void test_invalid_base58_errors(void)
{
    printf("\n========== Invalid Base58 Input Tests ==========\n\n");
    
    uint8_t output[64];
    size_t out_len;
    esp_err_t err;
    
    /* Invalid character '0' */
    out_len = sizeof(output);
    err = espsol_base58_decode("0ABC", output, &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE58, "Decode '0' returns INVALID_BASE58");
    
    /* Invalid character 'O' */
    out_len = sizeof(output);
    err = espsol_base58_decode("OABC", output, &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE58, "Decode 'O' returns INVALID_BASE58");
    
    /* Invalid character 'I' */
    out_len = sizeof(output);
    err = espsol_base58_decode("IABC", output, &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE58, "Decode 'I' returns INVALID_BASE58");
    
    /* Invalid character 'l' */
    out_len = sizeof(output);
    err = espsol_base58_decode("labc", output, &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE58, "Decode 'l' returns INVALID_BASE58");
}

static void test_invalid_base64_errors(void)
{
    printf("\n========== Invalid Base64 Input Tests ==========\n\n");
    
    uint8_t output[64];
    size_t out_len;
    esp_err_t err;
    
    /* Invalid length (not multiple of 4) */
    out_len = sizeof(output);
    err = espsol_base64_decode("abc", output, &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE64, "Decode invalid length returns INVALID_BASE64");
    
    /* Invalid character */
    out_len = sizeof(output);
    err = espsol_base64_decode("ab@d", output, &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_INVALID_BASE64, "Decode invalid char returns INVALID_BASE64");
}

static void test_signature_verification_errors(void)
{
    printf("\n========== Signature Verification Error Tests ==========\n\n");
    
    espsol_keypair_t keypair;
    uint8_t seed[32] = {0};
    uint8_t message[] = "test message";
    uint8_t wrong_message[] = "wrong message";
    uint8_t signature[64];
    esp_err_t err;
    
    /* Initialize and generate keypair */
    espsol_crypto_init();
    espsol_keypair_from_seed(seed, &keypair);
    
    /* Sign the message */
    err = espsol_sign(message, sizeof(message) - 1, &keypair, signature);
    TEST_ASSERT_EQ(err, ESP_OK, "Signing message succeeds");
    
    /* Verify with wrong message */
    err = espsol_verify(wrong_message, sizeof(wrong_message) - 1, signature, keypair.public_key);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_SIGNATURE_INVALID, "Verify wrong message returns SIGNATURE_INVALID");
    
    /* Verify with corrupted signature */
    signature[0] ^= 0xFF;  /* Corrupt first byte */
    err = espsol_verify(message, sizeof(message) - 1, signature, keypair.public_key);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_SIGNATURE_INVALID, "Verify corrupted sig returns SIGNATURE_INVALID");
}

/* ============================================================================
 * Transaction Limit Tests
 * ========================================================================== */

static void test_transaction_limit_errors(void)
{
    printf("\n========== Transaction Limit Error Tests ==========\n\n");
    
    espsol_tx_handle_t tx = NULL;
    uint8_t pubkey[32] = {0};
    esp_err_t err;
    
    espsol_tx_create(&tx);
    
    /* Add maximum instructions + 1 */
    for (int i = 0; i < ESPSOL_MAX_INSTRUCTIONS; i++) {
        pubkey[0] = (uint8_t)i;  /* Make each address unique */
        err = espsol_tx_add_transfer(tx, pubkey, pubkey, 1000);
        if (err != ESP_OK) {
            break;
        }
    }
    
    /* Try to add one more */
    pubkey[0] = 0xFF;
    err = espsol_tx_add_transfer(tx, pubkey, pubkey, 1000);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_MAX_INSTRUCTIONS, "Exceeding instruction limit returns MAX_INSTRUCTIONS");
    
    espsol_tx_destroy(tx);
}

/* ============================================================================
 * Main
 * ========================================================================== */

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════╗\n");
    printf("║     ESPSOL Error Handling Unit Tests       ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    
    /* Run all test suites */
    test_error_code_values();
    test_espsol_is_err_macro();
    test_base58_null_errors();
    test_base64_null_errors();
    test_address_null_errors();
    test_crypto_null_errors();
    test_crypto_uninitialized_keypair();
    test_transaction_null_errors();
    test_transaction_unsigned_errors();
    test_buffer_too_small_errors();
    test_invalid_base58_errors();
    test_invalid_base64_errors();
    test_signature_verification_errors();
    test_transaction_limit_errors();
    
    /* Summary */
    printf("\n");
    printf("╔════════════════════════════════════════════╗\n");
    printf("║            TEST SUMMARY                    ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║  Passed: %-3d                               ║\n", tests_passed);
    printf("║  Failed: %-3d                               ║\n", tests_failed);
    printf("║  Total:  %-3d                               ║\n", tests_passed + tests_failed);
    printf("╚════════════════════════════════════════════╝\n");
    printf("\n");
    
    if (tests_failed == 0) {
        printf("🎉 ALL ERROR HANDLING TESTS PASSED! 🎉\n\n");
        return 0;
    } else {
        printf("❌ SOME TESTS FAILED ❌\n\n");
        return 1;
    }
}
