/**
 * @file main.c
 * @brief ESPSOL Transfer SOL Example
 *
 * This example demonstrates how to:
 * - Create a wallet
 * - Request an airdrop (devnet)
 * - Transfer SOL to another address
 * - Confirm transactions
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "espsol.h"

static const char *TAG = "transfer_sol";

/* WiFi credentials */
#define WIFI_SSID      "YOUR_WIFI_SSID"
#define WIFI_PASSWORD  "YOUR_WIFI_PASSWORD"

/* Transfer amount */
#define TRANSFER_AMOUNT_SOL  0.1  /* Transfer 0.1 SOL */

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                                &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                                &wifi_event_handler, NULL));
    
    wifi_config_t wifi_config = {
        .sta = { .ssid = WIFI_SSID, .password = WIFI_PASSWORD },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi connected");
}

void app_main(void)
{
    esp_err_t ret;
    
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║    ESPSOL Transfer SOL Example         ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    
    /* Initialize NVS */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    /* Connect to WiFi */
    ESP_LOGI(TAG, "Connecting to WiFi...");
    wifi_init();
    
    /* Initialize ESPSOL */
    espsol_config_t config = ESPSOL_CONFIG_DEFAULT();
    config.rpc_url = ESPSOL_DEVNET_RPC;
    ESP_ERROR_CHECK(espsol_init(&config));
    
    /* Initialize RPC client */
    espsol_rpc_handle_t rpc;
    ESP_ERROR_CHECK(espsol_rpc_init(&rpc, config.rpc_url));
    
    /* ==================== Create Two Wallets ==================== */
    ESP_LOGI(TAG, "\n=== Step 1: Create Wallets ===");
    
    espsol_keypair_t sender, receiver;
    ESP_ERROR_CHECK(espsol_keypair_generate(&sender));
    ESP_ERROR_CHECK(espsol_keypair_generate(&receiver));
    
    char sender_addr[ESPSOL_ADDRESS_MAX_LEN];
    char receiver_addr[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&sender, sender_addr, sizeof(sender_addr));
    espsol_keypair_get_address(&receiver, receiver_addr, sizeof(receiver_addr));
    
    ESP_LOGI(TAG, "Sender:   %s", sender_addr);
    ESP_LOGI(TAG, "Receiver: %s", receiver_addr);
    
    /* ==================== Request Airdrop ==================== */
    ESP_LOGI(TAG, "\n=== Step 2: Request Airdrop (1 SOL) ===");
    
    char airdrop_sig[ESPSOL_SIGNATURE_MAX_LEN];
    ret = espsol_rpc_request_airdrop(rpc, sender_addr, ESPSOL_LAMPORTS_PER_SOL,
                                     airdrop_sig, sizeof(airdrop_sig));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Airdrop failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Devnet may be rate-limited. Try again later.");
        goto cleanup;
    }
    ESP_LOGI(TAG, "Airdrop signature: %.20s...", airdrop_sig);
    
    /* Wait for confirmation */
    ESP_LOGI(TAG, "Waiting for confirmation...");
    bool confirmed = false;
    ret = espsol_rpc_confirm_transaction(rpc, airdrop_sig, 30000, &confirmed);
    if (!confirmed) {
        ESP_LOGE(TAG, "Airdrop not confirmed");
        goto cleanup;
    }
    ESP_LOGI(TAG, "✓ Airdrop confirmed!");
    
    /* Check balance */
    uint64_t sender_balance;
    espsol_rpc_get_balance(rpc, sender_addr, &sender_balance);
    ESP_LOGI(TAG, "Sender balance: %.9f SOL", espsol_lamports_to_sol(sender_balance));
    
    /* ==================== Create Transfer Transaction ==================== */
    ESP_LOGI(TAG, "\n=== Step 3: Create Transfer Transaction ===");
    
    uint64_t transfer_lamports = espsol_sol_to_lamports(TRANSFER_AMOUNT_SOL);
    ESP_LOGI(TAG, "Transferring %.2f SOL (%llu lamports)", 
             TRANSFER_AMOUNT_SOL, (unsigned long long)transfer_lamports);
    
    /* Get latest blockhash */
    uint8_t blockhash[32];
    uint64_t last_valid_height;
    ESP_ERROR_CHECK(espsol_rpc_get_latest_blockhash(rpc, blockhash, &last_valid_height));
    
    /* Create transaction */
    espsol_transaction_t tx;
    ESP_ERROR_CHECK(espsol_tx_init(&tx));
    ESP_ERROR_CHECK(espsol_tx_set_fee_payer(&tx, sender.public_key));
    ESP_ERROR_CHECK(espsol_tx_set_recent_blockhash(&tx, blockhash));
    ESP_ERROR_CHECK(espsol_tx_add_transfer(&tx, sender.public_key, 
                                           receiver.public_key, transfer_lamports));
    
    ESP_LOGI(TAG, "Transaction created with %zu instruction(s)", tx.message.instruction_count);
    
    /* ==================== Sign Transaction ==================== */
    ESP_LOGI(TAG, "\n=== Step 4: Sign Transaction ===");
    
    ESP_ERROR_CHECK(espsol_tx_sign(&tx, &sender));
    
    char sig_preview[32];
    espsol_base58_encode(tx.signatures[0], 16, sig_preview, sizeof(sig_preview));
    ESP_LOGI(TAG, "Signature: %s...", sig_preview);
    
    /* ==================== Serialize and Send ==================== */
    ESP_LOGI(TAG, "\n=== Step 5: Send Transaction ===");
    
    char tx_base64[2048];
    ESP_ERROR_CHECK(espsol_tx_to_base64(&tx, tx_base64, sizeof(tx_base64)));
    
    char tx_sig[ESPSOL_SIGNATURE_MAX_LEN];
    ret = espsol_rpc_send_transaction(rpc, tx_base64, tx_sig, sizeof(tx_sig));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Send failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Error: %s", espsol_rpc_get_last_error(rpc));
        goto cleanup;
    }
    ESP_LOGI(TAG, "Transaction sent!");
    ESP_LOGI(TAG, "Signature: %s", tx_sig);
    
    /* ==================== Wait for Confirmation ==================== */
    ESP_LOGI(TAG, "\n=== Step 6: Wait for Confirmation ===");
    
    ret = espsol_rpc_confirm_transaction(rpc, tx_sig, 30000, &confirmed);
    if (!confirmed) {
        ESP_LOGE(TAG, "Transaction not confirmed");
        goto cleanup;
    }
    ESP_LOGI(TAG, "✓ Transaction confirmed!");
    
    /* ==================== Verify Balances ==================== */
    ESP_LOGI(TAG, "\n=== Step 7: Verify Final Balances ===");
    
    uint64_t final_sender, final_receiver;
    espsol_rpc_get_balance(rpc, sender_addr, &final_sender);
    espsol_rpc_get_balance(rpc, receiver_addr, &final_receiver);
    
    ESP_LOGI(TAG, "Sender:   %.9f SOL (was %.9f)", 
             espsol_lamports_to_sol(final_sender),
             espsol_lamports_to_sol(sender_balance));
    ESP_LOGI(TAG, "Receiver: %.9f SOL", 
             espsol_lamports_to_sol(final_receiver));
    
    /* ==================== Explorer Links ==================== */
    ESP_LOGI(TAG, "\n=== Solana Explorer ===");
    ESP_LOGI(TAG, "View transaction:");
    ESP_LOGI(TAG, "  https://explorer.solana.com/tx/%s?cluster=devnet", tx_sig);
    
    ESP_LOGI(TAG, "\n╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║       🎉 Transfer Complete! 🎉         ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    
cleanup:
    espsol_keypair_clear(&sender);
    espsol_keypair_clear(&receiver);
    espsol_rpc_deinit(rpc);
    espsol_deinit();
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
