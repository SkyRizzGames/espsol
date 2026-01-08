/**
 * @file espsol_ws.c
 * @brief ESPSOL WebSocket Client Implementation
 *
 * WebSocket client for real-time Solana subscriptions using ESP-IDF's
 * esp_websocket_client API.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include "espsol_ws.h"
#include "espsol_internal.h"

#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_websocket_client.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "espsol_ws";

/* ============================================================================
 * WebSocket Client Structure
 * ========================================================================== */

#define ESPSOL_WS_MAX_SUBSCRIPTIONS 16

typedef struct subscription_entry {
    int id;                          /**< Subscription ID from server */
    espsol_ws_sub_type_t type;      /**< Subscription type */
    bool active;                     /**< Is subscription active */
} subscription_entry_t;

struct espsol_ws_client {
    esp_websocket_client_handle_t ws_handle;    /**< ESP-IDF WebSocket handle */
    espsol_ws_config_t config;                   /**< Client configuration */
    bool connected;                              /**< Connection status */
    SemaphoreHandle_t mutex;                     /**< Thread-safety mutex */
    subscription_entry_t subscriptions[ESPSOL_WS_MAX_SUBSCRIPTIONS]; /**< Active subscriptions */
    int next_request_id;                         /**< JSON-RPC request ID counter */
};

/* ============================================================================
 * Forward Declarations
 * ========================================================================== */

static void websocket_event_handler(void *handler_args, esp_event_base_t base,
                                    int32_t event_id, void *event_data);
static esp_err_t send_jsonrpc_request(espsol_ws_handle_t handle,
                                      const char *method,
                                      cJSON *params,
                                      int *request_id);
static void process_notification(espsol_ws_handle_t handle, cJSON *json);
static void process_response(espsol_ws_handle_t handle, cJSON *json);

/* ============================================================================
 * Helper Functions
 * ========================================================================== */

/**
 * @brief Convert HTTP/HTTPS RPC endpoint to WebSocket endpoint
 */
static char *http_to_ws_endpoint(const char *http_endpoint)
{
    if (http_endpoint == NULL) {
        return NULL;
    }

    size_t len = strlen(http_endpoint);
    char *ws_endpoint = NULL;

    if (strncmp(http_endpoint, "https://", 8) == 0) {
        ws_endpoint = malloc(len + 2); // "wss://" is 6 chars, https:// is 8, so -2
        if (ws_endpoint) {
            snprintf(ws_endpoint, len + 2, "wss://%s", http_endpoint + 8);
        }
    } else if (strncmp(http_endpoint, "http://", 7) == 0) {
        ws_endpoint = malloc(len + 1); // "ws://" is 5 chars, http:// is 7, so -2
        if (ws_endpoint) {
            snprintf(ws_endpoint, len + 1, "ws://%s", http_endpoint + 7);
        }
    } else {
        // Already a WebSocket endpoint, just duplicate
        ws_endpoint = strdup(http_endpoint);
    }

    return ws_endpoint;
}

/**
 * @brief Find subscription by ID
 */
static subscription_entry_t *find_subscription(espsol_ws_handle_t handle, int id)
{
    if (!handle) return NULL;

    for (int i = 0; i < ESPSOL_WS_MAX_SUBSCRIPTIONS; i++) {
        if (handle->subscriptions[i].active && handle->subscriptions[i].id == id) {
            return &handle->subscriptions[i];
        }
    }
    return NULL;
}

/**
 * @brief Add new subscription
 */
static esp_err_t add_subscription(espsol_ws_handle_t handle, int id, espsol_ws_sub_type_t type)
{
    if (!handle) return ESP_ERR_INVALID_ARG;

    for (int i = 0; i < ESPSOL_WS_MAX_SUBSCRIPTIONS; i++) {
        if (!handle->subscriptions[i].active) {
            handle->subscriptions[i].id = id;
            handle->subscriptions[i].type = type;
            handle->subscriptions[i].active = true;
            return ESP_OK;
        }
    }

    ESP_LOGE(TAG, "Maximum subscriptions reached (%d)", ESPSOL_WS_MAX_SUBSCRIPTIONS);
    return ESP_ERR_NO_MEM;
}

