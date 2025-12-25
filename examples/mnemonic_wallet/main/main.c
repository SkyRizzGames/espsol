/**
 * @file main.c
 * @brief ESPSOL Mnemonic Wallet Example
 *
 * This example demonstrates how to:
 * - Generate a new wallet with a 12 or 24 word seed phrase
 * - Display the seed phrase for backup
 * - Restore a wallet from an existing seed phrase
 * - Use passphrase for extra security
 *
 * IMPORTANT: Seed phrases are sensitive! Never share them!
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "espsol.h"

static const char *TAG = "mnemonic_wallet";

/* NVS key for storing the wallet */
#define WALLET_NVS_KEY "main_wallet"

/**
 * @brief Demonstrate creating a new wallet with seed phrase
 */
static void demo_create_wallet_with_seedphrase(void)
{
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   CREATE NEW WALLET WITH SEED PHRASE   ");
    ESP_LOGI(TAG, "========================================");
    
    /* Generate a new 12-word mnemonic and keypair */
    char mnemonic[ESPSOL_MNEMONIC_12_MAX_LEN];
    espsol_keypair_t wallet;
    
    esp_err_t ret = espsol_keypair_generate_with_mnemonic(
        ESPSOL_MNEMONIC_12_WORDS,
        mnemonic, sizeof(mnemonic),
        &wallet
    );
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate wallet: %s", espsol_err_to_name(ret));
        return;
    }
    
    /* Display the seed phrase for backup */
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║          WRITE DOWN YOUR SEED PHRASE!                    ║");
    ESP_LOGI(TAG, "║  Store it safely - this is your wallet backup!           ║");
    ESP_LOGI(TAG, "╠══════════════════════════════════════════════════════════╣");
    
    /* Print mnemonic with numbered words for easy backup */
    espsol_mnemonic_print(mnemonic, "SEED PHRASE");
    
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "\n");
    
    /* Get and display the wallet address */
    char address[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&wallet, address, sizeof(address));
    ESP_LOGI(TAG, "✓ Wallet Address: %s", address);
    
    /* Save the keypair to NVS for persistence */
    ret = espsol_keypair_save_to_nvs(&wallet, WALLET_NVS_KEY);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Wallet saved to NVS (key: %s)", WALLET_NVS_KEY);
    } else {
        ESP_LOGW(TAG, "Failed to save wallet to NVS: %s", espsol_err_to_name(ret));
    }
    
    /* Clear sensitive data from memory */
    espsol_mnemonic_clear(mnemonic, sizeof(mnemonic));
    espsol_keypair_clear(&wallet);
    
    ESP_LOGI(TAG, "✓ Sensitive data cleared from memory");
}

/**
 * @brief Demonstrate creating a 24-word wallet (higher security)
 */
static void demo_create_24word_wallet(void)
{
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   CREATE 24-WORD WALLET (HIGH SECURITY)");
    ESP_LOGI(TAG, "========================================");
    
    char mnemonic[ESPSOL_MNEMONIC_24_MAX_LEN];
    espsol_keypair_t wallet;
    
    esp_err_t ret = espsol_keypair_generate_with_mnemonic(
        ESPSOL_MNEMONIC_24_WORDS,
        mnemonic, sizeof(mnemonic),
        &wallet
    );
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate 24-word wallet: %s", espsol_err_to_name(ret));
        return;
    }
    
    /* Count words to verify */
    int word_count = espsol_mnemonic_word_count(mnemonic);
    ESP_LOGI(TAG, "Generated %d-word mnemonic (256-bit entropy)", word_count);
    
    espsol_mnemonic_print(mnemonic, "24-WORD SEED PHRASE");
    
    char address[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&wallet, address, sizeof(address));
    ESP_LOGI(TAG, "✓ Wallet Address: %s", address);
    
    /* Cleanup */
    espsol_mnemonic_clear(mnemonic, sizeof(mnemonic));
    espsol_keypair_clear(&wallet);
}

/**
 * @brief Demonstrate restoring wallet from seed phrase
 */
static void demo_restore_from_seedphrase(void)
{
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   RESTORE WALLET FROM SEED PHRASE      ");
    ESP_LOGI(TAG, "========================================");
    
    /* Example: A known test seed phrase (DO NOT use in production!) */
    const char *test_mnemonic = 
        "abandon abandon abandon abandon abandon abandon "
        "abandon abandon abandon abandon abandon about";
    
    ESP_LOGI(TAG, "Restoring wallet from seed phrase:");
    ESP_LOGI(TAG, "\"%s\"", test_mnemonic);
    
    /* First, validate the mnemonic */
    esp_err_t ret = espsol_mnemonic_validate(test_mnemonic);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Invalid mnemonic: %s", espsol_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "✓ Mnemonic is valid");
    
    /* Restore the keypair */
    espsol_keypair_t wallet;
    ret = espsol_keypair_from_mnemonic(test_mnemonic, NULL, &wallet);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to restore wallet: %s", espsol_err_to_name(ret));
        return;
    }
    
    char address[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&wallet, address, sizeof(address));
    ESP_LOGI(TAG, "✓ Restored Wallet Address: %s", address);
    
    /* This is the well-known address for the "abandon" test mnemonic */
    /* You can verify this matches Phantom/Solflare with same phrase */
    
    espsol_keypair_clear(&wallet);
}

