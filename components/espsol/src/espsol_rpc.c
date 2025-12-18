/**
 * @file espsol_rpc.c
 * @brief ESPSOL RPC Client Implementation
 *
 * JSON-RPC 2.0 client for Solana RPC nodes using native ESP-IDF
 * esp_http_client and cJSON.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include "espsol_rpc.h"
#include "espsol_utils.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(ESP_PLATFORM) && ESP_PLATFORM
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
/* Host compilation stubs */
#define ESP_LOGI(tag, ...)
#define ESP_LOGW(tag, ...)
#define ESP_LOGE(tag, ...)
#define ESP_LOGD(tag, ...)
#endif

static const char *TAG = "espsol_rpc";

/* ============================================================================
 * RPC Client Internal Structure
 * ========================================================================== */

struct espsol_rpc_client {
    char *endpoint;                     /**< RPC endpoint URL */
    uint32_t timeout_ms;                /**< Request timeout */
    espsol_commitment_t commitment;     /**< Default commitment level */
    size_t buffer_size;                 /**< HTTP response buffer size */
    uint32_t request_id;                /**< JSON-RPC request ID counter */
    char last_error[256];               /**< Last error message */
    uint8_t max_retries;                /**< Max retry attempts */
    uint32_t retry_delay_ms;            /**< Initial retry delay */
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    esp_http_client_handle_t http_client;  /**< HTTP client handle */
#endif
    char *response_buffer;              /**< Response buffer */
};

/* ============================================================================
 * Internal Helper Functions
 * ========================================================================== */

/**
 * @brief Get commitment level as JSON-RPC string
 */
const char *espsol_commitment_to_str(espsol_commitment_t commitment)
{
    switch (commitment) {
        case ESPSOL_COMMITMENT_PROCESSED:
            return "processed";
        case ESPSOL_COMMITMENT_CONFIRMED:
            return "confirmed";
        case ESPSOL_COMMITMENT_FINALIZED:
            return "finalized";
        default:
            return "confirmed";
    }
}

#if defined(ESP_PLATFORM) && ESP_PLATFORM

/**
 * @brief HTTP event handler for esp_http_client
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    struct espsol_rpc_client *client = (struct espsol_rpc_client *)evt->user_data;
    
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER: %s: %s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (client && client->response_buffer && evt->data_len > 0) {
                size_t current_len = strlen(client->response_buffer);
                if (current_len + evt->data_len < client->buffer_size - 1) {
                    memcpy(client->response_buffer + current_len, evt->data, evt->data_len);
                    client->response_buffer[current_len + evt->data_len] = '\0';
                }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

/**
 * @brief Build JSON-RPC 2.0 request
 */
static char *build_jsonrpc_request(struct espsol_rpc_client *client,
                                    const char *method,
                                    cJSON *params)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return NULL;
    }
    
    cJSON_AddStringToObject(root, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(root, "id", ++client->request_id);
    cJSON_AddStringToObject(root, "method", method);
    
    if (params) {
        cJSON_AddItemToObject(root, "params", params);
    } else {
        cJSON_AddItemToObject(root, "params", cJSON_CreateArray());
    }
    
    char *request = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    return request;
}

/**
 * @brief Execute JSON-RPC request with retry logic
 */