/**
 * @brief Remove subscription
 */
static void remove_subscription(espsol_ws_handle_t handle, int id)
{
    if (!handle) return;

    subscription_entry_t *sub = find_subscription(handle, id);
    if (sub) {
        sub->active = false;
    }
}

/* ============================================================================
 * WebSocket Event Handler
 * ========================================================================== */

static void websocket_event_handler(void *handler_args, esp_event_base_t base,
                                    int32_t event_id, void *event_data)
{
    espsol_ws_handle_t handle = (espsol_ws_handle_t)handler_args;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WebSocket connected");
        handle->connected = true;
        
        if (handle->config.event_callback) {
            espsol_ws_event_t event = {
                .type = ESPSOL_WS_EVENT_CONNECTED,
                .user_data = handle->config.user_data
            };
            handle->config.event_callback(&event, handle->config.user_data);
        }
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WebSocket disconnected");
        handle->connected = false;
        
        if (handle->config.event_callback) {
            espsol_ws_event_t event = {
                .type = ESPSOL_WS_EVENT_DISCONNECTED,
                .user_data = handle->config.user_data
            };
            handle->config.event_callback(&event, handle->config.user_data);
        }
        break;

    case WEBSOCKET_EVENT_DATA:
        if (data->data_ptr && data->data_len > 0) {
            // Parse JSON-RPC message
            cJSON *json = cJSON_ParseWithLength(data->data_ptr, data->data_len);
            if (json) {
                cJSON *method = cJSON_GetObjectItem(json, "method");
                if (method && cJSON_IsString(method)) {
                    // This is a notification
                    process_notification(handle, json);
                } else {
                    // This is a response to our request
                    process_response(handle, json);
                }
                cJSON_Delete(json);
            } else {
                ESP_LOGE(TAG, "Failed to parse WebSocket message");
            }
        }
        break;

    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "WebSocket error");
        
        if (handle->config.event_callback) {
            espsol_ws_event_t event = {
                .type = ESPSOL_WS_EVENT_ERROR,
                .user_data = handle->config.user_data
            };
            handle->config.event_callback(&event, handle->config.user_data);
        }
        break;

    default:
        break;
    }
}

/* ============================================================================
 * JSON-RPC Message Processing
 * ========================================================================== */

