/**
 * @file main.c
 * @brief ESPSOL WebSocket Subscription Example
 *
 * This example demonstrates how to:
 * - Connect to Solana WebSocket RPC
 * - Subscribe to account changes
 * - Subscribe to transaction logs
 * - Subscribe to slot notifications
 * - Handle real-time notifications
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
#include "espsol_ws.h"

static const char *TAG = "websocket_sub";

/* WiFi credentials - change these to your network */
#define WIFI_SSID      CONFIG_EXAMPLE_WIFI_SSID
#define WIFI_PASSWORD  CONFIG_EXAMPLE_WIFI_PASSWORD

/* Known Solana accounts for testing (Devnet) */
#define WRAPPED_SOL_MINT    "So11111111111111111111111111111111111111112"
#define SYSTEM_PROGRAM      "11111111111111111111111111111111"

/* Event group for WiFi */
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

/* Subscription IDs */
static int account_sub_id = -1;
static int logs_sub_id = -1;
static int slot_sub_id = -1;

/* ============================================================================
 * WiFi Setup
 * ========================================================================== */

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

/* ============================================================================
 * WebSocket Event Handler
 * ========================================================================== */

static void ws_event_handler(const espsol_ws_event_t *event, void *user_data)
{
    switch (event->type) {
    case ESPSOL_WS_EVENT_CONNECTED:
        ESP_LOGI(TAG, "✓ WebSocket connected!");
        break;
        
    case ESPSOL_WS_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "✗ WebSocket disconnected");
        break;
        
    case ESPSOL_WS_EVENT_ERROR:
        ESP_LOGE(TAG, "✗ WebSocket error");
        break;
        
    case ESPSOL_WS_EVENT_DATA:
        ESP_LOGI(TAG, "📬 Received notification (type=%d, sub_id=%d)", 
                event->sub_type, event->subscription_id);
        
        switch (event->sub_type) {
        case ESPSOL_WS_SUB_ACCOUNT:
            ESP_LOGI(TAG, "  Account Update:");
            ESP_LOGI(TAG, "    Owner: %s", event->data.account.owner);
            ESP_LOGI(TAG, "    Lamports: %llu", event->data.account.lamports);
            ESP_LOGI(TAG, "    Executable: %s", 
                    event->data.account.executable ? "yes" : "no");
            ESP_LOGI(TAG, "    Rent Epoch: %llu", event->data.account.rent_epoch);
            break;
            
        case ESPSOL_WS_SUB_LOGS:
            ESP_LOGI(TAG, "  Transaction Logs:");
            ESP_LOGI(TAG, "    Signature: %s", event->data.logs.signature);
            if (event->data.logs.err[0] != '\0') {
                ESP_LOGE(TAG, "    Error: %s", event->data.logs.err);
            } else {
                ESP_LOGI(TAG, "    Status: Success");
            }
            break;
            
        case ESPSOL_WS_SUB_SLOT:
            ESP_LOGI(TAG, "  Slot Update:");
            ESP_LOGI(TAG, "    Current: %llu", event->data.slot.slot);
            ESP_LOGI(TAG, "    Parent: %llu", event->data.slot.parent);
            ESP_LOGI(TAG, "    Root: %llu", event->data.slot.root);
            break;
            
        case ESPSOL_WS_SUB_SIGNATURE:
            ESP_LOGI(TAG, "  Signature Status:");
            if (event->data.signature.err[0] != '\0') {
                ESP_LOGE(TAG, "    Error: %s", event->data.signature.err);
            } else {
                ESP_LOGI(TAG, "    Confirmed!");
            }
            break;
            
        default:
            ESP_LOGI(TAG, "  (Unknown subscription type)");
            break;
        }
        break;
    }
}

/* ============================================================================
 * Main Application
 * ========================================================================== */