static esp_err_t execute_rpc_request_internal(struct espsol_rpc_client *client,
                                               const char *request_body,
                                               cJSON **result)
{
    if (!client || !request_body || !result) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *result = NULL;
    client->last_error[0] = '\0';
    
    /* Clear response buffer */
    memset(client->response_buffer, 0, client->buffer_size);
    
    /* Set POST data */
    esp_http_client_set_post_field(client->http_client, request_body, strlen(request_body));
    
    ESP_LOGD(TAG, "RPC Request: %s", request_body);
    
    /* Perform HTTP request */
    esp_err_t err = esp_http_client_perform(client->http_client);
    if (err != ESP_OK) {
        snprintf(client->last_error, sizeof(client->last_error), 
                 "HTTP request failed: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", client->last_error);
        return ESP_ERR_ESPSOL_NETWORK_ERROR;
    }
    
    int status_code = esp_http_client_get_status_code(client->http_client);
    if (status_code != 200) {
        snprintf(client->last_error, sizeof(client->last_error),
                 "HTTP error: status code %d", status_code);
        ESP_LOGE(TAG, "%s", client->last_error);
        /* Return special error for rate limiting (429) to allow retry */
        if (status_code == 429) {
            return ESP_ERR_ESPSOL_RATE_LIMITED;
        }
        return ESP_ERR_ESPSOL_RPC_FAILED;
    }
    
    ESP_LOGD(TAG, "RPC Response: %s", client->response_buffer);
    
    /* Parse JSON response */
    cJSON *json = cJSON_Parse(client->response_buffer);
    if (!json) {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Failed to parse JSON response");
        ESP_LOGE(TAG, "%s", client->last_error);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    /* Check for JSON-RPC error */
    cJSON *error = cJSON_GetObjectItem(json, "error");
    if (error && cJSON_IsObject(error)) {
        cJSON *error_msg = cJSON_GetObjectItem(error, "message");
        cJSON *error_code = cJSON_GetObjectItem(error, "code");
        snprintf(client->last_error, sizeof(client->last_error),
                 "RPC error %d: %s",
                 error_code ? error_code->valueint : -1,
                 error_msg && error_msg->valuestring ? error_msg->valuestring : "Unknown error");
        ESP_LOGE(TAG, "%s", client->last_error);
        cJSON_Delete(json);
        return ESP_ERR_ESPSOL_RPC_FAILED;
    }
    
    /* Extract result */
    *result = cJSON_DetachItemFromObject(json, "result");
    cJSON_Delete(json);
    
    if (!*result) {
        snprintf(client->last_error, sizeof(client->last_error),
                 "No result in RPC response");
        ESP_LOGE(TAG, "%s", client->last_error);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    return ESP_OK;
}

/**
 * @brief Execute JSON-RPC request with automatic retry and exponential backoff
 */
static esp_err_t execute_rpc_request(struct espsol_rpc_client *client,
                                      const char *request_body,
                                      cJSON **result)
{
    esp_err_t err = ESP_FAIL;
    uint8_t attempt = 0;
    uint32_t delay_ms = client->retry_delay_ms;
    
    while (attempt <= client->max_retries) {
        err = execute_rpc_request_internal(client, request_body, result);
        
        /* Success - return immediately */
        if (err == ESP_OK) {
            return ESP_OK;
        }
        
        /* Don't retry on non-recoverable errors */
        if (err != ESP_ERR_ESPSOL_NETWORK_ERROR && 
            err != ESP_ERR_ESPSOL_RATE_LIMITED) {
            return err;
        }
        
        attempt++;
        
        /* If we have retries left, wait and try again */
        if (attempt <= client->max_retries) {
            ESP_LOGW(TAG, "Request failed, retry %u/%u in %lu ms...", 
                     attempt, client->max_retries, (unsigned long)delay_ms);
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
            delay_ms *= 2;  /* Exponential backoff */
            
            /* Cap delay at 10 seconds */
            if (delay_ms > 10000) {
                delay_ms = 10000;
            }
        }
    }
    
    ESP_LOGE(TAG, "Request failed after %u retries", client->max_retries);
    return err;
}

#endif /* ESP_PLATFORM */

/* ============================================================================
 * Connection Management
 * ========================================================================== */

esp_err_t espsol_rpc_init(espsol_rpc_handle_t *handle, const char *endpoint)
{
    espsol_rpc_config_t config = ESPSOL_RPC_CONFIG_DEFAULT();
    config.endpoint = endpoint;
    return espsol_rpc_init_with_config(handle, &config);
}

esp_err_t espsol_rpc_init_with_config(espsol_rpc_handle_t *handle,
                                       const espsol_rpc_config_t *config)
{
    if (!handle || !config || !config->endpoint) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    /* Allocate client structure */
    struct espsol_rpc_client *client = calloc(1, sizeof(struct espsol_rpc_client));
    if (!client) {
        ESP_LOGE(TAG, "Failed to allocate RPC client");
        return ESP_ERR_NO_MEM;
    }
    
    /* Copy endpoint */
    client->endpoint = strdup(config->endpoint);
    if (!client->endpoint) {
        free(client);
        ESP_LOGE(TAG, "Failed to allocate endpoint string");
        return ESP_ERR_NO_MEM;
    }
    
    /* Set configuration */
    client->timeout_ms = config->timeout_ms;
    client->commitment = config->commitment;
    client->buffer_size = config->buffer_size > 0 ? config->buffer_size : ESPSOL_DEFAULT_BUFFER_SIZE;
    client->request_id = 0;
    client->last_error[0] = '\0';
    client->max_retries = config->max_retries;
    client->retry_delay_ms = config->retry_delay_ms > 0 ? config->retry_delay_ms : 500;
    
    /* Allocate response buffer */
    client->response_buffer = calloc(1, client->buffer_size);
    if (!client->response_buffer) {
        free(client->endpoint);
        free(client);
        ESP_LOGE(TAG, "Failed to allocate response buffer");
        return ESP_ERR_NO_MEM;
    }
    
    /* Initialize HTTP client */
    esp_http_client_config_t http_config = {
        .url = client->endpoint,
        .method = HTTP_METHOD_POST,
        .timeout_ms = client->timeout_ms,
        .event_handler = http_event_handler,
        .user_data = client,
        .buffer_size = client->buffer_size,
        .buffer_size_tx = 1024,
        /* Use ESP-IDF global CA store or skip verification for devnet testing */
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    
    client->http_client = esp_http_client_init(&http_config);
    if (!client->http_client) {
        free(client->response_buffer);
        free(client->endpoint);
        free(client);
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_ERR_ESPSOL_NETWORK_ERROR;
    }
    
    /* Set JSON content type */
    esp_http_client_set_header(client->http_client, "Content-Type", "application/json");
    
    *handle = client;
    ESP_LOGI(TAG, "RPC client initialized: %s", config->endpoint);
    return ESP_OK;
#else
    /* Host compilation - no actual HTTP */
    (void)config;
    *handle = NULL;
    return ESP_OK;
#endif
}

esp_err_t espsol_rpc_deinit(espsol_rpc_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    if (client->http_client) {
        esp_http_client_cleanup(client->http_client);
    }
    
    free(client->response_buffer);
    free(client->endpoint);
    free(client);
    
    ESP_LOGI(TAG, "RPC client deinitialized");
#endif
    
    return ESP_OK;
}

esp_err_t espsol_rpc_set_timeout(espsol_rpc_handle_t handle, uint32_t timeout_ms)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    client->timeout_ms = timeout_ms;
    esp_http_client_set_timeout_ms(client->http_client, timeout_ms);
#else
    (void)timeout_ms;
#endif
    
    return ESP_OK;
}

esp_err_t espsol_rpc_set_commitment(espsol_rpc_handle_t handle,
                                     espsol_commitment_t commitment)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    client->commitment = commitment;
#else
    (void)commitment;
#endif
    
    return ESP_OK;
}

const char *espsol_rpc_get_last_error(espsol_rpc_handle_t handle)
{
    if (!handle) {
        return NULL;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    return client->last_error[0] ? client->last_error : NULL;
#else
    return NULL;
#endif
}

/* ============================================================================
 * Network Information
 * ========================================================================== */

esp_err_t espsol_rpc_get_version(espsol_rpc_handle_t handle,
                                  char *version, size_t len)
{
    if (!handle || !version || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    char *request = build_jsonrpc_request(client, "getVersion", NULL);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    /* Extract version string */
    cJSON *solana_core = cJSON_GetObjectItem(result, "solana-core");
    if (!solana_core || !cJSON_IsString(solana_core)) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    size_t ver_len = strlen(solana_core->valuestring);
    if (ver_len >= len) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    strncpy(version, solana_core->valuestring, len - 1);
    version[len - 1] = '\0';
    
    cJSON_Delete(result);
    return ESP_OK;
#else
    (void)len;
    strcpy(version, "1.18.0");  /* Mock version for host testing */
    return ESP_OK;
#endif
}

esp_err_t espsol_rpc_get_slot(espsol_rpc_handle_t handle, uint64_t *slot)
{
    if (!handle || !slot) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    /* Build params with commitment */
    cJSON *params = cJSON_CreateArray();
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "commitment", espsol_commitment_to_str(client->commitment));
    cJSON_AddItemToArray(params, config);
    
    char *request = build_jsonrpc_request(client, "getSlot", params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    if (!cJSON_IsNumber(result)) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    *slot = (uint64_t)result->valuedouble;
    cJSON_Delete(result);
    return ESP_OK;
#else
    *slot = 123456789;  /* Mock slot for host testing */
    return ESP_OK;
#endif
}

esp_err_t espsol_rpc_get_block_height(espsol_rpc_handle_t handle, uint64_t *height)
{
    if (!handle || !height) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    /* Build params with commitment */
    cJSON *params = cJSON_CreateArray();
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "commitment", espsol_commitment_to_str(client->commitment));
    cJSON_AddItemToArray(params, config);
    
    char *request = build_jsonrpc_request(client, "getBlockHeight", params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    if (!cJSON_IsNumber(result)) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    *height = (uint64_t)result->valuedouble;
    cJSON_Delete(result);
    return ESP_OK;
#else
    *height = 100000000;  /* Mock height for host testing */
    return ESP_OK;
#endif
}

esp_err_t espsol_rpc_get_health(espsol_rpc_handle_t handle, bool *is_healthy)
{
    if (!handle || !is_healthy) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    char *request = build_jsonrpc_request(client, "getHealth", NULL);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        /* Health check returns error if unhealthy */
        *is_healthy = false;
        return ESP_OK;
    }
    
    /* Health returns "ok" string if healthy */
    *is_healthy = cJSON_IsString(result) && 
                  strcmp(result->valuestring, "ok") == 0;
    
    cJSON_Delete(result);
    return ESP_OK;
#else
    *is_healthy = true;  /* Mock healthy for host testing */
    return ESP_OK;
#endif
}

/* ============================================================================
 * Account Operations
 * ========================================================================== */

esp_err_t espsol_rpc_get_balance(espsol_rpc_handle_t handle,
                                  const char *pubkey,
                                  uint64_t *lamports)
{
    if (!handle || !pubkey || !lamports) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    /* Build params: [pubkey, {commitment}] */
    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateString(pubkey));
    
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "commitment", espsol_commitment_to_str(client->commitment));
    cJSON_AddItemToArray(params, config);
    
    char *request = build_jsonrpc_request(client, "getBalance", params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    /* Extract value from result */
    cJSON *value = cJSON_GetObjectItem(result, "value");
    if (!value || !cJSON_IsNumber(value)) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    *lamports = (uint64_t)value->valuedouble;
    cJSON_Delete(result);
    return ESP_OK;
#else
    (void)pubkey;
    *lamports = 1000000000;  /* Mock 1 SOL for host testing */
    return ESP_OK;
#endif
}

esp_err_t espsol_rpc_get_account_info(espsol_rpc_handle_t handle,
                                       const char *pubkey,
                                       espsol_account_info_t *info)
{
    if (!handle || !pubkey || !info) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    /* Build params: [pubkey, {encoding, commitment}] */
    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateString(pubkey));
    
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "encoding", "base64");
    cJSON_AddStringToObject(config, "commitment", espsol_commitment_to_str(client->commitment));
    cJSON_AddItemToArray(params, config);
    
    char *request = build_jsonrpc_request(client, "getAccountInfo", params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    /* Check for null account (not found) */
    cJSON *value = cJSON_GetObjectItem(result, "value");
    if (!value || cJSON_IsNull(value)) {
        /* Account not found - return empty info */
        memset(info, 0, sizeof(espsol_account_info_t));
        cJSON_Delete(result);
        return ESP_OK;
    }
    
    /* Parse account info */
    cJSON *lamports_json = cJSON_GetObjectItem(value, "lamports");
    cJSON *owner_json = cJSON_GetObjectItem(value, "owner");
    cJSON *executable_json = cJSON_GetObjectItem(value, "executable");
    cJSON *rent_epoch_json = cJSON_GetObjectItem(value, "rentEpoch");
    cJSON *data_json = cJSON_GetObjectItem(value, "data");
    
    if (lamports_json && cJSON_IsNumber(lamports_json)) {
        info->lamports = (uint64_t)lamports_json->valuedouble;
    }
    
    if (owner_json && cJSON_IsString(owner_json)) {
        strncpy(info->owner, owner_json->valuestring, sizeof(info->owner) - 1);
        info->owner[sizeof(info->owner) - 1] = '\0';
    }
    
    if (executable_json) {
        info->executable = cJSON_IsTrue(executable_json);
    }
    
    if (rent_epoch_json && cJSON_IsNumber(rent_epoch_json)) {
        info->rent_epoch = (uint64_t)rent_epoch_json->valuedouble;
    }
    
    /* Parse data (base64 encoded) */
    if (data_json && cJSON_IsArray(data_json)) {
        cJSON *data_str = cJSON_GetArrayItem(data_json, 0);
        if (data_str && cJSON_IsString(data_str) && info->data && info->data_capacity > 0) {
            size_t decoded_len = info->data_capacity;
            err = espsol_base64_decode(data_str->valuestring,
                                        info->data, &decoded_len);
            if (err == ESP_OK) {
                info->data_len = decoded_len;
            } else if (err == ESP_ERR_ESPSOL_BUFFER_TOO_SMALL) {
                cJSON_Delete(result);
                return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
            }
        }
    }
    
    cJSON_Delete(result);
    return ESP_OK;
#else
    (void)pubkey;
    memset(info, 0, sizeof(espsol_account_info_t));
    info->lamports = 1000000000;
    strncpy(info->owner, "11111111111111111111111111111111", sizeof(info->owner) - 1);
    return ESP_OK;
#endif
}