static esp_err_t send_jsonrpc_request(espsol_ws_handle_t handle,
                                      const char *method,
                                      cJSON *params,
                                      int *request_id)
{
    if (!handle || !method) {
        return ESP_ERR_INVALID_ARG;
    }

    // Get next request ID
    int id = handle->next_request_id++;

    // Build JSON-RPC 2.0 request
    cJSON *request = cJSON_CreateObject();
    if (!request) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(request, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(request, "id", id);
    cJSON_AddStringToObject(request, "method", method);
    
    if (params) {
        cJSON_AddItemToObject(request, "params", params);
    } else {
        cJSON_AddArrayToObject(request, "params");
    }

    // Serialize to string
    char *json_str = cJSON_PrintUnformatted(request);
    cJSON_Delete(request);

    if (!json_str) {
        return ESP_ERR_NO_MEM;
    }

    // Send via WebSocket
    int sent = esp_websocket_client_send_text(handle->ws_handle, json_str, 
                                              strlen(json_str), 
                                              pdMS_TO_TICKS(handle->config.timeout_ms));
    
    free(json_str);

    if (sent < 0) {
        ESP_LOGE(TAG, "Failed to send WebSocket message");
        return ESP_ERR_ESPSOL_NETWORK_ERROR;
    }

    if (request_id) {
        *request_id = id;
    }

    return ESP_OK;
}

static void process_notification(espsol_ws_handle_t handle, cJSON *json)
{
    if (!handle || !json || !handle->config.event_callback) {
        return;
    }

    cJSON *method = cJSON_GetObjectItem(json, "method");
    cJSON *params = cJSON_GetObjectItem(json, "params");
    
    if (!method || !cJSON_IsString(method) || !params) {
        return;
    }

    const char *method_name = method->valuestring;
    
    // Extract subscription ID
    cJSON *result = cJSON_GetObjectItem(params, "result");
    cJSON *subscription = cJSON_GetObjectItem(params, "subscription");
    
    if (!subscription || !cJSON_IsNumber(subscription)) {
        return;
    }

    int sub_id = (int)subscription->valuedouble;
    subscription_entry_t *sub = find_subscription(handle, sub_id);
    
    if (!sub) {
        ESP_LOGW(TAG, "Received notification for unknown subscription: %d", sub_id);
        return;
    }

    // Create event
    espsol_ws_event_t event = {
        .type = ESPSOL_WS_EVENT_DATA,
        .sub_type = sub->type,
        .subscription_id = sub_id,
        .user_data = handle->config.user_data
    };

    // Parse notification data based on type
    if (strcmp(method_name, "accountNotification") == 0) {
        cJSON *value = cJSON_GetObjectItem(result, "value");
        if (value) {
            cJSON *lamports = cJSON_GetObjectItem(value, "lamports");
            cJSON *owner = cJSON_GetObjectItem(value, "owner");
            cJSON *executable = cJSON_GetObjectItem(value, "executable");
            cJSON *rent_epoch = cJSON_GetObjectItem(value, "rentEpoch");
            
            if (lamports) event.data.account.lamports = (uint64_t)lamports->valuedouble;
            if (owner && cJSON_IsString(owner)) {
                strncpy(event.data.account.owner, owner->valuestring, 
                       ESPSOL_ADDRESS_MAX_LEN - 1);
                event.data.account.owner[ESPSOL_ADDRESS_MAX_LEN - 1] = '\0';
            }
            if (executable) event.data.account.executable = cJSON_IsTrue(executable);
            if (rent_epoch) event.data.account.rent_epoch = (uint64_t)rent_epoch->valuedouble;
            
            // Note: Account data parsing would require Base64 decoding
            event.data.account.data = NULL;
            event.data.account.data_len = 0;
        }
    } 
    else if (strcmp(method_name, "signatureNotification") == 0) {
        cJSON *err = cJSON_GetObjectItem(result, "err");
        if (err && cJSON_IsString(err)) {
            strncpy(event.data.signature.err, err->valuestring, 127);
            event.data.signature.err[127] = '\0';
        } else {
            event.data.signature.err[0] = '\0';
        }
    }
    else if (strcmp(method_name, "logsNotification") == 0) {
        cJSON *value = cJSON_GetObjectItem(result, "value");
        if (value) {
            cJSON *signature = cJSON_GetObjectItem(value, "signature");
            cJSON *err = cJSON_GetObjectItem(value, "err");
            
            if (signature && cJSON_IsString(signature)) {
                strncpy(event.data.logs.signature, signature->valuestring,
                       ESPSOL_SIGNATURE_MAX_LEN - 1);
                event.data.logs.signature[ESPSOL_SIGNATURE_MAX_LEN - 1] = '\0';
            }
            
            if (err && cJSON_IsString(err)) {
                strncpy(event.data.logs.err, err->valuestring, 127);
                event.data.logs.err[127] = '\0';
            } else {
                event.data.logs.err[0] = '\0';
            }
            
            // Note: Full logs parsing would allocate array
            event.data.logs.logs = NULL;
            event.data.logs.log_count = 0;
        }
    }
    else if (strcmp(method_name, "slotNotification") == 0) {
        cJSON *slot = cJSON_GetObjectItem(result, "slot");
        cJSON *parent = cJSON_GetObjectItem(result, "parent");
        cJSON *root = cJSON_GetObjectItem(result, "root");
        
        if (slot) event.data.slot.slot = (uint64_t)slot->valuedouble;
        if (parent) event.data.slot.parent = (uint64_t)parent->valuedouble;
        if (root) event.data.slot.root = (uint64_t)root->valuedouble;
    }

    // Invoke callback
    handle->config.event_callback(&event, handle->config.user_data);
}

static void process_response(espsol_ws_handle_t handle, cJSON *json)
{
    if (!handle || !json) {
        return;
    }

    cJSON *id = cJSON_GetObjectItem(json, "id");
    cJSON *result = cJSON_GetObjectItem(json, "result");
    cJSON *error = cJSON_GetObjectItem(json, "error");
    
    (void)id; // May be used in future for request tracking

    if (error) {
        cJSON *message = cJSON_GetObjectItem(error, "message");
        const char *err_msg = (message && cJSON_IsString(message)) ? 
                              message->valuestring : "Unknown error";
        ESP_LOGE(TAG, "JSON-RPC error: %s", err_msg);
        return;
    }

    if (result && cJSON_IsNumber(result)) {
        int sub_id = (int)result->valuedouble;
        ESP_LOGI(TAG, "Subscription confirmed, ID: %d", sub_id);
    }
}

/* ============================================================================
 * Public API Implementation
 * ========================================================================== */

esp_err_t espsol_ws_init(espsol_ws_handle_t *handle, 
                         espsol_ws_event_cb_t event_callback,
                         void *user_data)
{
    if (!handle || !event_callback) {
        return ESP_ERR_INVALID_ARG;
    }

    // Get default endpoint from Kconfig
    const char *http_endpoint = CONFIG_ESPSOL_DEFAULT_RPC_ENDPOINT;
    char *ws_endpoint = http_to_ws_endpoint(http_endpoint);
    
    if (!ws_endpoint) {
        return ESP_ERR_NO_MEM;
    }

    espsol_ws_config_t config = {
        .endpoint = ws_endpoint,
        .timeout_ms = CONFIG_ESPSOL_RPC_TIMEOUT_MS,
        .commitment = ESPSOL_COMMITMENT_CONFIRMED,
        .buffer_size = CONFIG_ESPSOL_RPC_BUFFER_SIZE,
        .event_callback = event_callback,
        .user_data = user_data,
        .auto_reconnect = true,
        .reconnect_delay_ms = 5000
    };

    esp_err_t ret = espsol_ws_init_with_config(handle, &config);
    free(ws_endpoint);
    
    return ret;
}

esp_err_t espsol_ws_init_with_config(espsol_ws_handle_t *handle, 
                                     const espsol_ws_config_t *config)
{
    if (!handle || !config || !config->endpoint || !config->event_callback) {
        return ESP_ERR_INVALID_ARG;
    }

    // Allocate client structure
    espsol_ws_handle_t client = calloc(1, sizeof(struct espsol_ws_client));
    if (!client) {
        return ESP_ERR_NO_MEM;
    }

    // Copy configuration
    client->config = *config;
    client->config.endpoint = strdup(config->endpoint);
    if (!client->config.endpoint) {
        free(client);
        return ESP_ERR_NO_MEM;
    }

    // Create mutex
    client->mutex = xSemaphoreCreateMutex();
    if (!client->mutex) {
        free((void *)client->config.endpoint);
        free(client);
        return ESP_ERR_NO_MEM;
    }

    // Initialize WebSocket client
    esp_websocket_client_config_t ws_config = {
        .uri = client->config.endpoint,
        .buffer_size = (int)client->config.buffer_size,
        .reconnect_timeout_ms = (int)client->config.reconnect_delay_ms,
        .network_timeout_ms = (int)client->config.timeout_ms,
    };

    client->ws_handle = esp_websocket_client_init(&ws_config);
    if (!client->ws_handle) {
        vSemaphoreDelete(client->mutex);
        free((void *)client->config.endpoint);
        free(client);
        return ESP_ERR_ESPSOL_NETWORK_ERROR;
    }

    // Register event handler
    esp_err_t ret = esp_websocket_register_events(client->ws_handle,
                                                   WEBSOCKET_EVENT_ANY,
                                                   websocket_event_handler,
                                                   client);
    if (ret != ESP_OK) {
        esp_websocket_client_destroy(client->ws_handle);
        vSemaphoreDelete(client->mutex);
        free((void *)client->config.endpoint);
        free(client);
        return ret;
    }

    // Start WebSocket client
    ret = esp_websocket_client_start(client->ws_handle);
    if (ret != ESP_OK) {
        esp_websocket_client_destroy(client->ws_handle);
        vSemaphoreDelete(client->mutex);
        free((void *)client->config.endpoint);
        free(client);
        return ESP_ERR_ESPSOL_NETWORK_ERROR;
    }

    client->next_request_id = 1;
    *handle = client;

    ESP_LOGI(TAG, "WebSocket client initialized: %s", client->config.endpoint);
    return ESP_OK;
}

esp_err_t espsol_ws_cleanup(espsol_ws_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    // Stop WebSocket client
    if (handle->ws_handle) {
        esp_websocket_client_stop(handle->ws_handle);
        esp_websocket_client_destroy(handle->ws_handle);
    }

    // Cleanup
    if (handle->mutex) {
        vSemaphoreDelete(handle->mutex);
    }
    
    if (handle->config.endpoint) {
        free((void *)handle->config.endpoint);
    }

    free(handle);
    
    ESP_LOGI(TAG, "WebSocket client cleaned up");
    return ESP_OK;
}

esp_err_t espsol_ws_is_connected(espsol_ws_handle_t handle, bool *connected)
{
    if (!handle || !connected) {
        return ESP_ERR_INVALID_ARG;
    }

    *connected = handle->connected;
    return ESP_OK;
}

/* ============================================================================
 * Subscription API Implementation
 * ========================================================================== */

esp_err_t espsol_ws_account_subscribe(espsol_ws_handle_t handle,
                                      const char *account_address,
                                      int *subscription_id)
{
    if (!handle || !account_address) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    // Build parameters
    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateString(account_address));
    
    // Add config object
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "encoding", "base64");
    cJSON_AddStringToObject(config, "commitment", 
                           espsol_commitment_to_string(handle->config.commitment));
    cJSON_AddItemToArray(params, config);

    int request_id;
    esp_err_t ret = send_jsonrpc_request(handle, "accountSubscribe", params, &request_id);
    
    if (ret == ESP_OK && subscription_id) {
        *subscription_id = request_id; // Temporary, will be updated on response
        add_subscription(handle, request_id, ESPSOL_WS_SUB_ACCOUNT);
    }

    xSemaphoreGive(handle->mutex);
    return ret;
}

