/**
 * @file main.c
 * @brief ESPSOL Wallet Demo
 *
 * This example demonstrates how to:
 * - Generate new Ed25519 keypairs
 * - Import keypairs from seed/private key
 * - Sign and verify messages
 * - Store keypairs securely in NVS
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

static const char *TAG = "wallet_demo";

/* NVS key for storing the wallet */
#define WALLET_NVS_KEY "my_wallet"

void app_main(void)
{
    ESP_LOGI(TAG, "ESPSOL Wallet Demo");
    ESP_LOGI(TAG, "==================");
    
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    /* Initialize ESPSOL crypto */
    ESP_ERROR_CHECK(espsol_crypto_init());
    
    /* ==================== Generate New Keypair ==================== */
    ESP_LOGI(TAG, "\n--- Generate New Keypair ---");
    
    espsol_keypair_t wallet;
    ESP_ERROR_CHECK(espsol_keypair_generate(&wallet));
    
    char address[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&wallet, address, sizeof(address));
    ESP_LOGI(TAG, "New wallet address: %s", address);
    
    /* Export private key as Base58 (for backup) */
    char privkey_b58[128];
    espsol_keypair_to_base58(&wallet, privkey_b58, sizeof(privkey_b58));
    ESP_LOGI(TAG, "Private key (keep secret!): %s", privkey_b58);
    
    /* ==================== Sign a Message ==================== */
    ESP_LOGI(TAG, "\n--- Sign Message ---");
    
    const char *message = "Hello, Solana from ESP32!";
    uint8_t signature[ESPSOL_SIGNATURE_SIZE];
    
    ESP_ERROR_CHECK(espsol_sign_string(message, &wallet, signature));
    
    char sig_b58[128];
    espsol_base58_encode(signature, ESPSOL_SIGNATURE_SIZE, sig_b58, sizeof(sig_b58));
    ESP_LOGI(TAG, "Message: \"%s\"", message);
    ESP_LOGI(TAG, "Signature: %s", sig_b58);
    
    /* ==================== Verify Signature ==================== */
    ESP_LOGI(TAG, "\n--- Verify Signature ---");
    
    ret = espsol_verify((const uint8_t *)message, strlen(message),
                        signature, wallet.public_key);
    ESP_LOGI(TAG, "Signature valid: %s", ret == ESP_OK ? "YES" : "NO");
    
    /* Try with wrong message */
    ret = espsol_verify((const uint8_t *)"Wrong message", 13,
                        signature, wallet.public_key);
    ESP_LOGI(TAG, "Wrong message verification: %s", 
             ret == ESP_OK ? "VALID (unexpected!)" : "INVALID (expected)");
    
    /* ==================== Save to NVS ==================== */
    ESP_LOGI(TAG, "\n--- Save to NVS ---");
    
    ret = espsol_keypair_save_to_nvs(&wallet, WALLET_NVS_KEY);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Wallet saved to NVS key: %s", WALLET_NVS_KEY);
    } else {
        ESP_LOGE(TAG, "Failed to save wallet: %s", esp_err_to_name(ret));
    }
    
    /* Clear the original keypair */
    espsol_keypair_clear(&wallet);
    ESP_LOGI(TAG, "Original keypair cleared from memory");
    
    /* ==================== Load from NVS ==================== */
    ESP_LOGI(TAG, "\n--- Load from NVS ---");
    
    espsol_keypair_t loaded_wallet;
    ret = espsol_keypair_load_from_nvs(WALLET_NVS_KEY, &loaded_wallet);
    if (ret == ESP_OK) {
        char loaded_address[ESPSOL_ADDRESS_MAX_LEN];
        espsol_keypair_get_address(&loaded_wallet, loaded_address, sizeof(loaded_address));
        ESP_LOGI(TAG, "Loaded wallet address: %s", loaded_address);
        ESP_LOGI(TAG, "Addresses match: %s", 
                 strcmp(address, loaded_address) == 0 ? "YES" : "NO");
        
        /* Verify we can still sign with loaded keypair */
        uint8_t new_sig[ESPSOL_SIGNATURE_SIZE];
        espsol_sign_string(message, &loaded_wallet, new_sig);
        ESP_LOGI(TAG, "Signatures match: %s",
                 memcmp(signature, new_sig, ESPSOL_SIGNATURE_SIZE) == 0 ? "YES" : "NO");
        
        espsol_keypair_clear(&loaded_wallet);
    } else {
        ESP_LOGE(TAG, "Failed to load wallet: %s", esp_err_to_name(ret));
    }
    
    /* ==================== Generate from Seed ==================== */
    ESP_LOGI(TAG, "\n--- Generate from Seed ---");
    
    /* A deterministic seed produces the same keypair every time */
    uint8_t seed[32] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
    };
    
    espsol_keypair_t seeded_wallet;
    ESP_ERROR_CHECK(espsol_keypair_from_seed(seed, &seeded_wallet));
    
    char seeded_address[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&seeded_wallet, seeded_address, sizeof(seeded_address));
    ESP_LOGI(TAG, "Seeded wallet address: %s", seeded_address);
    ESP_LOGI(TAG, "(Same seed always produces the same address)");
    
    espsol_keypair_clear(&seeded_wallet);
    
    /* ==================== Import from Base58 ==================== */
    ESP_LOGI(TAG, "\n--- Import from Base58 ---");
    
    /* This is the exported private key from earlier */
    espsol_keypair_t imported_wallet;
    ret = espsol_keypair_from_base58(privkey_b58, &imported_wallet);
    if (ret == ESP_OK) {
        char imported_address[ESPSOL_ADDRESS_MAX_LEN];
        espsol_keypair_get_address(&imported_wallet, imported_address, sizeof(imported_address));
        ESP_LOGI(TAG, "Imported wallet address: %s", imported_address);
        ESP_LOGI(TAG, "Matches original: %s",
                 strcmp(address, imported_address) == 0 ? "YES" : "NO");
        espsol_keypair_clear(&imported_wallet);
    } else {
        ESP_LOGE(TAG, "Failed to import: %s", esp_err_to_name(ret));
    }
    
    /* ==================== Check NVS existence ==================== */
    ESP_LOGI(TAG, "\n--- Check NVS Existence ---");
    
    bool exists;
    espsol_keypair_exists_in_nvs(WALLET_NVS_KEY, &exists);
    ESP_LOGI(TAG, "Wallet '%s' exists in NVS: %s", WALLET_NVS_KEY, exists ? "YES" : "NO");
    
    espsol_keypair_exists_in_nvs("nonexistent_key", &exists);
    ESP_LOGI(TAG, "Wallet 'nonexistent_key' exists: %s", exists ? "YES" : "NO");
    
    /* ==================== Delete from NVS ==================== */
    ESP_LOGI(TAG, "\n--- Delete from NVS ---");
    
    ret = espsol_keypair_delete_from_nvs(WALLET_NVS_KEY);
    ESP_LOGI(TAG, "Delete result: %s", esp_err_to_name(ret));
    
    espsol_keypair_exists_in_nvs(WALLET_NVS_KEY, &exists);
    ESP_LOGI(TAG, "After deletion, exists: %s", exists ? "YES" : "NO");
    
    ESP_LOGI(TAG, "\n==================");
    ESP_LOGI(TAG, "Demo complete!");
    
    /* Idle loop */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