/* ============================================================================
 * Blockhash Operations
 * ========================================================================== */

esp_err_t espsol_rpc_get_latest_blockhash(espsol_rpc_handle_t handle,
                                           uint8_t blockhash[ESPSOL_BLOCKHASH_SIZE],
                                           uint64_t *last_valid_block_height)
{
    if (!handle || !blockhash) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    /* Build params with commitment */
    cJSON *params = cJSON_CreateArray();
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "commitment", espsol_commitment_to_str(client->commitment));
    cJSON_AddItemToArray(params, config);
    
    char *request = build_jsonrpc_request(client, "getLatestBlockhash", params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    /* Extract blockhash from result.value */
    cJSON *value = cJSON_GetObjectItem(result, "value");
    if (!value || !cJSON_IsObject(value)) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    cJSON *blockhash_json = cJSON_GetObjectItem(value, "blockhash");
    if (!blockhash_json || !cJSON_IsString(blockhash_json)) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    /* Decode base58 blockhash */
    size_t decoded_len = ESPSOL_BLOCKHASH_SIZE;
    err = espsol_base58_decode(blockhash_json->valuestring,
                               blockhash, &decoded_len);
    if (err != ESP_OK || decoded_len != ESPSOL_BLOCKHASH_SIZE) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    /* Extract last valid block height if requested */
    if (last_valid_block_height) {
        cJSON *height_json = cJSON_GetObjectItem(value, "lastValidBlockHeight");
        if (height_json && cJSON_IsNumber(height_json)) {
            *last_valid_block_height = (uint64_t)height_json->valuedouble;
        }
    }
    
    cJSON_Delete(result);
    return ESP_OK;
#else
    /* Mock blockhash for host testing */
    memset(blockhash, 0xAB, ESPSOL_BLOCKHASH_SIZE);
    if (last_valid_block_height) {
        *last_valid_block_height = 100000150;
    }
    return ESP_OK;
#endif
}