esp_err_t espsol_ws_account_unsubscribe(espsol_ws_handle_t handle,
                                        int subscription_id)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateNumber(subscription_id));

    esp_err_t ret = send_jsonrpc_request(handle, "accountUnsubscribe", params, NULL);
    
    if (ret == ESP_OK) {
        remove_subscription(handle, subscription_id);
    }

    xSemaphoreGive(handle->mutex);
    return ret;
}

esp_err_t espsol_ws_program_subscribe(espsol_ws_handle_t handle,
                                      const char *program_id,
                                      int *subscription_id)
{
    if (!handle || !program_id) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateString(program_id));
    
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "encoding", "base64");
    cJSON_AddStringToObject(config, "commitment",
                           espsol_commitment_to_string(handle->config.commitment));
    cJSON_AddItemToArray(params, config);

    int request_id;
    esp_err_t ret = send_jsonrpc_request(handle, "programSubscribe", params, &request_id);
    
    if (ret == ESP_OK && subscription_id) {
        *subscription_id = request_id;
        add_subscription(handle, request_id, ESPSOL_WS_SUB_PROGRAM);
    }

    xSemaphoreGive(handle->mutex);
    return ret;
}

esp_err_t espsol_ws_program_unsubscribe(espsol_ws_handle_t handle,
                                        int subscription_id)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateNumber(subscription_id));

    esp_err_t ret = send_jsonrpc_request(handle, "programUnsubscribe", params, NULL);
    
    if (ret == ESP_OK) {
        remove_subscription(handle, subscription_id);
    }

    xSemaphoreGive(handle->mutex);
    return ret;
}

