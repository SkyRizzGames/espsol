/**
 * @file espsol_ws.h
 * @brief ESPSOL WebSocket Client API
 *
 * This file provides the WebSocket client interface for real-time subscriptions
 * to Solana RPC nodes. Uses native esp_websocket_client for WebSocket connections
 * and cJSON for JSON-RPC 2.0 message handling.
 *
 * Features:
 * - Account subscriptions (accountSubscribe)
 * - Program account subscriptions (programSubscribe)
 * - Signature subscriptions (signatureSubscribe)
 * - Logs subscriptions (logsSubscribe)
 * - Slot notifications (slotSubscribe)
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#ifndef ESPSOL_WS_H
#define ESPSOL_WS_H

#include "espsol_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * WebSocket Client Handle
 * ========================================================================== */

/**
 * @brief Opaque handle for WebSocket client connection
 */
typedef struct espsol_ws_client *espsol_ws_handle_t;

/* ============================================================================
 * Subscription Types
 * ========================================================================== */

/**
 * @brief WebSocket subscription types
 */
typedef enum {
    ESPSOL_WS_SUB_ACCOUNT,       /**< Account state changes */
    ESPSOL_WS_SUB_PROGRAM,       /**< Program account changes */
    ESPSOL_WS_SUB_SIGNATURE,     /**< Transaction signature confirmations */
    ESPSOL_WS_SUB_LOGS,          /**< Transaction logs */
    ESPSOL_WS_SUB_SLOT,          /**< Slot notifications */
} espsol_ws_sub_type_t;

/**
 * @brief WebSocket event types
 */
typedef enum {
    ESPSOL_WS_EVENT_CONNECTED,      /**< WebSocket connected */
    ESPSOL_WS_EVENT_DISCONNECTED,   /**< WebSocket disconnected */
    ESPSOL_WS_EVENT_ERROR,          /**< WebSocket error */
    ESPSOL_WS_EVENT_DATA,           /**< Subscription data received */
} espsol_ws_event_type_t;

/* ============================================================================
 * Notification Structures
 * ========================================================================== */

/**
 * @brief Account notification data
 */
typedef struct {
    uint64_t slot;                             /**< Slot number */
    char owner[ESPSOL_ADDRESS_MAX_LEN];        /**< Account owner program */
    uint64_t lamports;                         /**< Account balance */
    uint8_t *data;                             /**< Account data */
    size_t data_len;                           /**< Data length */
    bool executable;                           /**< Is executable */
    uint64_t rent_epoch;                       /**< Rent epoch */
} espsol_ws_account_notification_t;

/**
 * @brief Signature notification data
 */
typedef struct {
    uint64_t slot;                             /**< Slot number */
    char signature[ESPSOL_SIGNATURE_MAX_LEN];  /**< Transaction signature */
    char err[128];                             /**< Error message if failed */
} espsol_ws_signature_notification_t;

/**
 * @brief Logs notification data
 */
typedef struct {
    uint64_t slot;                             /**< Slot number */
    char signature[ESPSOL_SIGNATURE_MAX_LEN];  /**< Transaction signature */
    char err[128];                             /**< Error message if failed */
    char **logs;                               /**< Array of log lines */
    size_t log_count;                          /**< Number of log lines */
} espsol_ws_logs_notification_t;

/**
 * @brief Slot notification data
 */
typedef struct {
    uint64_t slot;      /**< Current slot */
    uint64_t parent;    /**< Parent slot */
    uint64_t root;      /**< Root slot */
} espsol_ws_slot_notification_t;

/**
 * @brief WebSocket event data union
 */
typedef union {
    espsol_ws_account_notification_t account;       /**< Account notification */
    espsol_ws_signature_notification_t signature;   /**< Signature notification */
    espsol_ws_logs_notification_t logs;             /**< Logs notification */
    espsol_ws_slot_notification_t slot;             /**< Slot notification */
} espsol_ws_notification_data_t;

/**
 * @brief WebSocket event
 */
typedef struct {
    espsol_ws_event_type_t type;             /**< Event type */
    espsol_ws_sub_type_t sub_type;           /**< Subscription type (for data events) */
    int subscription_id;                     /**< Subscription ID */
    espsol_ws_notification_data_t data;      /**< Notification data */
    void *user_data;                         /**< User-provided context */
} espsol_ws_event_t;