esp_err_t espsol_rpc_get_latest_blockhash_str(espsol_rpc_handle_t handle,
                                               char *blockhash, size_t len,
                                               uint64_t *last_valid_block_height)
{
    if (!handle || !blockhash || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t hash_bytes[ESPSOL_BLOCKHASH_SIZE];
    esp_err_t err = espsol_rpc_get_latest_blockhash(handle, hash_bytes, last_valid_block_height);
    if (err != ESP_OK) {
        return err;
    }
    
    /* Encode to base58 */
    err = espsol_base58_encode(hash_bytes, ESPSOL_BLOCKHASH_SIZE, blockhash, len);
    if (err != ESP_OK) {
        return err;
    }
    
    return ESP_OK;
}

/* ============================================================================
 * Transaction Operations
 * ========================================================================== */

esp_err_t espsol_rpc_send_transaction(espsol_rpc_handle_t handle,
                                       const char *tx_base64,
                                       char *signature, size_t sig_len)
{
    if (!handle || !tx_base64 || !signature || sig_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    /* Build params: [tx_base64, {encoding, preflightCommitment}] */
    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateString(tx_base64));
    
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "encoding", "base64");
    cJSON_AddStringToObject(config, "preflightCommitment", 
                            espsol_commitment_to_str(client->commitment));
    cJSON_AddItemToArray(params, config);
    
    char *request = build_jsonrpc_request(client, "sendTransaction", params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    /* Result is the transaction signature (base58) */
    if (!cJSON_IsString(result)) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    size_t result_len = strlen(result->valuestring);
    if (result_len >= sig_len) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    strncpy(signature, result->valuestring, sig_len - 1);
    signature[sig_len - 1] = '\0';
    
    cJSON_Delete(result);
    return ESP_OK;
#else
    (void)tx_base64;
    strncpy(signature, "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpRzrFmBV6UjKdiSZkQUW", 
            sig_len - 1);
    signature[sig_len - 1] = '\0';
    return ESP_OK;
#endif
}

esp_err_t espsol_rpc_get_transaction(espsol_rpc_handle_t handle,
                                      const char *signature,
                                      espsol_tx_response_t *response)
{
    if (!handle || !signature || !response) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    /* Build params: [signature, {encoding, commitment}] */
    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateString(signature));
    
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "encoding", "json");
    cJSON_AddStringToObject(config, "commitment", espsol_commitment_to_str(client->commitment));
    cJSON_AddNumberToObject(config, "maxSupportedTransactionVersion", 0);
    cJSON_AddItemToArray(params, config);
    
    char *request = build_jsonrpc_request(client, "getTransaction", params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    /* Initialize response */
    memset(response, 0, sizeof(espsol_tx_response_t));
    strncpy(response->signature, signature, sizeof(response->signature) - 1);
    
    if (cJSON_IsNull(result)) {
        /* Transaction not found */
        response->confirmed = false;
        cJSON_Delete(result);
        return ESP_OK;
    }
    
    /* Parse transaction info */
    cJSON *slot_json = cJSON_GetObjectItem(result, "slot");
    if (slot_json && cJSON_IsNumber(slot_json)) {
        response->slot = (uint64_t)slot_json->valuedouble;
    }
    
    /* Check for transaction error */
    cJSON *meta = cJSON_GetObjectItem(result, "meta");
    if (meta && cJSON_IsObject(meta)) {
        cJSON *meta_err = cJSON_GetObjectItem(meta, "err");
        if (meta_err && !cJSON_IsNull(meta_err)) {
            char *err_str = cJSON_PrintUnformatted(meta_err);
            if (err_str) {
                strncpy(response->error, err_str, sizeof(response->error) - 1);
                response->error[sizeof(response->error) - 1] = '\0';
                free(err_str);
            }
            response->confirmed = false;
        } else {
            response->confirmed = true;
        }
    }
    
    cJSON_Delete(result);
    return ESP_OK;
#else
    memset(response, 0, sizeof(espsol_tx_response_t));
    strncpy(response->signature, signature, sizeof(response->signature) - 1);
    response->slot = 123456789;
    response->confirmed = true;
    return ESP_OK;
#endif
}

