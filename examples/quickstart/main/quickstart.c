/**
 * @file quickstart.c
 * @brief ESPSOL Quickstart Example
 *
 * This example demonstrates all core features of ESPSOL SDK:
 * - SDK initialization and configuration
 * - Keypair generation and management
 * - Base58/Base64 encoding utilities
 * - RPC client operations (balance, slot, blockhash)
 * - Transaction building and signing
 * - SPL Token operations (if enabled)
 *
 * This is a perfect starting point for new ESPSOL projects.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "espsol.h"
#include "espsol_crypto.h"
#include "espsol_rpc.h"
#include "espsol_tx.h"
#include "espsol_utils.h"
#include "espsol_token.h"

static const char *TAG = "quickstart";

/* ============================================================================
 * Configuration
 * ========================================================================== */

/* RPC endpoint - change to ESPSOL_MAINNET_RPC for production */
#define RPC_ENDPOINT ESPSOL_DEVNET_RPC

/* Demo wallet for testing - DO NOT USE IN PRODUCTION */
#define DEMO_WALLET_KEY "demo_wallet"

/* ============================================================================
 * Helper Functions
 * ========================================================================== */

/**
 * @brief Initialize NVS flash storage
 */
static esp_err_t init_nvs(void)
{
    ESP_LOGI(TAG, "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

/**
 * @brief Print a separator line
 */
static void print_separator(const char *title)
{
    ESP_LOGI(TAG, "\n========================================");
    if (title) {
        ESP_LOGI(TAG, " %s", title);
        ESP_LOGI(TAG, "========================================");
    }
}

/**
 * @brief Print keypair information
 */
static void print_keypair_info(const espsol_keypair_t *keypair)
{
    char address[ESPSOL_ADDRESS_MAX_LEN];
    esp_err_t ret = espsol_pubkey_to_base58(keypair->public_key, address, sizeof(address));
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Public Key (Base58): %s", address);
    } else {
        ESP_LOGE(TAG, "Failed to encode public key: %s", esp_err_to_name(ret));
    }
}

/* ============================================================================
 * Demo 1: SDK Initialization
 * ========================================================================== */

static void demo_sdk_init(void)
{
    print_separator("Demo 1: SDK Initialization");

    /* Get SDK version */
    const char *version = espsol_get_version();
    ESP_LOGI(TAG, "ESPSOL SDK Version: %s", version);

    /* Initialize SDK with default config */
    espsol_config_t config = ESPSOL_CONFIG_DEFAULT();
    esp_err_t ret = espsol_init(&config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ SDK initialized successfully");
    } else {
        ESP_LOGE(TAG, "✗ SDK initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    /* Verify initialization */
    ESP_LOGI(TAG, "SDK is ready for Solana operations!");
}

/* ============================================================================
 * Demo 2: Keypair Generation and Management
 * ========================================================================== */

static void demo_keypair_management(espsol_keypair_t *keypair)
{
    print_separator("Demo 2: Keypair Management");

    /* Generate a new keypair */
    ESP_LOGI(TAG, "Generating new Ed25519 keypair...");
    esp_err_t ret = espsol_keypair_generate(keypair);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ Keypair generation failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "✓ Keypair generated successfully");
    print_keypair_info(keypair);

    /* Save keypair to NVS */
    ESP_LOGI(TAG, "Saving keypair to NVS flash...");
    ret = espsol_keypair_save(keypair, DEMO_WALLET_KEY);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Keypair saved to NVS");
    } else {
        ESP_LOGW(TAG, "⚠ Keypair save failed: %s", esp_err_to_name(ret));
    }

    /* Load keypair from NVS */
    ESP_LOGI(TAG, "Loading keypair from NVS...");
    espsol_keypair_t loaded_keypair;
    ret = espsol_keypair_load(&loaded_keypair, DEMO_WALLET_KEY);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Keypair loaded from NVS");
        
        /* Verify loaded keypair matches */
        if (memcmp(keypair->public_key, loaded_keypair.public_key, 
                   ESPSOL_PUBKEY_SIZE) == 0) {
            ESP_LOGI(TAG, "✓ Loaded keypair matches original");
        } else {
            ESP_LOGE(TAG, "✗ Loaded keypair doesn't match!");
        }
    } else {
        ESP_LOGW(TAG, "⚠ Keypair load failed: %s", esp_err_to_name(ret));
    }
}

/* ============================================================================
 * Demo 3: Encoding Utilities
 * ========================================================================== */

