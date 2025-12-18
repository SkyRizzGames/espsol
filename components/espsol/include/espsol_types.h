/**
 * @file espsol_types.h
 * @brief ESPSOL Common Types and Constants
 *
 * This file contains all common type definitions, constants, and error codes
 * used throughout the ESPSOL Solana component.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#ifndef ESPSOL_TYPES_H
#define ESPSOL_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ESP-IDF compatibility layer for host testing */
#if defined(ESP_PLATFORM) && ESP_PLATFORM
#include "esp_err.h"
#else
/* Mock ESP-IDF types for host compilation */
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_NO_MEM 0x101
#define ESP_FAIL -1
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Version Information
 * ========================================================================== */

#define ESPSOL_VERSION_MAJOR    0
#define ESPSOL_VERSION_MINOR    1
#define ESPSOL_VERSION_PATCH    0
#define ESPSOL_VERSION_STRING   "0.1.0"

/* ============================================================================
 * Size Constants
 * ========================================================================== */

/** @brief Size of a Solana public key in bytes */
#define ESPSOL_PUBKEY_SIZE          32

/** @brief Size of a Solana private key in bytes (Ed25519 secret key) */
#define ESPSOL_PRIVKEY_SIZE         64

/** @brief Size of an Ed25519 signature in bytes */
#define ESPSOL_SIGNATURE_SIZE       64

/** @brief Size of a seed for keypair generation in bytes */
#define ESPSOL_SEED_SIZE            32

/** @brief Size of a blockhash in bytes */
#define ESPSOL_BLOCKHASH_SIZE       32

/** @brief Maximum length of a Base58-encoded Solana address (including null terminator) */
#define ESPSOL_ADDRESS_MAX_LEN      45

/** @brief Maximum length of a Base58-encoded signature (including null terminator) */
#define ESPSOL_SIGNATURE_MAX_LEN    90

/** @brief Maximum length of a Base58-encoded private key (including null terminator) */
#define ESPSOL_PRIVKEY_BASE58_LEN   128

/* ============================================================================
 * Transaction Limits
 * ========================================================================== */

/** @brief Maximum number of instructions per transaction */
#define ESPSOL_MAX_INSTRUCTIONS     10

/** @brief Maximum number of accounts per transaction */
#define ESPSOL_MAX_ACCOUNTS         20

/** @brief Maximum number of signers per transaction */
#define ESPSOL_MAX_SIGNERS          4

/** @brief Maximum serialized transaction size */
#define ESPSOL_MAX_TX_SIZE          1232

/** @brief Maximum instruction data size */
#define ESPSOL_MAX_INSTRUCTION_DATA 256

/* ============================================================================
 * Network Constants
 * ========================================================================== */

/** @brief Solana Mainnet RPC endpoint */
#define ESPSOL_MAINNET_RPC      "https://api.mainnet-beta.solana.com"

/** @brief Solana Devnet RPC endpoint */
#define ESPSOL_DEVNET_RPC       "https://api.devnet.solana.com"

/** @brief Solana Testnet RPC endpoint */
#define ESPSOL_TESTNET_RPC      "https://api.testnet.solana.com"

/** @brief Local test validator RPC endpoint */
#define ESPSOL_LOCALNET_RPC     "http://localhost:8899"

/** @brief Number of lamports per SOL */
#define ESPSOL_LAMPORTS_PER_SOL 1000000000ULL

/* ============================================================================
 * Default Configuration
 * ========================================================================== */

/** @brief Default RPC request timeout in milliseconds */
#define ESPSOL_DEFAULT_TIMEOUT_MS   30000

/** @brief Default HTTP buffer size */
#define ESPSOL_DEFAULT_BUFFER_SIZE  4096

/* ============================================================================
 * Error Codes
 * ========================================================================== */

/** @brief ESPSOL error base */
#define ESP_ERR_ESPSOL_BASE         0x50000

/** @brief Invalid argument provided */
#define ESP_ERR_ESPSOL_INVALID_ARG          (ESP_ERR_ESPSOL_BASE + 0x01)

/** @brief Buffer too small for operation */
#define ESP_ERR_ESPSOL_BUFFER_TOO_SMALL     (ESP_ERR_ESPSOL_BASE + 0x02)

/** @brief Encoding/decoding failed */
#define ESP_ERR_ESPSOL_ENCODING_FAILED      (ESP_ERR_ESPSOL_BASE + 0x03)

/** @brief Invalid Base58 input */
#define ESP_ERR_ESPSOL_INVALID_BASE58       (ESP_ERR_ESPSOL_BASE + 0x04)

/** @brief Invalid Base64 input */
#define ESP_ERR_ESPSOL_INVALID_BASE64       (ESP_ERR_ESPSOL_BASE + 0x05)

/** @brief Keypair not initialized */
#define ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT     (ESP_ERR_ESPSOL_BASE + 0x06)