esp_err_t espsol_rpc_confirm_transaction(espsol_rpc_handle_t handle,
                                          const char *signature,
                                          uint32_t timeout_ms,
                                          bool *confirmed)
{
    if (!handle || !signature || !confirmed) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    uint32_t elapsed = 0;
    const uint32_t poll_interval = 500;  /* Poll every 500ms */
    
    while (elapsed < timeout_ms) {
        espsol_tx_response_t response;
        esp_err_t err = espsol_rpc_get_transaction(handle, signature, &response);
        
        if (err != ESP_OK) {
            return err;
        }
        
        if (response.confirmed) {
            *confirmed = true;
            return ESP_OK;
        }
        
        if (response.error[0] != '\0') {
            /* Transaction failed */
            *confirmed = false;
            return ESP_OK;
        }
        
        vTaskDelay(pdMS_TO_TICKS(poll_interval));
        elapsed += poll_interval;
    }
    
    *confirmed = false;
    return ESP_ERR_ESPSOL_TIMEOUT;
#else
    (void)signature;
    (void)timeout_ms;
    *confirmed = true;
    return ESP_OK;
#endif
}

esp_err_t espsol_rpc_get_signature_statuses(espsol_rpc_handle_t handle,
                                             const char **signatures,
                                             size_t count,
                                             bool *confirmed)
{
    if (!handle || !signatures || count == 0 || !confirmed) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    /* Build params: [[signatures], {searchTransactionHistory}] */
    cJSON *params = cJSON_CreateArray();
    
    cJSON *sig_array = cJSON_CreateArray();
    for (size_t i = 0; i < count; i++) {
        cJSON_AddItemToArray(sig_array, cJSON_CreateString(signatures[i]));
    }
    cJSON_AddItemToArray(params, sig_array);
    
    cJSON *config = cJSON_CreateObject();
    cJSON_AddBoolToObject(config, "searchTransactionHistory", true);
    cJSON_AddItemToArray(params, config);
    
    char *request = build_jsonrpc_request(client, "getSignatureStatuses", params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    /* Parse value array */
    cJSON *value = cJSON_GetObjectItem(result, "value");
    if (!value || !cJSON_IsArray(value)) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    size_t array_size = cJSON_GetArraySize(value);
    for (size_t i = 0; i < count && i < array_size; i++) {
        cJSON *status = cJSON_GetArrayItem(value, i);
        if (cJSON_IsNull(status)) {
            confirmed[i] = false;
        } else if (cJSON_IsObject(status)) {
            cJSON *status_err = cJSON_GetObjectItem(status, "err");
            confirmed[i] = (status_err == NULL || cJSON_IsNull(status_err));
        } else {
            confirmed[i] = false;
        }
    }
    
    cJSON_Delete(result);
    return ESP_OK;
#else
    for (size_t i = 0; i < count; i++) {
        (void)signatures[i];
        confirmed[i] = true;
    }
    return ESP_OK;
#endif
}

/* ============================================================================
 * Airdrop
 * ========================================================================== */

esp_err_t espsol_rpc_request_airdrop(espsol_rpc_handle_t handle,
                                      const char *pubkey,
                                      uint64_t lamports,
                                      char *signature, size_t sig_len)
{
    if (!handle || !pubkey || !signature || sig_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    /* Build params: [pubkey, lamports, {commitment}] */
    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateString(pubkey));
    cJSON_AddItemToArray(params, cJSON_CreateNumber((double)lamports));
    
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "commitment", espsol_commitment_to_str(client->commitment));
    cJSON_AddItemToArray(params, config);
    
    char *request = build_jsonrpc_request(client, "requestAirdrop", params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    /* Result is the airdrop transaction signature */
    if (!cJSON_IsString(result)) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    size_t result_len = strlen(result->valuestring);
    if (result_len >= sig_len) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    strncpy(signature, result->valuestring, sig_len - 1);
    signature[sig_len - 1] = '\0';
    
    cJSON_Delete(result);
    return ESP_OK;
#else
    (void)pubkey;
    (void)lamports;
    strncpy(signature, "AirdropSignature123456789ABCDEF", sig_len - 1);
    signature[sig_len - 1] = '\0';
    return ESP_OK;
#endif
}

/* ============================================================================
 * Token Operations
 * ========================================================================== */

esp_err_t espsol_rpc_get_token_accounts_by_owner(espsol_rpc_handle_t handle,
                                                   const char *owner,
                                                   const char *mint,
                                                   espsol_token_account_t *accounts,
                                                   size_t *count)
{
    if (!handle || !owner || !accounts || !count || *count == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    size_t max_accounts = *count;
    *count = 0;
    
    /* Build params: [owner, {mint/programId}, {encoding}] */
    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateString(owner));
    
    /* Filter by mint or program */
    cJSON *filter = cJSON_CreateObject();
    if (mint) {
        cJSON_AddStringToObject(filter, "mint", mint);
    } else {
        /* Token Program ID */
        cJSON_AddStringToObject(filter, "programId", "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA");
    }
    cJSON_AddItemToArray(params, filter);
    
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "encoding", "jsonParsed");
    cJSON_AddStringToObject(config, "commitment", espsol_commitment_to_str(client->commitment));
    cJSON_AddItemToArray(params, config);
    
    char *request = build_jsonrpc_request(client, "getTokenAccountsByOwner", params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    /* Parse value array */
    cJSON *value = cJSON_GetObjectItem(result, "value");
    if (!value || !cJSON_IsArray(value)) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    size_t array_size = cJSON_GetArraySize(value);
    for (size_t i = 0; i < array_size && *count < max_accounts; i++) {
        cJSON *item = cJSON_GetArrayItem(value, i);
        if (!item) continue;
        
        cJSON *pubkey_json = cJSON_GetObjectItem(item, "pubkey");
        cJSON *account = cJSON_GetObjectItem(item, "account");
        if (!pubkey_json || !account) continue;
        
        cJSON *data = cJSON_GetObjectItem(account, "data");
        if (!data) continue;
        
        cJSON *parsed = cJSON_GetObjectItem(data, "parsed");
        if (!parsed) continue;
        
        cJSON *info = cJSON_GetObjectItem(parsed, "info");
        if (!info) continue;
        
        espsol_token_account_t *ta = &accounts[*count];
        memset(ta, 0, sizeof(espsol_token_account_t));
        
        /* Token account address */
        if (cJSON_IsString(pubkey_json)) {
            strncpy(ta->address, pubkey_json->valuestring, sizeof(ta->address) - 1);
        }
        
        /* Mint */
        cJSON *mint_json = cJSON_GetObjectItem(info, "mint");
        if (mint_json && cJSON_IsString(mint_json)) {
            strncpy(ta->mint, mint_json->valuestring, sizeof(ta->mint) - 1);
        }
        
        /* Owner */
        cJSON *owner_json = cJSON_GetObjectItem(info, "owner");
        if (owner_json && cJSON_IsString(owner_json)) {
            strncpy(ta->owner, owner_json->valuestring, sizeof(ta->owner) - 1);
        }
        
        /* Token amount */
        cJSON *token_amount = cJSON_GetObjectItem(info, "tokenAmount");
        if (token_amount) {
            cJSON *amount_json = cJSON_GetObjectItem(token_amount, "amount");
            cJSON *decimals_json = cJSON_GetObjectItem(token_amount, "decimals");
            
            if (amount_json && cJSON_IsString(amount_json)) {
                ta->amount = strtoull(amount_json->valuestring, NULL, 10);
            }
            if (decimals_json && cJSON_IsNumber(decimals_json)) {
                ta->decimals = (uint8_t)decimals_json->valueint;
            }
        }
        
        (*count)++;
    }
    
    cJSON_Delete(result);
    
    if (array_size > max_accounts) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    return ESP_OK;
#else
    (void)owner;
    (void)mint;
    *count = 0;
    return ESP_OK;
#endif
}

esp_err_t espsol_rpc_get_token_balance(espsol_rpc_handle_t handle,
                                        const char *token_account,
                                        uint64_t *amount,
                                        uint8_t *decimals)
{
    if (!handle || !token_account || !amount) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    /* Build params: [token_account, {commitment}] */
    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateString(token_account));
    
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "commitment", espsol_commitment_to_str(client->commitment));
    cJSON_AddItemToArray(params, config);
    
    char *request = build_jsonrpc_request(client, "getTokenAccountBalance", params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    /* Parse value */
    cJSON *value = cJSON_GetObjectItem(result, "value");
    if (!value || !cJSON_IsObject(value)) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    cJSON *amount_json = cJSON_GetObjectItem(value, "amount");
    if (amount_json && cJSON_IsString(amount_json)) {
        *amount = strtoull(amount_json->valuestring, NULL, 10);
    }
    
    if (decimals) {
        cJSON *decimals_json = cJSON_GetObjectItem(value, "decimals");
        if (decimals_json && cJSON_IsNumber(decimals_json)) {
            *decimals = (uint8_t)decimals_json->valueint;
        }
    }
    
    cJSON_Delete(result);
    return ESP_OK;
#else
    (void)token_account;
    *amount = 1000000;
    if (decimals) {
        *decimals = 6;
    }
    return ESP_OK;
#endif
}

