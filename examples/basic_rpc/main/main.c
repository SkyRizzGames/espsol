/**
 * @file main.c
 * @brief ESPSOL Basic RPC Example
 *
 * This example demonstrates how to:
 * - Initialize ESPSOL SDK
 * - Connect to Solana RPC
 * - Query network information
 * - Check account balances
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "espsol.h"

static const char *TAG = "basic_rpc";

/* WiFi credentials - change these to your network */
#define WIFI_SSID      "YOUR_WIFI_SSID"
#define WIFI_PASSWORD  "YOUR_WIFI_PASSWORD"

/* Known Solana address for testing (Devnet faucet) */
#define TEST_ADDRESS   "So11111111111111111111111111111111111111112"

/* Event group for WiFi */
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected, reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
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
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    /* Wait for connection */
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdFALSE, portMAX_DELAY);
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESPSOL Basic RPC Example");
    ESP_LOGI(TAG, "========================");
    
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    /* Connect to WiFi */
    ESP_LOGI(TAG, "Connecting to WiFi...");
    wifi_init();
    ESP_LOGI(TAG, "WiFi connected!");
    
    /* Initialize ESPSOL */
    espsol_config_t config = ESPSOL_CONFIG_DEFAULT();
    config.rpc_url = ESPSOL_DEVNET_RPC;  /* Use devnet for testing */
    ESP_ERROR_CHECK(espsol_init(&config));
    
    /* Initialize RPC client */
    espsol_rpc_handle_t rpc;
    ESP_ERROR_CHECK(espsol_rpc_init(&rpc, config.rpc_url));
    
    /* ==================== RPC Examples ==================== */
    
    /* 1. Get Solana version */
    ESP_LOGI(TAG, "\n--- Solana Version ---");
    char version[64];
    if (espsol_rpc_get_version(rpc, version, sizeof(version)) == ESP_OK) {
        ESP_LOGI(TAG, "Solana version: %s", version);
    }
    
    /* 2. Get current slot */
    ESP_LOGI(TAG, "\n--- Current Slot ---");
    uint64_t slot;
    if (espsol_rpc_get_slot(rpc, &slot) == ESP_OK) {
        ESP_LOGI(TAG, "Current slot: %llu", (unsigned long long)slot);
    }
    
    /* 3. Get block height */
    ESP_LOGI(TAG, "\n--- Block Height ---");
    uint64_t height;
    if (espsol_rpc_get_block_height(rpc, &height) == ESP_OK) {
        ESP_LOGI(TAG, "Block height: %llu", (unsigned long long)height);
    }
    
    /* 4. Check node health */
    ESP_LOGI(TAG, "\n--- Node Health ---");
    bool healthy;
    if (espsol_rpc_get_health(rpc, &healthy) == ESP_OK) {
        ESP_LOGI(TAG, "Node healthy: %s", healthy ? "YES" : "NO");
    }
    
    /* 5. Get account balance */
    ESP_LOGI(TAG, "\n--- Account Balance ---");
    uint64_t balance;
    if (espsol_rpc_get_balance(rpc, TEST_ADDRESS, &balance) == ESP_OK) {
        ESP_LOGI(TAG, "Address: %s", TEST_ADDRESS);
        ESP_LOGI(TAG, "Balance: %.9f SOL (%llu lamports)", 
                 espsol_lamports_to_sol(balance), (unsigned long long)balance);
    }
    
    /* 6. Get latest blockhash */
    ESP_LOGI(TAG, "\n--- Latest Blockhash ---");
    uint8_t blockhash[32];
    uint64_t last_valid_height;
    if (espsol_rpc_get_latest_blockhash(rpc, blockhash, &last_valid_height) == ESP_OK) {
        char blockhash_str[64];
        espsol_base58_encode(blockhash, 32, blockhash_str, sizeof(blockhash_str));
        ESP_LOGI(TAG, "Blockhash: %s", blockhash_str);
        ESP_LOGI(TAG, "Valid until block: %llu", (unsigned long long)last_valid_height);
    }
    
    /* Cleanup */
    espsol_rpc_deinit(rpc);
    espsol_deinit();
    
    ESP_LOGI(TAG, "\n========================");
    ESP_LOGI(TAG, "Example complete!");
    
    /* Idle loop */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