static void demo_encoding_utils(const espsol_keypair_t *keypair)
{
    print_separator("Demo 3: Encoding Utilities");

    char encoded[128];
    uint8_t decoded[64];
    size_t decoded_len;
    esp_err_t ret;

    /* Base58 encoding */
    ESP_LOGI(TAG, "Testing Base58 encoding...");
    ret = espsol_base58_encode(keypair->public_key, ESPSOL_PUBKEY_SIZE, 
                                 encoded, sizeof(encoded));
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Base58 encode: %s", encoded);
        
        /* Base58 decoding */
        ret = espsol_base58_decode(encoded, decoded, sizeof(decoded), &decoded_len);
        if (ret == ESP_OK && decoded_len == ESPSOL_PUBKEY_SIZE) {
            ESP_LOGI(TAG, "✓ Base58 decode: %zu bytes", decoded_len);
            
            /* Verify round-trip */
            if (memcmp(keypair->public_key, decoded, ESPSOL_PUBKEY_SIZE) == 0) {
                ESP_LOGI(TAG, "✓ Base58 round-trip successful");
            } else {
                ESP_LOGE(TAG, "✗ Base58 round-trip failed!");
            }
        } else {
            ESP_LOGE(TAG, "✗ Base58 decode failed: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGE(TAG, "✗ Base58 encode failed: %s", esp_err_to_name(ret));
    }

    /* Base64 encoding */
    ESP_LOGI(TAG, "Testing Base64 encoding...");
    const char *test_data = "Hello Solana!";
    ret = espsol_base64_encode((const uint8_t *)test_data, strlen(test_data),
                                encoded, sizeof(encoded));
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Base64 encode: %s", encoded);
        
        /* Base64 decoding */
        ret = espsol_base64_decode(encoded, decoded, sizeof(decoded), &decoded_len);
        if (ret == ESP_OK) {
            decoded[decoded_len] = '\0'; // Null-terminate for printing
            ESP_LOGI(TAG, "✓ Base64 decode: %s", (char *)decoded);
        } else {
            ESP_LOGE(TAG, "✗ Base64 decode failed: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGE(TAG, "✗ Base64 encode failed: %s", esp_err_to_name(ret));
    }

    /* Address validation */
    ESP_LOGI(TAG, "Testing address validation...");
    const char *valid_addr = "11111111111111111111111111111111";  // System Program
    const char *invalid_addr = "invalid_address_123";
    
    bool is_valid = espsol_is_valid_address(valid_addr);
    ESP_LOGI(TAG, "✓ Valid address check: %s -> %s", valid_addr, 
             is_valid ? "VALID" : "INVALID");
    
    is_valid = espsol_is_valid_address(invalid_addr);
    ESP_LOGI(TAG, "✓ Invalid address check: %s -> %s", invalid_addr,
             is_valid ? "VALID" : "INVALID");
}

/* ============================================================================
 * Demo 4: Cryptographic Operations
 * ========================================================================== */

static void demo_crypto_operations(const espsol_keypair_t *keypair)
{
    print_separator("Demo 4: Cryptographic Operations");

    /* Message to sign */
    const char *message = "Solana on ESP32!";
    uint8_t signature[ESPSOL_SIGNATURE_SIZE];

    /* Sign message */
    ESP_LOGI(TAG, "Signing message: \"%s\"", message);
    esp_err_t ret = espsol_sign((const uint8_t *)message, strlen(message),
                                 signature, keypair);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Message signed successfully");
        
        /* Print signature (first 16 bytes) */
        ESP_LOGI(TAG, "Signature (hex): %02x%02x%02x%02x...%02x%02x%02x%02x",
                 signature[0], signature[1], signature[2], signature[3],
                 signature[60], signature[61], signature[62], signature[63]);
        
        /* Verify signature */
        ESP_LOGI(TAG, "Verifying signature...");
        ret = espsol_verify((const uint8_t *)message, strlen(message),
                            signature, keypair->public_key);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "✓ Signature verified successfully!");
        } else {
            ESP_LOGE(TAG, "✗ Signature verification failed: %s", 
                     esp_err_to_name(ret));
        }
        
        /* Test invalid signature detection */
        ESP_LOGI(TAG, "Testing invalid signature detection...");
        signature[0] ^= 0xFF; // Corrupt signature
        ret = espsol_verify((const uint8_t *)message, strlen(message),
                            signature, keypair->public_key);
        if (ret == ESP_ERR_ESPSOL_INVALID_SIGNATURE) {
            ESP_LOGI(TAG, "✓ Invalid signature correctly detected");
        } else {
            ESP_LOGE(TAG, "✗ Invalid signature not detected!");
        }
    } else {
        ESP_LOGE(TAG, "✗ Message signing failed: %s", esp_err_to_name(ret));
    }
}

