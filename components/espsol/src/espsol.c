/**
 * @file espsol.c
 * @brief ESPSOL Core SDK Implementation
 *
 * Implements the core SDK initialization, configuration management,
 * and version information.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include "espsol.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "espsol";

/* ============================================================================
 * Internal State
 * ========================================================================== */

static bool s_initialized = false;
static espsol_config_t s_config;

/* ============================================================================
 * Error Name Mapping
 * ========================================================================== */

typedef struct {
    esp_err_t code;
    const char *name;
} espsol_err_entry_t;

static const espsol_err_entry_t s_error_names[] = {
    { ESP_ERR_ESPSOL_INVALID_ARG,       "Invalid argument" },
    { ESP_ERR_ESPSOL_BUFFER_TOO_SMALL,  "Buffer too small" },
    { ESP_ERR_ESPSOL_ENCODING_FAILED,   "Encoding failed" },
    { ESP_ERR_ESPSOL_INVALID_BASE58,    "Invalid Base58 input" },
    { ESP_ERR_ESPSOL_INVALID_BASE64,    "Invalid Base64 input" },
    { ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT,  "Keypair not initialized" },
    { ESP_ERR_ESPSOL_SIGNATURE_INVALID, "Signature verification failed" },
    { ESP_ERR_ESPSOL_RPC_FAILED,        "RPC request failed" },
    { ESP_ERR_ESPSOL_RPC_PARSE_ERROR,   "RPC response parse error" },
    { ESP_ERR_ESPSOL_TX_BUILD_ERROR,    "Transaction build error" },
    { ESP_ERR_ESPSOL_TX_NOT_SIGNED,     "Transaction not signed" },
    { ESP_ERR_ESPSOL_MAX_ACCOUNTS,      "Maximum accounts exceeded" },
    { ESP_ERR_ESPSOL_MAX_INSTRUCTIONS,  "Maximum instructions exceeded" },
    { ESP_ERR_ESPSOL_NVS_ERROR,         "NVS storage error" },
    { ESP_ERR_ESPSOL_CRYPTO_ERROR,      "Crypto operation failed" },
    { ESP_ERR_ESPSOL_NETWORK_ERROR,     "Network error" },
    { ESP_ERR_ESPSOL_TIMEOUT,           "Operation timeout" },
    { ESP_ERR_ESPSOL_NOT_INITIALIZED,   "Component not initialized" },
};

#define NUM_ERROR_ENTRIES (sizeof(s_error_names) / sizeof(s_error_names[0]))

/* ============================================================================
 * Public Functions
 * ========================================================================== */

esp_err_t espsol_init(const espsol_config_t *config)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "ESPSOL already initialized");
        return ESP_OK;
    }

    /* Apply default configuration if not provided */
    if (config == NULL) {
        espsol_config_t default_config = ESPSOL_CONFIG_DEFAULT();
        memcpy(&s_config, &default_config, sizeof(espsol_config_t));
    } else {
        memcpy(&s_config, config, sizeof(espsol_config_t));
    }

    /* Validate configuration */
    if (s_config.rpc_url == NULL || strlen(s_config.rpc_url) == 0) {
        ESP_LOGE(TAG, "Invalid RPC URL");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_config.timeout_ms < 1000) {
        ESP_LOGW(TAG, "Timeout too low, setting to 1000ms");
        s_config.timeout_ms = 1000;
    }

    s_initialized = true;

    ESP_LOGI(TAG, "ESPSOL v%s initialized", ESPSOL_VERSION_STRING);
    ESP_LOGI(TAG, "RPC: %s, Timeout: %lu ms, Commitment: %d",
             s_config.rpc_url,
             (unsigned long)s_config.timeout_ms,
             (int)s_config.commitment);

    return ESP_OK;
}

esp_err_t espsol_deinit(void)
{
    if (!s_initialized) {
        return ESP_ERR_ESPSOL_NOT_INITIALIZED;
    }

    /* Clear configuration */
    memset(&s_config, 0, sizeof(espsol_config_t));
    s_initialized = false;

    ESP_LOGI(TAG, "ESPSOL deinitialized");
    return ESP_OK;
}

bool espsol_is_initialized(void)
{
    return s_initialized;
}

const char* espsol_get_version(void)
{
    return ESPSOL_VERSION_STRING;
}

void espsol_get_version_numbers(uint8_t *major, uint8_t *minor, uint8_t *patch)
{
    if (major) *major = ESPSOL_VERSION_MAJOR;
    if (minor) *minor = ESPSOL_VERSION_MINOR;
    if (patch) *patch = ESPSOL_VERSION_PATCH;
}

const char* espsol_err_to_name(esp_err_t err)
{
    /* Check ESPSOL-specific errors */
    for (size_t i = 0; i < NUM_ERROR_ENTRIES; i++) {
        if (s_error_names[i].code == err) {
            return s_error_names[i].name;
        }
    }

    /* Fall back to ESP-IDF error names */
    return esp_err_to_name(err);
}

const espsol_config_t* espsol_get_config(void)
{
    if (!s_initialized) {
        return NULL;
    }
    return &s_config;
}
