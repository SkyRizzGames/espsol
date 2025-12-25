/**
 * @file test_tx.c
 * @brief Host-based Unit Tests for ESPSOL Transaction Module
 *
 * Tests transaction building, signing, and serialization.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Include ESPSOL headers */
#include "espsol_types.h"
#include "espsol_utils.h"
#include "espsol_crypto.h"
#include "espsol_tx.h"

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

/* ============================================================================
 * Transaction Tests
 * ========================================================================== */

static void test_tx_create_destroy(void)
{
    printf("\n========== Transaction Create/Destroy Tests ==========\n\n");
    
    espsol_tx_handle_t tx = NULL;
    esp_err_t err;
    
    /* Test 1: Create transaction */
    err = espsol_tx_create(&tx);
    TEST_ASSERT_EQ(err, ESP_OK, "Create transaction");
    TEST_ASSERT(tx != NULL, "Transaction handle not NULL");
    
    /* Test 2: Destroy transaction */
    err = espsol_tx_destroy(tx);
    TEST_ASSERT_EQ(err, ESP_OK, "Destroy transaction");
    
    /* Test 3: NULL argument handling */
    err = espsol_tx_create(NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Create with NULL returns error");
    
    err = espsol_tx_destroy(NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Destroy NULL returns error");
}

static void test_tx_configuration(void)
{
    printf("\n========== Transaction Configuration Tests ==========\n\n");
    
    espsol_tx_handle_t tx = NULL;
    esp_err_t err;
    
    err = espsol_tx_create(&tx);
    TEST_ASSERT_EQ(err, ESP_OK, "Create transaction");
    
    /* Test fee payer */
    uint8_t fee_payer[32] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
    };
    
    err = espsol_tx_set_fee_payer(tx, fee_payer);
    TEST_ASSERT_EQ(err, ESP_OK, "Set fee payer");
    
    err = espsol_tx_set_fee_payer(tx, NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Set NULL fee payer returns error");
    
    /* Test blockhash */
    uint8_t blockhash[32] = {
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89
    };
    
    err = espsol_tx_set_recent_blockhash(tx, blockhash);
    TEST_ASSERT_EQ(err, ESP_OK, "Set blockhash");
    
    err = espsol_tx_set_recent_blockhash(tx, NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "Set NULL blockhash returns error");
    
    espsol_tx_destroy(tx);
}

static void test_tx_add_transfer(void)
{
    printf("\n========== Transaction Add Transfer Tests ==========\n\n");
    
    espsol_tx_handle_t tx = NULL;
    esp_err_t err;
    
    err = espsol_tx_create(&tx);
    TEST_ASSERT_EQ(err, ESP_OK, "Create transaction");
    
    uint8_t from[32] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
    };
    
    uint8_t to[32] = {
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40
    };
    
    /* Add transfer instruction */
    uint64_t lamports = 1000000000ULL;  /* 1 SOL */
    err = espsol_tx_add_transfer(tx, from, to, lamports);
    TEST_ASSERT_EQ(err, ESP_OK, "Add transfer instruction");
    
    /* Check instruction count */
    size_t count = 0;
    err = espsol_tx_get_instruction_count(tx, &count);
    TEST_ASSERT_EQ(err, ESP_OK, "Get instruction count");
    TEST_ASSERT_EQ(count, 1, "Instruction count is 1");
    
    /* Add another transfer */
    err = espsol_tx_add_transfer(tx, from, to, 500000000ULL);
    TEST_ASSERT_EQ(err, ESP_OK, "Add second transfer");
    
    err = espsol_tx_get_instruction_count(tx, &count);
    TEST_ASSERT_EQ(count, 2, "Instruction count is 2");
    
    /* Test NULL arguments */
    err = espsol_tx_add_transfer(tx, NULL, to, lamports);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "NULL from returns error");
    
    err = espsol_tx_add_transfer(tx, from, NULL, lamports);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "NULL to returns error");
    
    espsol_tx_destroy(tx);
}