void app_main(void)
{
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  ESPSOL WebSocket Subscription Demo   ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    /* Connect to WiFi */
    ESP_LOGI(TAG, "📡 Connecting to WiFi...");
    wifi_init();
    ESP_LOGI(TAG, "✓ WiFi connected");
    ESP_LOGI(TAG, "");
    
    /* Wait a moment for network to stabilize */
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    /* Initialize WebSocket client */
    ESP_LOGI(TAG, "🔌 Initializing WebSocket client...");
    espsol_ws_handle_t ws_handle = NULL;
    ret = espsol_ws_init(&ws_handle, ws_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to initialize WebSocket: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "✓ WebSocket client initialized");
    ESP_LOGI(TAG, "");
    
    /* Wait for connection */
    ESP_LOGI(TAG, "⏳ Waiting for WebSocket connection...");
    bool connected = false;
    for (int i = 0; i < 20; i++) {
        vTaskDelay(pdMS_TO_TICKS(500));
        espsol_ws_is_connected(ws_handle, &connected);
        if (connected) break;
    }
    
    if (!connected) {
        ESP_LOGE(TAG, "✗ WebSocket connection timeout");
        espsol_ws_cleanup(ws_handle);
        return;
    }
    ESP_LOGI(TAG, "✓ WebSocket connected!");
    ESP_LOGI(TAG, "");
    
    /* Subscribe to account changes */
    ESP_LOGI(TAG, "📝 Subscribing to account: %s", WRAPPED_SOL_MINT);
    ret = espsol_ws_account_subscribe(ws_handle, WRAPPED_SOL_MINT, &account_sub_id);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Account subscription ID: %d", account_sub_id);
    } else {
        ESP_LOGE(TAG, "✗ Failed to subscribe: %s", esp_err_to_name(ret));
    }
    ESP_LOGI(TAG, "");
    
    /* Subscribe to transaction logs (all transactions) */
    ESP_LOGI(TAG, "📝 Subscribing to transaction logs (all)");
    ret = espsol_ws_logs_subscribe(ws_handle, "all", &logs_sub_id);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Logs subscription ID: %d", logs_sub_id);
    } else {
        ESP_LOGE(TAG, "✗ Failed to subscribe: %s", esp_err_to_name(ret));
    }
    ESP_LOGI(TAG, "");
    
    /* Subscribe to slot notifications */
    ESP_LOGI(TAG, "📝 Subscribing to slot notifications");
    ret = espsol_ws_slot_subscribe(ws_handle, &slot_sub_id);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Slot subscription ID: %d", slot_sub_id);
    } else {
        ESP_LOGE(TAG, "✗ Failed to subscribe: %s", esp_err_to_name(ret));
    }
    ESP_LOGI(TAG, "");
    
    ESP_LOGI(TAG, "🎧 Listening for notifications...");
    ESP_LOGI(TAG, "(Press Ctrl+C to stop)");
    ESP_LOGI(TAG, "");
    
    /* Keep running and receiving notifications */
    /* In a real application, you would do your work here */
    /* The event handler will be called automatically for notifications */
    
    for (int i = 0; i < 60; i++) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        /* Check connection status every 10 seconds */
        if (i % 10 == 0) {
            espsol_ws_is_connected(ws_handle, &connected);
            if (!connected) {
                ESP_LOGW(TAG, "⚠ WebSocket disconnected, waiting for reconnect...");
            }
        }
    }
    
    /* Cleanup after 60 seconds (for demo purposes) */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "🛑 Stopping subscriptions...");
    
    if (account_sub_id >= 0) {
        espsol_ws_account_unsubscribe(ws_handle, account_sub_id);
        ESP_LOGI(TAG, "✓ Unsubscribed from account");
    }
    
    if (logs_sub_id >= 0) {
        espsol_ws_logs_unsubscribe(ws_handle, logs_sub_id);
        ESP_LOGI(TAG, "✓ Unsubscribed from logs");
    }
    
    if (slot_sub_id >= 0) {
        espsol_ws_slot_unsubscribe(ws_handle, slot_sub_id);
        ESP_LOGI(TAG, "✓ Unsubscribed from slots");
    }
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "🧹 Cleaning up...");
    espsol_ws_cleanup(ws_handle);
    ESP_LOGI(TAG, "✓ Done!");
}
