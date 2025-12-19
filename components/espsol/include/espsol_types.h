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
 *
 * All ESPSOL-specific error codes start at ESP_ERR_ESPSOL_BASE (0x50000).
 * Use espsol_err_to_name() to get human-readable error descriptions.
 * Use ESPSOL_IS_ERR() macro to check if an error is ESPSOL-specific.
 *
 * Error Code Ranges:
 *   0x50001-0x50005: Encoding errors (Base58, Base64)
 *   0x50006-0x50007: Crypto errors (keypair, signature)
 *   0x50008-0x50009: RPC errors (request, parse)
 *   0x5000A-0x5000D: Transaction errors (build, sign, limits)
 *   0x5000E-0x50013: System errors (NVS, network, timeout)
 * ========================================================================== */

/** @brief ESPSOL error base (0x50000) */
#define ESP_ERR_ESPSOL_BASE         0x50000

/**
 * @brief Invalid argument provided to ESPSOL function
 * @details Returned when NULL pointers or invalid values are passed.
 *          Check all required parameters are non-NULL.
 */
#define ESP_ERR_ESPSOL_INVALID_ARG          (ESP_ERR_ESPSOL_BASE + 0x01)

/**
 * @brief Buffer too small for operation
 * @details The output buffer provided is too small to hold the result.
 *          Increase the buffer size and retry.
 */
#define ESP_ERR_ESPSOL_BUFFER_TOO_SMALL     (ESP_ERR_ESPSOL_BASE + 0x02)

/**
 * @brief Encoding/decoding operation failed
 * @details A general encoding error occurred (not Base58/Base64 specific).
 */
#define ESP_ERR_ESPSOL_ENCODING_FAILED      (ESP_ERR_ESPSOL_BASE + 0x03)

/**
 * @brief Invalid Base58 input string
 * @details The input contains characters not in the Base58 alphabet
 *          (e.g., 0, O, I, l) or has an invalid checksum.
 */
#define ESP_ERR_ESPSOL_INVALID_BASE58       (ESP_ERR_ESPSOL_BASE + 0x04)

/**
 * @brief Invalid Base64 input string
 * @details The input contains invalid Base64 characters or
 *          has incorrect padding.
 */
#define ESP_ERR_ESPSOL_INVALID_BASE64       (ESP_ERR_ESPSOL_BASE + 0x05)

/**
 * @brief Keypair not initialized
 * @details The keypair structure has not been properly initialized.
 *          Use espsol_keypair_generate() or espsol_keypair_from_seed() first.
 */
#define ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT     (ESP_ERR_ESPSOL_BASE + 0x06)

/**
 * @brief Signature verification failed
 * @details The Ed25519 signature is invalid for the given message
 *          and public key combination.
 */
#define ESP_ERR_ESPSOL_SIGNATURE_INVALID    (ESP_ERR_ESPSOL_BASE + 0x07)

/**
 * @brief RPC request failed
 * @details The JSON-RPC request to the Solana node failed.
 *          Check network connectivity and RPC endpoint URL.
 */
#define ESP_ERR_ESPSOL_RPC_FAILED           (ESP_ERR_ESPSOL_BASE + 0x08)

/**
 * @brief RPC response parse error
 * @details Failed to parse the JSON response from the RPC server.
 *          The response may be malformed or in an unexpected format.
 */
#define ESP_ERR_ESPSOL_RPC_PARSE_ERROR      (ESP_ERR_ESPSOL_BASE + 0x09)

/**
 * @brief Transaction build error
 * @details Failed to build the transaction. Ensure fee payer and
 *          blockhash are set before signing.
 */
#define ESP_ERR_ESPSOL_TX_BUILD_ERROR       (ESP_ERR_ESPSOL_BASE + 0x0A)

/**
 * @brief Transaction not signed
 * @details Attempted to serialize or send an unsigned transaction.
 *          Call espsol_tx_sign() before serialization.
 */
#define ESP_ERR_ESPSOL_TX_NOT_SIGNED        (ESP_ERR_ESPSOL_BASE + 0x0B)

/**
 * @brief Maximum accounts exceeded
 * @details Transaction has too many accounts. Limit: ESPSOL_MAX_ACCOUNTS (20).
 *          Reduce the number of instructions or accounts.
 */
#define ESP_ERR_ESPSOL_MAX_ACCOUNTS         (ESP_ERR_ESPSOL_BASE + 0x0C)

/**
 * @brief Maximum instructions exceeded
 * @details Transaction has too many instructions. Limit: ESPSOL_MAX_INSTRUCTIONS (10).
 *          Split into multiple transactions.
 */
#define ESP_ERR_ESPSOL_MAX_INSTRUCTIONS     (ESP_ERR_ESPSOL_BASE + 0x0D)

/**
 * @brief NVS storage error
 * @details Failed to read/write keypair to NVS flash.
 *          Ensure NVS is initialized with nvs_flash_init().
 */
#define ESP_ERR_ESPSOL_NVS_ERROR            (ESP_ERR_ESPSOL_BASE + 0x0E)

/**
 * @brief Crypto operation failed
 * @details Ed25519 cryptographic operation failed.
 *          This may indicate a library initialization issue.
 */
#define ESP_ERR_ESPSOL_CRYPTO_ERROR         (ESP_ERR_ESPSOL_BASE + 0x0F)

/**
 * @brief Network/connection error
 * @details Failed to establish network connection to RPC server.
 *          Check WiFi connectivity and endpoint URL.
 */
#define ESP_ERR_ESPSOL_NETWORK_ERROR        (ESP_ERR_ESPSOL_BASE + 0x10)

/**
 * @brief Operation timeout
 * @details The operation exceeded the configured timeout.
 *          Increase timeout or check network conditions.
 */
#define ESP_ERR_ESPSOL_TIMEOUT              (ESP_ERR_ESPSOL_BASE + 0x11)

/**
 * @brief Component not initialized
 * @details ESPSOL component has not been initialized.
 *          Call espsol_init() before using other functions.
 */
#define ESP_ERR_ESPSOL_NOT_INITIALIZED      (ESP_ERR_ESPSOL_BASE + 0x12)

/**
 * @brief Rate limited by RPC server (HTTP 429)
 * @details Too many requests sent to the RPC server.
 *          Implement backoff or use a paid RPC endpoint.
 */
#define ESP_ERR_ESPSOL_RATE_LIMITED         (ESP_ERR_ESPSOL_BASE + 0x13)

/** @brief Highest ESPSOL error code (for range checking) */
#define ESP_ERR_ESPSOL_MAX                  ESP_ERR_ESPSOL_RATE_LIMITED

/**
 * @brief Check if an error code is an ESPSOL-specific error
 * @param err Error code to check
 * @return true if error is ESPSOL-specific (0x50001-0x50013)
 */
#define ESPSOL_IS_ERR(err) \
    ((err) >= (ESP_ERR_ESPSOL_BASE + 1) && (err) <= ESP_ERR_ESPSOL_MAX)

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
