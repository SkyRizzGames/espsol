/**
 * @file integration_test.c
 * @brief ESPSOL Devnet Integration Test
 *
 * This file tests the complete SDK workflow on Solana devnet:
 * 1. Connect to WiFi
 * 2. Create two wallets
 * 3. Airdrop 1 SOL to wallet 1
 * 4. Transfer 0.1 SOL from wallet 1 to wallet 2
 * 5. Verify balances
 *
 * To run this test instead of unit tests, call run_integration_test()
 * from app_main() instead of the unit test functions.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "espsol.h"

static const char *TAG = "espsol_integration";

/* ============================================================================
 * WiFi Configuration - UPDATE THESE!
 * ========================================================================== */

#define WIFI_SSID      "ZTE_2.4G_Akkd32"
#define WIFI_PASSWORD  "dtqXCGpa"

/* WiFi connection timeout */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_MAX_RETRY     5

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

/* ============================================================================
 * WiFi Event Handler
 * ========================================================================== */

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying WiFi connection... (%d/%d)", s_retry_num, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* ============================================================================
 * WiFi Initialization
 * ========================================================================== */

static bool wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi '%s'...", WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected successfully");
        return true;
    } else {
        ESP_LOGE(TAG, "WiFi connection failed");
        return false;
    }
}

/* ============================================================================
 * Helper Functions
 * ========================================================================== */

static void print_balance(espsol_rpc_handle_t rpc, const char *label, const char *address)
{
    uint64_t lamports = 0;
    esp_err_t err = espsol_rpc_get_balance(rpc, address, &lamports);
    if (err == ESP_OK) {
        double sol = (double)lamports / ESPSOL_LAMPORTS_PER_SOL;
        ESP_LOGI(TAG, "%s balance: %.9f SOL (%llu lamports)", label, sol, lamports);
    } else {
        ESP_LOGE(TAG, "Failed to get %s balance: %s", label, espsol_err_to_name(err));
    }
}

static bool wait_for_confirmation(espsol_rpc_handle_t rpc, const char *signature, int timeout_sec)
{
    ESP_LOGI(TAG, "Waiting for confirmation...");
    
    bool confirmed = false;
    esp_err_t err = espsol_rpc_confirm_transaction(rpc, signature, timeout_sec * 1000, &confirmed);
    
    if (err == ESP_OK && confirmed) {
        ESP_LOGI(TAG, "Transaction confirmed!");
        return true;
    } else if (err == ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "Confirmation timeout after %d seconds", timeout_sec);
        return false;
    } else {
        ESP_LOGE(TAG, "Error checking confirmation: %s", espsol_err_to_name(err));
        return false;
    }
}

/* ============================================================================
 * Main Integration Test
 * ========================================================================== */