/**
 * @brief Demonstrate using passphrase for extra security
 */
static void demo_passphrase_wallet(void)
{
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   PASSPHRASE-PROTECTED WALLET          ");
    ESP_LOGI(TAG, "========================================");
    
    const char *mnemonic = 
        "abandon abandon abandon abandon abandon abandon "
        "abandon abandon abandon abandon abandon about";
    
    /* Without passphrase */
    espsol_keypair_t wallet_no_pass;
    espsol_keypair_from_mnemonic(mnemonic, NULL, &wallet_no_pass);
    
    char addr_no_pass[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&wallet_no_pass, addr_no_pass, sizeof(addr_no_pass));
    
    /* With passphrase "secret" */
    espsol_keypair_t wallet_with_pass;
    espsol_keypair_from_mnemonic(mnemonic, "secret", &wallet_with_pass);
    
    char addr_with_pass[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&wallet_with_pass, addr_with_pass, sizeof(addr_with_pass));
    
    ESP_LOGI(TAG, "Same mnemonic, different passphrases:");
    ESP_LOGI(TAG, "  No passphrase:   %s", addr_no_pass);
    ESP_LOGI(TAG, "  With 'secret':   %s", addr_with_pass);
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "Notice: Different passphrases = Different wallets!");
    ESP_LOGI(TAG, "This provides plausible deniability - you can have");
    ESP_LOGI(TAG, "a 'decoy' wallet and a 'real' wallet from same phrase.");
    
    espsol_keypair_clear(&wallet_no_pass);
    espsol_keypair_clear(&wallet_with_pass);
}

/**
 * @brief Demonstrate word validation
 */
static void demo_word_validation(void)
{
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   BIP39 WORD VALIDATION                ");
    ESP_LOGI(TAG, "========================================");
    
    /* Test some words */
    const char *test_words[] = {
        "abandon",    /* Valid - first word */
        "zoo",        /* Valid - last word */
        "bitcoin",    /* Invalid - not in wordlist */
        "solana",     /* Invalid - not in wordlist */
        "satoshi",    /* Valid - it's in the list! */
        "wallet",     /* Valid */
    };
    
    for (int i = 0; i < sizeof(test_words) / sizeof(test_words[0]); i++) {
        int index;
        bool valid = espsol_mnemonic_word_valid(test_words[i], &index);
        if (valid) {
            ESP_LOGI(TAG, "  '%s' - VALID (index: %d)", test_words[i], index);
        } else {
            ESP_LOGI(TAG, "  '%s' - INVALID (not in BIP39 wordlist)", test_words[i]);
        }
    }
    
    /* Show some words from the wordlist */
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "First 5 words in BIP39 wordlist:");
    for (int i = 0; i < 5; i++) {
        ESP_LOGI(TAG, "  %4d: %s", i, espsol_mnemonic_get_word(i));
    }
    ESP_LOGI(TAG, "Last 5 words in BIP39 wordlist:");
    for (int i = 2043; i < 2048; i++) {
        ESP_LOGI(TAG, "  %4d: %s", i, espsol_mnemonic_get_word(i));
    }
}

/**
 * @brief Demonstrate invalid mnemonic detection
 */
static void demo_invalid_mnemonic_detection(void)
{
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   INVALID MNEMONIC DETECTION           ");
    ESP_LOGI(TAG, "========================================");
    
    /* Test cases for validation */
    struct {
        const char *mnemonic;
        const char *description;
    } test_cases[] = {
        {
            "abandon abandon abandon abandon abandon abandon "
            "abandon abandon abandon abandon abandon about",
            "Valid 12-word mnemonic"
        },
        {
            "abandon abandon abandon abandon abandon abandon "
            "abandon abandon abandon abandon abandon wrong",
            "Invalid checksum (wrong last word)"
        },
        {
            "abandon abandon abandon abandon abandon abandon",
            "Too few words (6 instead of 12)"
        },
        {
            "bitcoin ethereum solana cardano polygon avalanche "
            "near cosmos polkadot tezos algorand stellar",
            "Invalid words (not in BIP39 wordlist)"
        },
        {
            "",
            "Empty mnemonic"
        }
    };
    
    for (int i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        esp_err_t ret = espsol_mnemonic_validate(test_cases[i].mnemonic);
        ESP_LOGI(TAG, "\nTest: %s", test_cases[i].description);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "  Result: ✓ VALID");
        } else {
            ESP_LOGI(TAG, "  Result: ✗ INVALID (%s)", espsol_err_to_name(ret));
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║            ESPSOL MNEMONIC WALLET EXAMPLE                 ║");
    ESP_LOGI(TAG, "║                                                          ║");
    ESP_LOGI(TAG, "║  Demonstrates BIP39 seed phrase generation and recovery  ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════╝");
    
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    /* Initialize ESPSOL crypto */
    ESP_ERROR_CHECK(espsol_crypto_init());
    
    /* Run demonstrations */
    demo_create_wallet_with_seedphrase();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    demo_create_24word_wallet();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    demo_restore_from_seedphrase();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    demo_passphrase_wallet();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    demo_word_validation();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    demo_invalid_mnemonic_detection();
    
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   DEMO COMPLETE                        ");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "Remember: NEVER share your seed phrase!");
    ESP_LOGI(TAG, "Anyone with your seed phrase has full access to your wallet.");
    
    /* Idle loop */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