esp_err_t espsol_ws_signature_subscribe(espsol_ws_handle_t handle,
                                        const char *signature,
                                        int *subscription_id)
{
    if (!handle || !signature) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateString(signature));
    
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "commitment",
                           espsol_commitment_to_string(handle->config.commitment));
    cJSON_AddItemToArray(params, config);

    int request_id;
    esp_err_t ret = send_jsonrpc_request(handle, "signatureSubscribe", params, &request_id);
    
    if (ret == ESP_OK && subscription_id) {
        *subscription_id = request_id;
        add_subscription(handle, request_id, ESPSOL_WS_SUB_SIGNATURE);
    }

    xSemaphoreGive(handle->mutex);
    return ret;
}

esp_err_t espsol_ws_signature_unsubscribe(espsol_ws_handle_t handle,
                                          int subscription_id)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateNumber(subscription_id));

    esp_err_t ret = send_jsonrpc_request(handle, "signatureUnsubscribe", params, NULL);
    
    if (ret == ESP_OK) {
        remove_subscription(handle, subscription_id);
    }

    xSemaphoreGive(handle->mutex);
    return ret;
}

esp_err_t espsol_ws_logs_subscribe(espsol_ws_handle_t handle,
                                   const char *mentions,
                                   int *subscription_id)
{
    if (!handle || !mentions) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    cJSON *params = cJSON_CreateArray();
    
    // "all" or "allWithVotes" for all transactions, or specific address
    if (strcmp(mentions, "all") == 0 || strcmp(mentions, "allWithVotes") == 0) {
        cJSON_AddItemToArray(params, cJSON_CreateString(mentions));
    } else {
        cJSON *filter = cJSON_CreateObject();
        cJSON_AddStringToObject(filter, "mentions", mentions);
        cJSON_AddItemToArray(params, filter);
    }
    
    cJSON *config = cJSON_CreateObject();
    cJSON_AddStringToObject(config, "commitment",
                           espsol_commitment_to_string(handle->config.commitment));
    cJSON_AddItemToArray(params, config);

    int request_id;
    esp_err_t ret = send_jsonrpc_request(handle, "logsSubscribe", params, &request_id);
    
    if (ret == ESP_OK && subscription_id) {
        *subscription_id = request_id;
        add_subscription(handle, request_id, ESPSOL_WS_SUB_LOGS);
    }

    xSemaphoreGive(handle->mutex);
    return ret;
}