/** @brief Signature verification failed */
#define ESP_ERR_ESPSOL_SIGNATURE_INVALID    (ESP_ERR_ESPSOL_BASE + 0x07)

/** @brief RPC request failed */
#define ESP_ERR_ESPSOL_RPC_FAILED           (ESP_ERR_ESPSOL_BASE + 0x08)

/** @brief RPC response parse error */
#define ESP_ERR_ESPSOL_RPC_PARSE_ERROR      (ESP_ERR_ESPSOL_BASE + 0x09)

/** @brief Transaction build error */
#define ESP_ERR_ESPSOL_TX_BUILD_ERROR       (ESP_ERR_ESPSOL_BASE + 0x0A)

/** @brief Transaction not signed */
#define ESP_ERR_ESPSOL_TX_NOT_SIGNED        (ESP_ERR_ESPSOL_BASE + 0x0B)

/** @brief Maximum accounts exceeded */
#define ESP_ERR_ESPSOL_MAX_ACCOUNTS         (ESP_ERR_ESPSOL_BASE + 0x0C)

/** @brief Maximum instructions exceeded */
#define ESP_ERR_ESPSOL_MAX_INSTRUCTIONS     (ESP_ERR_ESPSOL_BASE + 0x0D)

/** @brief NVS storage error */
#define ESP_ERR_ESPSOL_NVS_ERROR            (ESP_ERR_ESPSOL_BASE + 0x0E)

/** @brief Crypto operation failed */
#define ESP_ERR_ESPSOL_CRYPTO_ERROR         (ESP_ERR_ESPSOL_BASE + 0x0F)

/** @brief Network/connection error */
#define ESP_ERR_ESPSOL_NETWORK_ERROR        (ESP_ERR_ESPSOL_BASE + 0x10)

/** @brief Timeout error */
#define ESP_ERR_ESPSOL_TIMEOUT              (ESP_ERR_ESPSOL_BASE + 0x11)

/** @brief Component not initialized */
#define ESP_ERR_ESPSOL_NOT_INITIALIZED      (ESP_ERR_ESPSOL_BASE + 0x12)

/** @brief Rate limited by RPC server (HTTP 429) */
#define ESP_ERR_ESPSOL_RATE_LIMITED         (ESP_ERR_ESPSOL_BASE + 0x13)

/* ============================================================================
 * Enumerations
 * ========================================================================== */

/**
 * @brief Solana commitment levels
 */
typedef enum {
    ESPSOL_COMMITMENT_PROCESSED = 0,    /**< Transaction has been processed (fastest, least reliable) */
    ESPSOL_COMMITMENT_CONFIRMED = 1,    /**< Transaction has been confirmed by supermajority */
    ESPSOL_COMMITMENT_FINALIZED = 2     /**< Transaction has been finalized (slowest, most reliable) */
} espsol_commitment_t;

/**
 * @brief ESPSOL log levels (maps to ESP-IDF log levels)
 */
typedef enum {
    ESPSOL_LOG_NONE = 0,
    ESPSOL_LOG_ERROR = 1,
    ESPSOL_LOG_WARN = 2,
    ESPSOL_LOG_INFO = 3,
    ESPSOL_LOG_DEBUG = 4,
    ESPSOL_LOG_VERBOSE = 5
} espsol_log_level_t;

/* ============================================================================
 * Configuration Structures
 * ========================================================================== */

/**
 * @brief ESPSOL component configuration
 */
typedef struct {
    const char *rpc_url;                /**< RPC endpoint URL */
    uint32_t timeout_ms;                /**< Request timeout in milliseconds */
    espsol_commitment_t commitment;     /**< Default commitment level */
    bool use_tls;                       /**< Force TLS for connections */
    espsol_log_level_t log_level;       /**< Logging level */
} espsol_config_t;

/**
 * @brief Default configuration initializer
 */
#define ESPSOL_CONFIG_DEFAULT() { \
    .rpc_url = ESPSOL_DEVNET_RPC, \
    .timeout_ms = ESPSOL_DEFAULT_TIMEOUT_MS, \
    .commitment = ESPSOL_COMMITMENT_CONFIRMED, \
    .use_tls = true, \
    .log_level = ESPSOL_LOG_INFO \
}

/* ============================================================================
 * Utility Macros
 * ========================================================================== */

/**
 * @brief Convert lamports to SOL
 */
static inline double espsol_lamports_to_sol(uint64_t lamports) {
    return (double)lamports / (double)ESPSOL_LAMPORTS_PER_SOL;
}

/**
 * @brief Convert SOL to lamports
 */
static inline uint64_t espsol_sol_to_lamports(double sol) {
    return (uint64_t)(sol * (double)ESPSOL_LAMPORTS_PER_SOL);
}

#ifdef __cplusplus
}
#endif

#endif /* ESPSOL_TYPES_H */