/* ============================================================================
 * Generic RPC Call
 * ========================================================================== */

esp_err_t espsol_rpc_call(espsol_rpc_handle_t handle,
                           const char *method,
                           const char *params_json,
                           char *response, size_t response_len)
{
    if (!handle || !method || !response || response_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    /* Parse params if provided */
    cJSON *params = NULL;
    if (params_json && params_json[0] != '\0') {
        params = cJSON_Parse(params_json);
        if (!params) {
            return ESP_ERR_INVALID_ARG;
        }
    }
    
    char *request = build_jsonrpc_request(client, method, params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    /* Convert result to string */
    char *result_str = cJSON_PrintUnformatted(result);
    cJSON_Delete(result);
    
    if (!result_str) {
        return ESP_ERR_NO_MEM;
    }
    
    size_t result_len = strlen(result_str);
    if (result_len >= response_len) {
        free(result_str);
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    strncpy(response, result_str, response_len - 1);
    response[response_len - 1] = '\0';
    free(result_str);
    
    return ESP_OK;
#else
    (void)method;
    (void)params_json;
    strncpy(response, "{}", response_len - 1);
    response[response_len - 1] = '\0';
    return ESP_OK;
#endif
}

/* ============================================================================
 * Utility Functions
 * ========================================================================== */

esp_err_t espsol_rpc_get_minimum_balance_for_rent_exemption(
    espsol_rpc_handle_t handle,
    size_t data_len,
    uint64_t *lamports)
{
    if (!handle || !lamports) {
        return ESP_ERR_INVALID_ARG;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    struct espsol_rpc_client *client = handle;
    
    /* Build params: [data_len, {commitment}] */
    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateNumber((double)data_len));
    
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "commitment", espsol_commitment_to_str(client->commitment));
    cJSON_AddItemToArray(params, config);
    
    char *request = build_jsonrpc_request(client, "getMinimumBalanceForRentExemption", params);
    if (!request) {
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *result = NULL;
    esp_err_t err = execute_rpc_request(client, request, &result);
    free(request);
    
    if (err != ESP_OK) {
        return err;
    }
    
    if (!cJSON_IsNumber(result)) {
        cJSON_Delete(result);
        return ESP_ERR_ESPSOL_RPC_PARSE_ERROR;
    }
    
    *lamports = (uint64_t)result->valuedouble;
    cJSON_Delete(result);
    return ESP_OK;
#else
    /* Mock calculation based on data length */
    *lamports = 890880 + (data_len * 6960 / 1000);
    return ESP_OK;
#endif
}