/* ============================================================================
 * Callback Function Types
 * ========================================================================== */

/**
 * @brief WebSocket event callback function
 *
 * @param event Pointer to event data
 * @param user_data User-provided context pointer
 */
typedef void (*espsol_ws_event_cb_t)(const espsol_ws_event_t *event, void *user_data);

/* ============================================================================
 * Configuration Structure
 * ========================================================================== */

/**
 * @brief WebSocket client configuration
 */
typedef struct {
    const char *endpoint;                /**< WebSocket RPC endpoint (ws:// or wss://) */
    uint32_t timeout_ms;                 /**< Connection timeout in milliseconds */
    espsol_commitment_t commitment;      /**< Default commitment level */
    size_t buffer_size;                  /**< WebSocket buffer size */
    espsol_ws_event_cb_t event_callback; /**< Event callback function */
    void *user_data;                     /**< User context for callbacks */
    bool auto_reconnect;                 /**< Auto-reconnect on disconnect */
    uint32_t reconnect_delay_ms;         /**< Delay before reconnect attempt */
} espsol_ws_config_t;

/* ============================================================================
 * Client Management Functions
 * ========================================================================== */

/**
 * @brief Initialize WebSocket client with default configuration
 *
 * Creates a WebSocket client using default settings from Kconfig.
 * The endpoint will be derived from the HTTP RPC endpoint by replacing
 * http:// with ws:// and https:// with wss://.
 *
 * @param[out] handle Pointer to store the client handle
 * @param[in] event_callback Event callback function
 * @param[in] user_data User context pointer
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle or event_callback is NULL
 *   - ESP_ERR_NO_MEM: Failed to allocate client
 *   - ESP_ERR_ESPSOL_NETWORK_ERROR: Failed to connect
 */
esp_err_t espsol_ws_init(espsol_ws_handle_t *handle, 
                         espsol_ws_event_cb_t event_callback,
                         void *user_data);

/**
 * @brief Initialize WebSocket client with custom configuration
 *
 * @param[out] handle Pointer to store the client handle
 * @param[in] config Client configuration
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle or config is NULL
 *   - ESP_ERR_NO_MEM: Failed to allocate client
 *   - ESP_ERR_ESPSOL_NETWORK_ERROR: Failed to connect
 */
esp_err_t espsol_ws_init_with_config(espsol_ws_handle_t *handle, 
                                     const espsol_ws_config_t *config);

/**
 * @brief Close WebSocket connection and cleanup resources
 *
 * Unsubscribes from all active subscriptions and closes the connection.
 *
 * @param[in] handle WebSocket client handle
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle is NULL
 */
esp_err_t espsol_ws_cleanup(espsol_ws_handle_t handle);

/**
 * @brief Check if WebSocket is connected
 *
 * @param[in] handle WebSocket client handle
 * @param[out] connected Pointer to store connection status
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle or connected is NULL
 */
esp_err_t espsol_ws_is_connected(espsol_ws_handle_t handle, bool *connected);

/* ============================================================================
 * Subscription Functions
 * ========================================================================== */

/**
 * @brief Subscribe to account changes
 *
 * Receives notifications when the account data or lamports change.
 *
 * @param[in] handle WebSocket client handle
 * @param[in] account_address Base58-encoded account address
 * @param[out] subscription_id Pointer to store subscription ID
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle or account_address is NULL
 *   - ESP_ERR_ESPSOL_NETWORK_ERROR: Failed to send subscription
 *   - ESP_ERR_ESPSOL_RPC_PARSE_ERROR: Failed to parse response
 */
esp_err_t espsol_ws_account_subscribe(espsol_ws_handle_t handle,
                                      const char *account_address,
                                      int *subscription_id);

/**
 * @brief Unsubscribe from account changes
 *
 * @param[in] handle WebSocket client handle
 * @param[in] subscription_id Subscription ID to cancel
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle is NULL
 *   - ESP_ERR_ESPSOL_NETWORK_ERROR: Failed to send unsubscribe
 */
esp_err_t espsol_ws_account_unsubscribe(espsol_ws_handle_t handle,
                                        int subscription_id);