static void test_tx_sign_and_serialize(void)
{
    printf("\n========== Transaction Sign and Serialize Tests ==========\n\n");
    
    espsol_tx_handle_t tx = NULL;
    esp_err_t err;
    
    err = espsol_tx_create(&tx);
    TEST_ASSERT_EQ(err, ESP_OK, "Create transaction");
    
    /* Generate a keypair */
    espsol_keypair_t keypair;
    uint8_t seed[32] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
    };
    
    err = espsol_keypair_from_seed(seed, &keypair);
    TEST_ASSERT_EQ(err, ESP_OK, "Generate keypair from seed");
    
    /* Recipient */
    uint8_t to[32] = {
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40
    };
    
    /* Blockhash */
    uint8_t blockhash[32] = {
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
        0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89
    };
    
    /* Configure transaction */
    err = espsol_tx_set_fee_payer(tx, keypair.public_key);
    TEST_ASSERT_EQ(err, ESP_OK, "Set fee payer");
    
    err = espsol_tx_set_recent_blockhash(tx, blockhash);
    TEST_ASSERT_EQ(err, ESP_OK, "Set blockhash");
    
    /* Add transfer */
    err = espsol_tx_add_transfer(tx, keypair.public_key, to, 1000000000ULL);
    TEST_ASSERT_EQ(err, ESP_OK, "Add transfer");
    
    /* Check not signed yet */
    TEST_ASSERT(!espsol_tx_is_signed(tx), "Transaction not signed yet");
    
    /* Try to serialize before signing - should fail */
    uint8_t buffer[1232];
    size_t out_len = 0;
    err = espsol_tx_serialize(tx, buffer, sizeof(buffer), &out_len);
    TEST_ASSERT_EQ(err, ESP_ERR_ESPSOL_TX_NOT_SIGNED, "Serialize unsigned fails");
    
    /* Sign the transaction */
    err = espsol_tx_sign(tx, &keypair);
    TEST_ASSERT_EQ(err, ESP_OK, "Sign transaction");
    
    /* Check signed */
    TEST_ASSERT(espsol_tx_is_signed(tx), "Transaction is signed");
    
    /* Get signature count */
    size_t sig_count = 0;
    err = espsol_tx_get_signature_count(tx, &sig_count);
    TEST_ASSERT_EQ(err, ESP_OK, "Get signature count");
    TEST_ASSERT_EQ(sig_count, 1, "Signature count is 1");
    
    /* Serialize */
    err = espsol_tx_serialize(tx, buffer, sizeof(buffer), &out_len);
    TEST_ASSERT_EQ(err, ESP_OK, "Serialize signed transaction");
    TEST_ASSERT(out_len > 0, "Serialized length > 0");
    printf("  Serialized transaction length: %zu bytes\n", out_len);
    
    /* Convert to Base64 */
    char base64[2048];
    err = espsol_tx_to_base64(tx, base64, sizeof(base64));
    TEST_ASSERT_EQ(err, ESP_OK, "Convert to Base64");
    printf("  Base64 transaction: %.80s...\n", base64);
    
    /* Get signature as Base58 */
    char sig_base58[128];
    err = espsol_tx_get_signature_base58(tx, sig_base58, sizeof(sig_base58));
    TEST_ASSERT_EQ(err, ESP_OK, "Get signature Base58");
    printf("  Transaction signature: %s\n", sig_base58);
    
    espsol_tx_destroy(tx);
}

static void test_tx_reset(void)
{
    printf("\n========== Transaction Reset Tests ==========\n\n");
    
    espsol_tx_handle_t tx = NULL;
    esp_err_t err;
    
    err = espsol_tx_create(&tx);
    TEST_ASSERT_EQ(err, ESP_OK, "Create transaction");
    
    /* Add some data */
    uint8_t pubkey[32] = { 1, 2, 3 };
    uint8_t blockhash[32] = { 4, 5, 6 };
    
    espsol_tx_set_fee_payer(tx, pubkey);
    espsol_tx_set_recent_blockhash(tx, blockhash);
    espsol_tx_add_transfer(tx, pubkey, pubkey, 1000);
    
    size_t count = 0;
    espsol_tx_get_instruction_count(tx, &count);
    TEST_ASSERT_EQ(count, 1, "Has 1 instruction before reset");
    
    /* Reset */
    err = espsol_tx_reset(tx);
    TEST_ASSERT_EQ(err, ESP_OK, "Reset transaction");
    
    espsol_tx_get_instruction_count(tx, &count);
    TEST_ASSERT_EQ(count, 0, "Has 0 instructions after reset");
    
    espsol_tx_destroy(tx);
}

