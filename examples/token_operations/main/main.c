/**
 * @file main.c
 * @brief ESPSOL Token Operations Example
 *
 * This example demonstrates SPL Token operations:
 * - Query token accounts
 * - Check token balances
 * - Build token transfer transactions
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

static const char *TAG = "token_ops";

/* WiFi credentials */
#define WIFI_SSID      "YOUR_WIFI_SSID"
#define WIFI_PASSWORD  "YOUR_WIFI_PASSWORD"

/* Well-known SPL tokens on devnet for testing */
#define USDC_MINT_DEVNET "4zMMC9srt5Ri5X14GAgXhaHii3GnPAEERYPJgZJDncDU"

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
    ESP_LOGI(TAG, "║    ESPSOL Token Operations Example     ║");
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
    
    /* ==================== Create Wallets ==================== */
    ESP_LOGI(TAG, "\n=== Create Wallets ===");
    
    espsol_keypair_t owner, mint_authority;
    ESP_ERROR_CHECK(espsol_keypair_generate(&owner));
    ESP_ERROR_CHECK(espsol_keypair_generate(&mint_authority));
    
    char owner_addr[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&owner, owner_addr, sizeof(owner_addr));
    ESP_LOGI(TAG, "Owner address: %s", owner_addr);
    
    /* ==================== Token Program Constants ==================== */
    ESP_LOGI(TAG, "\n=== SPL Token Program Info ===");
    
    char token_program[ESPSOL_ADDRESS_MAX_LEN];
    espsol_pubkey_to_address(ESPSOL_TOKEN_PROGRAM_ID, token_program, sizeof(token_program));
    ESP_LOGI(TAG, "Token Program: %s", token_program);
    
    char ata_program[ESPSOL_ADDRESS_MAX_LEN];
    espsol_pubkey_to_address(ESPSOL_ATA_PROGRAM_ID, ata_program, sizeof(ata_program));
    ESP_LOGI(TAG, "ATA Program:   %s", ata_program);
    
    /* ==================== Query Token Accounts ==================== */
    ESP_LOGI(TAG, "\n=== Query Token Accounts ===");
    ESP_LOGI(TAG, "Looking for token accounts owned by: %s", owner_addr);
    
    espsol_token_account_t accounts[5];
    size_t count = 5;
    
    ret = espsol_rpc_get_token_accounts_by_owner(rpc, owner_addr, NULL,
                                                  accounts, &count);
    if (ret == ESP_OK) {
        if (count == 0) {
            ESP_LOGI(TAG, "No token accounts found (new wallet)");
        } else {
            ESP_LOGI(TAG, "Found %zu token account(s):", count);
            for (size_t i = 0; i < count; i++) {
                ESP_LOGI(TAG, "  [%zu] Account: %s", i+1, accounts[i].address);
                ESP_LOGI(TAG, "       Mint: %s", accounts[i].mint);
                ESP_LOGI(TAG, "       Balance: %llu (decimals: %u)", 
                         (unsigned long long)accounts[i].amount, accounts[i].decimals);
            }
        }
    } else {
        ESP_LOGW(TAG, "Failed to query token accounts: %s", esp_err_to_name(ret));
    }
    
    /* ==================== Build Token Instructions ==================== */
    ESP_LOGI(TAG, "\n=== Build Token Instructions (Demo) ===");
    
    /* Note: These instructions are just built, not executed.
     * To execute, you would need to:
     * 1. Have SOL for rent
     * 2. Have the mint authority for InitializeMint
     * 3. Have the token account owner for transfers
     */
    
    /* Create a mock mint and token account for instruction building */
    uint8_t mock_mint[32], mock_token_account[32], mock_destination[32];
    espsol_random_bytes(mock_mint, 32);
    espsol_random_bytes(mock_token_account, 32);
    espsol_random_bytes(mock_destination, 32);
    
    espsol_transaction_t tx;
    espsol_tx_init(&tx);
    
    /* Demo: Initialize Mint instruction */
    ESP_LOGI(TAG, "\nBuilding InitializeMint instruction...");
    ret = espsol_tx_add_token_initialize_mint(
        &tx,
        mock_mint,              /* mint address */
        6,                      /* decimals */
        mint_authority.public_key,  /* mint authority */
        NULL                    /* freeze authority (optional) */
    );
    ESP_LOGI(TAG, "  Result: %s", ret == ESP_OK ? "OK" : esp_err_to_name(ret));
    
    /* Demo: Transfer instruction */
    ESP_LOGI(TAG, "\nBuilding Transfer instruction...");
    ret = espsol_tx_add_token_transfer(
        &tx,
        mock_token_account,     /* source */
        mock_destination,       /* destination */
        owner.public_key,       /* owner/authority */
        1000000                 /* amount (1 token with 6 decimals) */
    );
    ESP_LOGI(TAG, "  Result: %s", ret == ESP_OK ? "OK" : esp_err_to_name(ret));
    
    /* Demo: MintTo instruction */
    ESP_LOGI(TAG, "\nBuilding MintTo instruction...");
    ret = espsol_tx_add_token_mint_to(
        &tx,
        mock_mint,              /* mint */
        mock_token_account,     /* destination */
        mint_authority.public_key,  /* mint authority */
        1000000000              /* amount (1000 tokens with 6 decimals) */
    );
    ESP_LOGI(TAG, "  Result: %s", ret == ESP_OK ? "OK" : esp_err_to_name(ret));
    
    /* Demo: Burn instruction */
    ESP_LOGI(TAG, "\nBuilding Burn instruction...");
    ret = espsol_tx_add_token_burn(
        &tx,
        mock_token_account,     /* token account */
        mock_mint,              /* mint */
        owner.public_key,       /* owner */
        500000                  /* amount to burn */
    );
    ESP_LOGI(TAG, "  Result: %s", ret == ESP_OK ? "OK" : esp_err_to_name(ret));
    
    ESP_LOGI(TAG, "\nTransaction has %zu instruction(s)", tx.message.instruction_count);
    
    /* ==================== ATA Derivation ==================== */
    ESP_LOGI(TAG, "\n=== Associated Token Account Derivation ===");
    
    uint8_t usdc_mint[32];
    size_t mint_len = 32;
    espsol_base58_decode(USDC_MINT_DEVNET, usdc_mint, &mint_len);
    
    uint8_t ata_address[32];
    ret = espsol_derive_ata(owner.public_key, usdc_mint, ata_address);
    
    if (ret == ESP_OK) {
        char ata_str[ESPSOL_ADDRESS_MAX_LEN];
        espsol_pubkey_to_address(ata_address, ata_str, sizeof(ata_str));
        ESP_LOGI(TAG, "Owner: %s", owner_addr);
        ESP_LOGI(TAG, "Mint:  %s", USDC_MINT_DEVNET);
        ESP_LOGI(TAG, "ATA:   %s", ata_str);
    } else {
        ESP_LOGE(TAG, "Failed to derive ATA: %s", esp_err_to_name(ret));
    }
    
    /* ==================== Cleanup ==================== */
    ESP_LOGI(TAG, "\n╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║         Example Complete!              ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    
    espsol_keypair_clear(&owner);
    espsol_keypair_clear(&mint_authority);
    espsol_rpc_deinit(rpc);
    espsol_deinit();
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