/**
 * @brief Subscribe to program account changes
 *
 * Receives notifications when any account owned by the specified program changes.
 *
 * @param[in] handle WebSocket client handle
 * @param[in] program_id Base58-encoded program ID
 * @param[out] subscription_id Pointer to store subscription ID
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle or program_id is NULL
 *   - ESP_ERR_ESPSOL_NETWORK_ERROR: Failed to send subscription
 *   - ESP_ERR_ESPSOL_RPC_PARSE_ERROR: Failed to parse response
 */
esp_err_t espsol_ws_program_subscribe(espsol_ws_handle_t handle,
                                      const char *program_id,
                                      int *subscription_id);

/**
 * @brief Unsubscribe from program account changes
 *
 * @param[in] handle WebSocket client handle
 * @param[in] subscription_id Subscription ID to cancel
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle is NULL
 *   - ESP_ERR_ESPSOL_NETWORK_ERROR: Failed to send unsubscribe
 */
esp_err_t espsol_ws_program_unsubscribe(espsol_ws_handle_t handle,
                                        int subscription_id);

/**
 * @brief Subscribe to transaction signature status
 *
 * Receives notification when transaction reaches the specified commitment level.
 *
 * @param[in] handle WebSocket client handle
 * @param[in] signature Base58-encoded transaction signature
 * @param[out] subscription_id Pointer to store subscription ID
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle or signature is NULL
 *   - ESP_ERR_ESPSOL_NETWORK_ERROR: Failed to send subscription
 *   - ESP_ERR_ESPSOL_RPC_PARSE_ERROR: Failed to parse response
 */
esp_err_t espsol_ws_signature_subscribe(espsol_ws_handle_t handle,
                                        const char *signature,
                                        int *subscription_id);

/**
 * @brief Unsubscribe from signature status
 *
 * @param[in] handle WebSocket client handle
 * @param[in] subscription_id Subscription ID to cancel
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle is NULL
 *   - ESP_ERR_ESPSOL_NETWORK_ERROR: Failed to send unsubscribe
 */
esp_err_t espsol_ws_signature_unsubscribe(espsol_ws_handle_t handle,
                                          int subscription_id);

/**
 * @brief Subscribe to transaction logs
 *
 * Receives all logs from transactions that mention the specified address.
 *
 * @param[in] handle WebSocket client handle
 * @param[in] mentions Base58-encoded address to filter logs ("all" for all transactions)
 * @param[out] subscription_id Pointer to store subscription ID
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle or mentions is NULL
 *   - ESP_ERR_ESPSOL_NETWORK_ERROR: Failed to send subscription
 *   - ESP_ERR_ESPSOL_RPC_PARSE_ERROR: Failed to parse response
 */
esp_err_t espsol_ws_logs_subscribe(espsol_ws_handle_t handle,
                                   const char *mentions,
                                   int *subscription_id);

/**
 * @brief Unsubscribe from logs
 *
 * @param[in] handle WebSocket client handle
 * @param[in] subscription_id Subscription ID to cancel
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle is NULL
 *   - ESP_ERR_ESPSOL_NETWORK_ERROR: Failed to send unsubscribe
 */
esp_err_t espsol_ws_logs_unsubscribe(espsol_ws_handle_t handle,
                                     int subscription_id);

/**
 * @brief Subscribe to slot notifications
 *
 * Receives notifications when a new slot is processed.
 *
 * @param[in] handle WebSocket client handle
 * @param[out] subscription_id Pointer to store subscription ID
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle is NULL
 *   - ESP_ERR_ESPSOL_NETWORK_ERROR: Failed to send subscription
 *   - ESP_ERR_ESPSOL_RPC_PARSE_ERROR: Failed to parse response
 */
esp_err_t espsol_ws_slot_subscribe(espsol_ws_handle_t handle,
                                   int *subscription_id);

/**
 * @brief Unsubscribe from slot notifications
 *
 * @param[in] handle WebSocket client handle
 * @param[in] subscription_id Subscription ID to cancel
 *
 * @return
 *   - ESP_OK: Success
 *   - ESP_ERR_INVALID_ARG: handle is NULL
 *   - ESP_ERR_ESPSOL_NETWORK_ERROR: Failed to send unsubscribe
 */
esp_err_t espsol_ws_slot_unsubscribe(espsol_ws_handle_t handle,
                                     int subscription_id);

#ifdef __cplusplus
}
#endif

#endif /* ESPSOL_WS_H */