/* ============================================================================
 * Demo 5: RPC Client Operations
 * ========================================================================== */

static void demo_rpc_operations(const espsol_keypair_t *keypair)
{
    print_separator("Demo 5: RPC Client Operations");

    /* Initialize RPC client */
    ESP_LOGI(TAG, "Initializing RPC client...");
    ESP_LOGI(TAG, "Endpoint: %s", RPC_ENDPOINT);
    
    espsol_rpc_handle_t rpc;
    esp_err_t ret = espsol_rpc_init(&rpc, RPC_ENDPOINT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ RPC init failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "✓ RPC client initialized");

    /* Get network version */
    ESP_LOGI(TAG, "Querying network version...");
    char version[128];
    ret = espsol_rpc_get_version(rpc, version, sizeof(version));
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Network version: %s", version);
    } else {
        ESP_LOGW(TAG, "⚠ Get version failed: %s", esp_err_to_name(ret));
    }

    /* Get current slot */
    ESP_LOGI(TAG, "Querying current slot...");
    uint64_t slot;
    ret = espsol_rpc_get_slot(rpc, &slot);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Current slot: %" PRIu64, slot);
    } else {
        ESP_LOGW(TAG, "⚠ Get slot failed: %s", esp_err_to_name(ret));
    }

    /* Get account balance */
    ESP_LOGI(TAG, "Querying account balance...");
    char address[ESPSOL_ADDRESS_MAX_LEN];
    espsol_pubkey_to_base58(keypair->public_key, address, sizeof(address));
    
    uint64_t balance;
    ret = espsol_rpc_get_balance(rpc, address, &balance);
    if (ret == ESP_OK) {
        double sol_balance = (double)balance / ESPSOL_LAMPORTS_PER_SOL;
        ESP_LOGI(TAG, "✓ Balance: %" PRIu64 " lamports (%.9f SOL)", 
                 balance, sol_balance);
        
        if (balance == 0) {
            ESP_LOGI(TAG, "💡 Tip: Get free devnet SOL at https://faucet.solana.com");
            ESP_LOGI(TAG, "    Address: %s", address);
        }
    } else {
        ESP_LOGW(TAG, "⚠ Get balance failed: %s", esp_err_to_name(ret));
    }

    /* Get latest blockhash */
    ESP_LOGI(TAG, "Querying latest blockhash...");
    char blockhash[ESPSOL_BLOCKHASH_MAX_LEN];
    uint64_t last_valid_height;
    ret = espsol_rpc_get_latest_blockhash(rpc, blockhash, sizeof(blockhash),
                                           &last_valid_height);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Latest blockhash: %s", blockhash);
        ESP_LOGI(TAG, "  Last valid height: %" PRIu64, last_valid_height);
    } else {
        ESP_LOGW(TAG, "⚠ Get blockhash failed: %s", esp_err_to_name(ret));
    }

    /* Cleanup */
    espsol_rpc_deinit(rpc);
    ESP_LOGI(TAG, "✓ RPC client cleaned up");
}

/* ============================================================================
 * Demo 6: Transaction Building
 * ========================================================================== */