esp_err_t espsol_ws_logs_unsubscribe(espsol_ws_handle_t handle,
                                     int subscription_id)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateNumber(subscription_id));

    esp_err_t ret = send_jsonrpc_request(handle, "logsUnsubscribe", params, NULL);
    
    if (ret == ESP_OK) {
        remove_subscription(handle, subscription_id);
    }

    xSemaphoreGive(handle->mutex);
    return ret;
}

esp_err_t espsol_ws_slot_subscribe(espsol_ws_handle_t handle,
                                   int *subscription_id)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    cJSON *params = cJSON_CreateArray();

    int request_id;
    esp_err_t ret = send_jsonrpc_request(handle, "slotSubscribe", params, &request_id);
    
    if (ret == ESP_OK && subscription_id) {
        *subscription_id = request_id;
        add_subscription(handle, request_id, ESPSOL_WS_SUB_SLOT);
    }

    xSemaphoreGive(handle->mutex);
    return ret;
}

esp_err_t espsol_ws_slot_unsubscribe(espsol_ws_handle_t handle,
                                     int subscription_id)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateNumber(subscription_id));

    esp_err_t ret = send_jsonrpc_request(handle, "slotUnsubscribe", params, NULL);
    
    if (ret == ESP_OK) {
        remove_subscription(handle, subscription_id);
    }

    xSemaphoreGive(handle->mutex);
    return ret;
}