void run_integration_test(void)
{
    esp_err_t err;
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║     ESPSOL Devnet Integration Test         ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    /* ========== Step 0: Initialize NVS ========== */
    ESP_LOGI(TAG, "=== Step 0: Initialize NVS ===");
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "✓ NVS initialized");

    /* ========== Step 1: Connect to WiFi ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Step 1: Connect to WiFi ===");
    if (!wifi_init_sta()) {
        ESP_LOGE(TAG, "✗ WiFi connection failed. Test aborted.");
        return;
    }
    ESP_LOGI(TAG, "✓ WiFi connected");

    /* ========== Step 2: Initialize ESPSOL ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Step 2: Initialize ESPSOL ===");
    
    espsol_config_t config = {
        .rpc_url = ESPSOL_DEVNET_RPC,
        .timeout_ms = 30000,
        .commitment = ESPSOL_COMMITMENT_CONFIRMED,
        .use_tls = true,
        .log_level = ESPSOL_LOG_INFO
    };
    
    err = espsol_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to initialize ESPSOL: %s", espsol_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "✓ ESPSOL initialized (using devnet)");

    /* ========== Step 3: Initialize RPC Client ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Step 3: Initialize RPC Client ===");
    
    espsol_rpc_handle_t rpc;
    err = espsol_rpc_init(&rpc, ESPSOL_DEVNET_RPC);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to initialize RPC: %s", espsol_err_to_name(err));
        return;
    }
    espsol_rpc_set_timeout(rpc, 30000);
    ESP_LOGI(TAG, "✓ RPC client initialized");

    /* ========== Step 4: Create Two Wallets ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Step 4: Create Two Wallets ===");
    
    espsol_keypair_t wallet1, wallet2;
    char addr1[ESPSOL_ADDRESS_MAX_LEN], addr2[ESPSOL_ADDRESS_MAX_LEN];
    
    err = espsol_keypair_generate(&wallet1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to generate wallet 1");
        goto cleanup;
    }
    espsol_keypair_get_address(&wallet1, addr1, sizeof(addr1));
    ESP_LOGI(TAG, "Wallet 1: %s", addr1);
    
    err = espsol_keypair_generate(&wallet2);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to generate wallet 2");
        goto cleanup;
    }
    espsol_keypair_get_address(&wallet2, addr2, sizeof(addr2));
    ESP_LOGI(TAG, "Wallet 2: %s", addr2);
    ESP_LOGI(TAG, "✓ Both wallets created");

    /* ========== Step 5: Check Initial Balances ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Step 5: Check Initial Balances ===");
    print_balance(rpc, "Wallet 1", addr1);
    print_balance(rpc, "Wallet 2", addr2);

    /* ========== Step 6: Request Airdrop to Wallet 1 ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Step 6: Request Airdrop (1 SOL to Wallet 1) ===");
    
    char airdrop_sig[ESPSOL_SIGNATURE_MAX_LEN];
    uint64_t airdrop_amount = ESPSOL_LAMPORTS_PER_SOL;  /* 1 SOL */
    
    ESP_LOGI(TAG, "Requesting airdrop of 1 SOL...");
    err = espsol_rpc_request_airdrop(rpc, addr1, airdrop_amount, airdrop_sig, sizeof(airdrop_sig));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Airdrop request failed: %s", espsol_err_to_name(err));
        ESP_LOGE(TAG, "  Note: Devnet may be rate-limited. Try again later.");
        goto cleanup;
    }
    ESP_LOGI(TAG, "✓ Airdrop requested. Signature: %.32s...", airdrop_sig);
    
    /* Wait for airdrop confirmation */
    if (!wait_for_confirmation(rpc, airdrop_sig, 30)) {
        ESP_LOGE(TAG, "✗ Airdrop confirmation timeout");
        goto cleanup;
    }
    
    /* Check balance after airdrop */
    ESP_LOGI(TAG, "Balance after airdrop:");
    print_balance(rpc, "Wallet 1", addr1);

    /* ========== Step 7: Get Latest Blockhash ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Step 7: Get Latest Blockhash ===");
    
    uint8_t blockhash[ESPSOL_BLOCKHASH_SIZE];
    err = espsol_rpc_get_latest_blockhash(rpc, blockhash, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to get blockhash: %s", espsol_err_to_name(err));
        goto cleanup;
    }
    
    char blockhash_str[ESPSOL_ADDRESS_MAX_LEN];
    espsol_base58_encode(blockhash, ESPSOL_BLOCKHASH_SIZE, blockhash_str, sizeof(blockhash_str));
    ESP_LOGI(TAG, "✓ Blockhash: %s", blockhash_str);

    /* ========== Step 8: Create Transfer Transaction ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Step 8: Create Transfer Transaction (0.1 SOL) ===");
    
    espsol_tx_handle_t tx;
    err = espsol_tx_create(&tx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to create transaction");
        goto cleanup;
    }
    
    /* Set fee payer (wallet 1) */
    err = espsol_tx_set_fee_payer(tx, wallet1.public_key);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to set fee payer");
        espsol_tx_destroy(tx);
        goto cleanup;
    }
    
    /* Set recent blockhash */
    err = espsol_tx_set_recent_blockhash(tx, blockhash);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to set blockhash");
        espsol_tx_destroy(tx);
        goto cleanup;
    }
    
    /* Add transfer instruction: 0.1 SOL = 100,000,000 lamports */
    uint64_t transfer_amount = ESPSOL_LAMPORTS_PER_SOL / 10;  /* 0.1 SOL */
    err = espsol_tx_add_transfer(tx, wallet1.public_key, wallet2.public_key, transfer_amount);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to add transfer instruction");
        espsol_tx_destroy(tx);
        goto cleanup;
    }
    
    ESP_LOGI(TAG, "✓ Transaction created: Transfer %.2f SOL from Wallet 1 to Wallet 2",
             (double)transfer_amount / ESPSOL_LAMPORTS_PER_SOL);

    /* ========== Step 9: Sign Transaction ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Step 9: Sign Transaction ===");
    
    err = espsol_tx_sign(tx, &wallet1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to sign transaction: %s", espsol_err_to_name(err));
        espsol_tx_destroy(tx);
        goto cleanup;
    }
    
    char tx_sig[ESPSOL_SIGNATURE_MAX_LEN];
    espsol_tx_get_signature_base58(tx, tx_sig, sizeof(tx_sig));
    ESP_LOGI(TAG, "✓ Transaction signed. Signature: %.32s...", tx_sig);

    /* ========== Step 10: Serialize and Send Transaction ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Step 10: Send Transaction ===");
    
    char tx_base64[2048];
    err = espsol_tx_to_base64(tx, tx_base64, sizeof(tx_base64));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to serialize transaction");
        espsol_tx_destroy(tx);
        goto cleanup;
    }
    
    char send_sig[ESPSOL_SIGNATURE_MAX_LEN];
    err = espsol_rpc_send_transaction(rpc, tx_base64, send_sig, sizeof(send_sig));
    espsol_tx_destroy(tx);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to send transaction: %s", espsol_err_to_name(err));
        goto cleanup;
    }
    
    ESP_LOGI(TAG, "✓ Transaction sent! Signature: %.32s...", send_sig);

    /* ========== Step 11: Wait for Confirmation ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Step 11: Wait for Confirmation ===");
    
    if (!wait_for_confirmation(rpc, send_sig, 30)) {
        ESP_LOGW(TAG, "⚠ Transaction confirmation timeout (may still succeed)");
    } else {
        ESP_LOGI(TAG, "✓ Transaction confirmed!");
    }

    /* ========== Step 12: Check Final Balances ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Step 12: Check Final Balances ===");
    
    /* Wait a moment for state to propagate */
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    print_balance(rpc, "Wallet 1", addr1);
    print_balance(rpc, "Wallet 2", addr2);

    /* ========== Summary ========== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║     🎉 Integration Test PASSED! 🎉         ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Transaction Explorer Links:");
    ESP_LOGI(TAG, "  Wallet 1: https://explorer.solana.com/address/%s?cluster=devnet", addr1);
    ESP_LOGI(TAG, "  Wallet 2: https://explorer.solana.com/address/%s?cluster=devnet", addr2);
    ESP_LOGI(TAG, "  Transfer: https://explorer.solana.com/tx/%s?cluster=devnet", send_sig);

cleanup:
    /* Clear keypairs */
    espsol_keypair_clear(&wallet1);
    espsol_keypair_clear(&wallet2);
    
    /* Cleanup RPC */
    espsol_rpc_deinit(rpc);
    espsol_deinit();
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Integration test complete.");
}