static void demo_transaction_building(const espsol_keypair_t *keypair)
{
    print_separator("Demo 6: Transaction Building");

    /* Create transaction */
    ESP_LOGI(TAG, "Creating transaction...");
    espsol_tx_t *tx;
    esp_err_t ret = espsol_tx_create(&tx);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ Transaction creation failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "✓ Transaction created");

    /* Set fee payer */
    ret = espsol_tx_set_fee_payer(tx, keypair->public_key);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Fee payer set");
    } else {
        ESP_LOGE(TAG, "✗ Set fee payer failed: %s", esp_err_to_name(ret));
        espsol_tx_destroy(tx);
        return;
    }

    /* Set blockhash (example - normally get from RPC) */
    const char *example_blockhash = "4sGjMW1sUnHzSxGspuhpqLDx6wiyjNtZAMdL4VZHirAn";
    uint8_t blockhash_bytes[32];
    size_t decoded_len;
    ret = espsol_base58_decode(example_blockhash, blockhash_bytes, 
                                sizeof(blockhash_bytes), &decoded_len);
    if (ret == ESP_OK) {
        ret = espsol_tx_set_blockhash(tx, blockhash_bytes);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "✓ Blockhash set");
        } else {
            ESP_LOGE(TAG, "✗ Set blockhash failed: %s", esp_err_to_name(ret));
        }
    }

    /* Add transfer instruction */
    ESP_LOGI(TAG, "Adding transfer instruction...");
    const char *recipient = "11111111111111111111111111111111"; // System Program as example
    uint8_t recipient_bytes[32];
    ret = espsol_base58_decode(recipient, recipient_bytes, 
                                sizeof(recipient_bytes), &decoded_len);
    if (ret == ESP_OK) {
        ret = espsol_tx_add_transfer(tx, keypair->public_key, recipient_bytes, 
                                      1000000); // 0.001 SOL
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "✓ Transfer instruction added (0.001 SOL)");
        } else {
            ESP_LOGE(TAG, "✗ Add transfer failed: %s", esp_err_to_name(ret));
        }
    }

    /* Sign transaction */
    ESP_LOGI(TAG, "Signing transaction...");
    ret = espsol_tx_sign(tx, keypair);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Transaction signed");
    } else {
        ESP_LOGE(TAG, "✗ Transaction signing failed: %s", esp_err_to_name(ret));
    }

    /* Serialize to Base64 */
    ESP_LOGI(TAG, "Serializing transaction...");
    char tx_base64[2048];
    ret = espsol_tx_to_base64(tx, tx_base64, sizeof(tx_base64));
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Transaction serialized to Base64");
        ESP_LOGI(TAG, "  Length: %zu bytes", strlen(tx_base64));
        ESP_LOGI(TAG, "  Preview: %.60s...", tx_base64);
    } else {
        ESP_LOGE(TAG, "✗ Serialization failed: %s", esp_err_to_name(ret));
    }

    /* Cleanup */
    espsol_tx_destroy(tx);
    ESP_LOGI(TAG, "✓ Transaction destroyed");
}

/* ============================================================================
 * Demo 7: SPL Token Operations (if enabled)
 * ========================================================================== */

#ifdef CONFIG_ESPSOL_ENABLE_SPL_TOKENS
static void demo_token_operations(const espsol_keypair_t *keypair)
{
    print_separator("Demo 7: SPL Token Operations");

    /* Example USDC mint address on devnet */
    const char *usdc_mint = "4zMMC9srt5Ri5X14GAgXhaHii3GnPAEERYPJgZJDncDU";
    uint8_t mint_bytes[32];
    size_t decoded_len;
    
    esp_err_t ret = espsol_base58_decode(usdc_mint, mint_bytes, 
                                          sizeof(mint_bytes), &decoded_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ Invalid mint address");
        return;
    }

    /* Derive Associated Token Account (ATA) */
    ESP_LOGI(TAG, "Deriving ATA for USDC...");
    uint8_t ata[32];
    ret = espsol_token_get_associated_address(keypair->public_key, mint_bytes, ata);
    if (ret == ESP_OK) {
        char ata_str[ESPSOL_ADDRESS_MAX_LEN];
        espsol_pubkey_to_base58(ata, ata_str, sizeof(ata_str));
        ESP_LOGI(TAG, "✓ ATA Address: %s", ata_str);
    } else {
        ESP_LOGE(TAG, "✗ ATA derivation failed: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "✓ Token operations demo complete");
}
#endif

/* ============================================================================
 * Main Application
 * ========================================================================== */

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " ESPSOL SDK Quickstart Example");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "This example demonstrates all core");
    ESP_LOGI(TAG, "features of the ESPSOL SDK.");
    ESP_LOGI(TAG, "");

    /* Initialize NVS */
    ESP_ERROR_CHECK(init_nvs());

    /* Initialize network (for RPC demo) */
    ESP_LOGI(TAG, "Connecting to WiFi...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "✓ WiFi connected");

    /* Keypair for all demos */
    espsol_keypair_t keypair;

    /* Run all demos */
    demo_sdk_init();
    demo_keypair_management(&keypair);
    demo_encoding_utils(&keypair);
    demo_crypto_operations(&keypair);
    demo_rpc_operations(&keypair);
    demo_transaction_building(&keypair);
    
#ifdef CONFIG_ESPSOL_ENABLE_SPL_TOKENS
    demo_token_operations(&keypair);
#endif

    /* Final summary */
    print_separator("Quickstart Complete!");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "🎉 All demos completed successfully!");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Next steps:");
    ESP_LOGI(TAG, "1. Get devnet SOL from https://faucet.solana.com");
    ESP_LOGI(TAG, "2. Modify this example for your use case");
    ESP_LOGI(TAG, "3. See other examples in examples/ directory");
    ESP_LOGI(TAG, "4. Read documentation at docs/");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Happy building on Solana + ESP32! 🚀");
    ESP_LOGI(TAG, "");

    /* Cleanup */
    espsol_deinit();
}