static void test_tx_custom_instruction(void)
{
    printf("\n========== Custom Instruction Tests ==========\n\n");
    
    espsol_tx_handle_t tx = NULL;
    esp_err_t err;
    
    err = espsol_tx_create(&tx);
    TEST_ASSERT_EQ(err, ESP_OK, "Create transaction");
    
    /* Define accounts */
    espsol_account_meta_t accounts[2];
    
    /* Account 1 - signer, writable */
    memset(accounts[0].pubkey, 0x01, 32);
    accounts[0].is_signer = true;
    accounts[0].is_writable = true;
    
    /* Account 2 - not signer, writable */
    memset(accounts[1].pubkey, 0x02, 32);
    accounts[1].is_signer = false;
    accounts[1].is_writable = true;
    
    /* Program ID */
    uint8_t program_id[32];
    memset(program_id, 0x03, 32);
    
    /* Instruction data */
    uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
    
    /* Add custom instruction */
    err = espsol_tx_add_instruction(tx, program_id, accounts, 2, data, sizeof(data));
    TEST_ASSERT_EQ(err, ESP_OK, "Add custom instruction");
    
    size_t count = 0;
    espsol_tx_get_instruction_count(tx, &count);
    TEST_ASSERT_EQ(count, 1, "Instruction count is 1");
    
    espsol_tx_destroy(tx);
}

static void test_tx_memo(void)
{
    printf("\n========== Memo Instruction Tests ==========\n\n");
    
    espsol_tx_handle_t tx = NULL;
    esp_err_t err;
    
    err = espsol_tx_create(&tx);
    TEST_ASSERT_EQ(err, ESP_OK, "Create transaction");
    
    /* Add memo */
    err = espsol_tx_add_memo(tx, "Hello from ESP32!");
    TEST_ASSERT_EQ(err, ESP_OK, "Add memo instruction");
    
    size_t count = 0;
    espsol_tx_get_instruction_count(tx, &count);
    TEST_ASSERT_EQ(count, 1, "Instruction count is 1");
    
    /* Test NULL memo */
    err = espsol_tx_add_memo(tx, NULL);
    TEST_ASSERT_EQ(err, ESP_ERR_INVALID_ARG, "NULL memo returns error");
    
    espsol_tx_destroy(tx);
}

static void test_program_ids(void)
{
    printf("\n========== Program ID Tests ==========\n\n");
    
    /* System Program should be all zeros */
    uint8_t expected_system[32] = { 0 };
    TEST_ASSERT(memcmp(ESPSOL_SYSTEM_PROGRAM_ID, expected_system, 32) == 0, 
                "System Program ID is all zeros");
    
    /* Token Program ID should be valid */
    TEST_ASSERT(ESPSOL_TOKEN_PROGRAM_ID[0] == 0x06, "Token Program ID first byte");
    
    /* Verify they're different */
    TEST_ASSERT(memcmp(ESPSOL_SYSTEM_PROGRAM_ID, ESPSOL_TOKEN_PROGRAM_ID, 32) != 0,
                "System and Token Program IDs are different");
    
    TEST_ASSERT(memcmp(ESPSOL_MEMO_PROGRAM_ID, ESPSOL_TOKEN_PROGRAM_ID, 32) != 0,
                "Memo and Token Program IDs are different");
}

/* ============================================================================
 * Main
 * ========================================================================== */

int main(void)
{
    printf("==============================================\n");
    printf("   ESPSOL Transaction Module Host Tests\n");
    printf("==============================================\n");
    
    /* Run all tests */
    test_tx_create_destroy();
    test_tx_configuration();
    test_tx_add_transfer();
    test_tx_sign_and_serialize();
    test_tx_reset();
    test_tx_custom_instruction();
    test_tx_memo();
    test_program_ids();
    
    /* Summary */
    printf("\n==============================================\n");
    printf("Test Summary: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("==============================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
