# ESPSOL SDK Documentation

**Version**: 0.1.0  
**Author**: SkyRizz  
**License**: Apache 2.0

A production-grade ESP-IDF component for Solana blockchain integration on ESP32 devices.

---

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
   - [Prerequisites](#prerequisites)
   - [Installation](#installation)
   - [Basic Project Setup](#basic-project-setup)
3. [Core Concepts](#core-concepts)
   - [Error Handling](#error-handling)
   - [Memory Management](#memory-management)
   - [Constants and Limits](#constants-and-limits)
4. [API Reference](#api-reference)
   - [Core SDK](#core-sdk-espsol_inith)
   - [Utilities](#utilities-espsol_utilsh)
   - [Cryptography](#cryptography-espsol_cryptoh)
   - [Mnemonic/Seed Phrase](#mnemonicseed-phrase-espsol_mneomich)
   - [RPC Client](#rpc-client-espsol_rpch)
   - [Transactions](#transactions-espsol_txh)
   - [SPL Tokens](#spl-tokens-espsol_tokenh)
5. [Examples](#examples)
   - [Query Network Information](#query-network-information)
   - [Generate and Store Keypair](#generate-and-store-keypair)
   - [Create Wallet with Seed Phrase](#create-wallet-with-seed-phrase)
   - [Transfer SOL](#transfer-sol)
   - [Transfer SPL Tokens](#transfer-spl-tokens)
6. [Configuration](#configuration)
7. [Troubleshooting](#troubleshooting)

---

## Introduction

ESPSOL is a pure C implementation of a Solana SDK designed specifically for ESP32 microcontrollers running ESP-IDF (FreeRTOS). Unlike Arduino-based solutions, ESPSOL uses native ESP-IDF APIs for optimal performance and reliability.

### Key Features

- **Pure C implementation** - No C++ dependencies, minimal memory footprint
- **Native ESP-IDF APIs** - Uses `esp_http_client`, `cJSON`, `nvs_flash`
- **Ed25519 cryptography** - Via libsodium for keypair generation and signing
- **BIP39 mnemonic support** - 12/24-word seed phrases for wallet backup and recovery
- **Full RPC support** - JSON-RPC 2.0 client with retry and backoff
- **Transaction building** - System Program and SPL Token instructions
- **Secure storage** - NVS integration for persistent keypair storage
- **Automatic retry** - Exponential backoff for network reliability

### Resource Usage

| Resource | Usage |
|----------|-------|
| Flash | ~860 KB |
| RAM (static) | ~4 KB |
| RAM (runtime) | ~8-16 KB |
| Stack per operation | ~4 KB recommended |

---

## Getting Started

### Prerequisites

- **ESP-IDF v5.0+** (tested with v5.5.1)
- **ESP32, ESP32-S2, ESP32-S3, or ESP32-C3**
- **Wi-Fi or Ethernet connectivity**
- Basic familiarity with ESP-IDF development

### Installation

#### Option 1: Clone into components

```bash
cd your_project
mkdir -p components
git clone https://github.com/SkyRizzGames/espsol.git components/espsol
```

#### Option 2: ESP Component Registry (coming soon)

```bash
idf.py add-dependency "SkyRizzGames/espsol"
```

### Basic Project Setup

1. **Create a new ESP-IDF project**:

```bash
idf.py create-project my_solana_app
cd my_solana_app
```

2. **Add ESPSOL to your project**:

```bash
mkdir -p components
git clone https://github.com/SkyRizzGames/espsol.git components/espsol
```

3. **Update your main CMakeLists.txt**:

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES espsol nvs_flash esp_wifi
)
```

4. **Include ESPSOL in your code**:

```c
#include "espsol.h"
```

5. **Build and flash**:

```bash
idf.py set-target esp32  # or esp32c3, esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## Core Concepts

### Error Handling

All ESPSOL functions return `esp_err_t`. Always check return values:

```c
esp_err_t err = espsol_keypair_generate(&keypair);
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to generate keypair: %s", espsol_err_to_name(err));
    return err;
}
```

#### ESPSOL Error Codes

| Error Code | Value | Description |
|------------|-------|-------------|
| `ESP_ERR_ESPSOL_INVALID_ARG` | 0x50001 | Invalid argument provided |
| `ESP_ERR_ESPSOL_BUFFER_TOO_SMALL` | 0x50002 | Output buffer too small |
| `ESP_ERR_ESPSOL_ENCODING_FAILED` | 0x50003 | Encoding/decoding error |
| `ESP_ERR_ESPSOL_INVALID_BASE58` | 0x50004 | Invalid Base58 input |
| `ESP_ERR_ESPSOL_INVALID_BASE64` | 0x50005 | Invalid Base64 input |
| `ESP_ERR_ESPSOL_KEYPAIR_NOT_INIT` | 0x50006 | Keypair not initialized |
| `ESP_ERR_ESPSOL_SIGNATURE_INVALID` | 0x50007 | Signature verification failed |
| `ESP_ERR_ESPSOL_RPC_FAILED` | 0x50008 | RPC request failed |
| `ESP_ERR_ESPSOL_RPC_PARSE_ERROR` | 0x50009 | JSON parse error |
| `ESP_ERR_ESPSOL_TX_BUILD_ERROR` | 0x5000A | Transaction build error |
| `ESP_ERR_ESPSOL_TX_NOT_SIGNED` | 0x5000B | Transaction not signed |
| `ESP_ERR_ESPSOL_MAX_ACCOUNTS` | 0x5000C | Max accounts exceeded |
| `ESP_ERR_ESPSOL_MAX_INSTRUCTIONS` | 0x5000D | Max instructions exceeded |
| `ESP_ERR_ESPSOL_NVS_ERROR` | 0x5000E | NVS storage error |
| `ESP_ERR_ESPSOL_CRYPTO_ERROR` | 0x5000F | Crypto operation failed |
| `ESP_ERR_ESPSOL_NETWORK_ERROR` | 0x50010 | Network connection error |
| `ESP_ERR_ESPSOL_TIMEOUT` | 0x50011 | Operation timeout |
| `ESP_ERR_ESPSOL_NOT_INITIALIZED` | 0x50012 | Component not initialized |
| `ESP_ERR_ESPSOL_RATE_LIMITED` | 0x50013 | Rate limited (HTTP 429) |
| `ESP_ERR_ESPSOL_INVALID_MNEMONIC` | 0x50014 | Invalid mnemonic phrase |

### Memory Management

ESPSOL uses static buffers where possible to avoid heap fragmentation:

- **Stack buffers** - Most operations use stack-allocated buffers
- **Caller-allocated** - Output buffers are always provided by the caller
- **Explicit cleanup** - Use `espsol_keypair_clear()` to zero sensitive data

```c
// Good practice: always clear keypairs when done
espsol_keypair_t keypair;
espsol_keypair_generate(&keypair);
// ... use keypair ...
espsol_keypair_clear(&keypair);  // Zeros private key
```

### Constants and Limits

```c
// Size constants
#define ESPSOL_PUBKEY_SIZE          32    // Public key size in bytes
#define ESPSOL_PRIVKEY_SIZE         64    // Private key size in bytes
#define ESPSOL_SIGNATURE_SIZE       64    // Signature size in bytes
#define ESPSOL_BLOCKHASH_SIZE       32    // Blockhash size in bytes
#define ESPSOL_ADDRESS_MAX_LEN      45    // Max Base58 address length

// Transaction limits
#define ESPSOL_MAX_INSTRUCTIONS     10    // Max instructions per tx
#define ESPSOL_MAX_ACCOUNTS         20    // Max accounts per tx
#define ESPSOL_MAX_SIGNERS          4     // Max signers per tx
#define ESPSOL_MAX_TX_SIZE          1232  // Max serialized tx size

// Network endpoints
#define ESPSOL_MAINNET_RPC  "https://api.mainnet-beta.solana.com"
#define ESPSOL_DEVNET_RPC   "https://api.devnet.solana.com"
#define ESPSOL_TESTNET_RPC  "https://api.testnet.solana.com"

// Utility
#define ESPSOL_LAMPORTS_PER_SOL  1000000000ULL  // 1 SOL = 10^9 lamports
```

---

## API Reference

### Core SDK (`espsol.h`)

The main header that includes all ESPSOL modules.

#### espsol_init

Initialize the ESPSOL component.

```c
esp_err_t espsol_init(const espsol_config_t *config);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `config` | `const espsol_config_t*` | Configuration (NULL for defaults) |

**Returns:** `ESP_OK` on success

**Example:**
```c
espsol_config_t config = ESPSOL_CONFIG_DEFAULT();
config.rpc_url = ESPSOL_DEVNET_RPC;
ESP_ERROR_CHECK(espsol_init(&config));
```

#### espsol_deinit

Deinitialize the ESPSOL component.

```c
esp_err_t espsol_deinit(void);
```

#### espsol_get_version

Get the ESPSOL version string.

```c
const char* espsol_get_version(void);
```

**Returns:** Version string (e.g., "0.1.0")

#### espsol_err_to_name

Convert an error code to human-readable string.

```c
const char* espsol_err_to_name(esp_err_t err);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `err` | `esp_err_t` | Error code to convert |

**Returns:** Error description string

---

### Utilities (`espsol_utils.h`)

Encoding/decoding utilities for Base58, Base64, and hex.

#### espsol_base58_encode

Encode binary data to Base58 (Solana/Bitcoin alphabet).

```c
esp_err_t espsol_base58_encode(
    const uint8_t *data,      // Input data
    size_t data_len,          // Input length
    char *output,             // Output buffer (null-terminated)
    size_t output_len         // Output buffer size
);
```

**Example:**
```c
uint8_t pubkey[32] = { /* ... */ };
char address[ESPSOL_ADDRESS_MAX_LEN];
ESP_ERROR_CHECK(espsol_base58_encode(pubkey, 32, address, sizeof(address)));
printf("Address: %s\n", address);
```

#### espsol_base58_decode

Decode Base58 string to binary data.

```c
esp_err_t espsol_base58_decode(
    const char *input,        // Base58 string
    uint8_t *output,          // Output buffer
    size_t *output_len        // In: buffer size, Out: decoded length
);
```

**Example:**
```c
const char *address = "11111111111111111111111111111111";
uint8_t pubkey[32];
size_t len = sizeof(pubkey);
ESP_ERROR_CHECK(espsol_base58_decode(address, pubkey, &len));
// len is now 32
```

#### espsol_base64_encode

Encode binary data to Base64.

```c
esp_err_t espsol_base64_encode(
    const uint8_t *data,      // Input data
    size_t data_len,          // Input length
    char *output,             // Output buffer
    size_t output_len         // Output buffer size
);
```

#### espsol_base64_decode

Decode Base64 string to binary data.

```c
esp_err_t espsol_base64_decode(
    const char *input,        // Base64 string
    uint8_t *output,          // Output buffer
    size_t *output_len        // In: buffer size, Out: decoded length
);
```

#### espsol_pubkey_to_address

Convert a 32-byte public key to Base58 address string.

```c
esp_err_t espsol_pubkey_to_address(
    const uint8_t pubkey[32], // Public key bytes
    char *address,            // Output address string
    size_t len                // Buffer size (>= 45)
);
```

#### espsol_address_to_pubkey

Convert a Base58 address to 32-byte public key.

```c
esp_err_t espsol_address_to_pubkey(
    const char *address,      // Base58 address
    uint8_t pubkey[32]        // Output public key
);
```

#### espsol_is_valid_address

Check if a string is a valid Solana address.

```c
bool espsol_is_valid_address(const char *address);
```

**Example:**
```c
if (espsol_is_valid_address("So11111111111111111111111111111111111111112")) {
    printf("Valid address\n");
}
```

#### Utility Macros

```c
// Convert lamports to SOL
double sol = espsol_lamports_to_sol(1000000000);  // Returns 1.0

// Convert SOL to lamports
uint64_t lamports = espsol_sol_to_lamports(0.5);  // Returns 500000000
```

---

### Cryptography (`espsol_crypto.h`)

Ed25519 keypair generation, signing, and verification.

#### espsol_keypair_t

Keypair structure containing public and private keys.

```c
typedef struct {
    uint8_t public_key[32];   // Ed25519 public key
    uint8_t private_key[64];  // Ed25519 private key (seed + public)
    bool initialized;         // Validity flag
} espsol_keypair_t;
```

#### espsol_crypto_init

Initialize the crypto subsystem.

```c
esp_err_t espsol_crypto_init(void);
```

Call once at startup before using crypto functions.

#### espsol_keypair_generate

Generate a new random Ed25519 keypair.

```c
esp_err_t espsol_keypair_generate(espsol_keypair_t *keypair);
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `keypair` | `espsol_keypair_t*` | Output keypair structure |

**Example:**
```c
espsol_keypair_t keypair;
ESP_ERROR_CHECK(espsol_keypair_generate(&keypair));

char address[ESPSOL_ADDRESS_MAX_LEN];
espsol_keypair_get_address(&keypair, address, sizeof(address));
ESP_LOGI(TAG, "Generated wallet: %s", address);
```

#### espsol_keypair_from_seed

Generate keypair from a 32-byte seed (deterministic).

```c
esp_err_t espsol_keypair_from_seed(
    const uint8_t seed[32],   // 32-byte seed
    espsol_keypair_t *keypair // Output keypair
);
```

**Example:**
```c
// Same seed always produces same keypair
uint8_t seed[32] = {0x01, 0x02, /* ... */};
espsol_keypair_t keypair;
ESP_ERROR_CHECK(espsol_keypair_from_seed(seed, &keypair));
```

#### espsol_keypair_from_private_key

Import keypair from 64-byte private key.

```c
esp_err_t espsol_keypair_from_private_key(
    const uint8_t private_key[64],  // 64-byte private key
    espsol_keypair_t *keypair       // Output keypair
);
```

#### espsol_keypair_from_base58

Import keypair from Base58-encoded private key.

```c
esp_err_t espsol_keypair_from_base58(
    const char *base58_key,   // Base58 private key string
    espsol_keypair_t *keypair // Output keypair
);
```

**Example:**
```c
// Import from Phantom/Solflare export
const char *privkey_b58 = "4wBqpZM9...";  // Your private key
espsol_keypair_t keypair;
ESP_ERROR_CHECK(espsol_keypair_from_base58(privkey_b58, &keypair));
```

#### espsol_keypair_to_base58

Export keypair private key to Base58.

```c
esp_err_t espsol_keypair_to_base58(
    const espsol_keypair_t *keypair, // Keypair to export
    char *output,                    // Output buffer
    size_t output_len                // Buffer size (>= 128)
);
```

#### espsol_keypair_get_address

Get the wallet address as Base58 string.

```c
esp_err_t espsol_keypair_get_address(
    const espsol_keypair_t *keypair, // Keypair
    char *address,                   // Output address
    size_t address_len               // Buffer size (>= 45)
);
```

#### espsol_sign

Sign a message with Ed25519.

```c
esp_err_t espsol_sign(
    const uint8_t *message,          // Message to sign
    size_t message_len,              // Message length
    const espsol_keypair_t *keypair, // Keypair to sign with
    uint8_t signature[64]            // Output signature
);
```

**Example:**
```c
uint8_t message[] = "Hello, Solana!";
uint8_t signature[ESPSOL_SIGNATURE_SIZE];
ESP_ERROR_CHECK(espsol_sign(message, sizeof(message) - 1, &keypair, signature));
```

#### espsol_verify

Verify an Ed25519 signature.

```c
esp_err_t espsol_verify(
    const uint8_t *message,          // Original message
    size_t message_len,              // Message length
    const uint8_t signature[64],     // Signature to verify
    const uint8_t public_key[32]     // Signer's public key
);
```

**Returns:** `ESP_OK` if valid, `ESP_ERR_ESPSOL_SIGNATURE_INVALID` if invalid

#### espsol_keypair_clear

Securely zero out a keypair (for security).

```c
esp_err_t espsol_keypair_clear(espsol_keypair_t *keypair);
```

**Always call this when done with a keypair.**

#### NVS Storage Functions (ESP32 only)

##### espsol_keypair_save_to_nvs

Save a keypair to Non-Volatile Storage.

```c
esp_err_t espsol_keypair_save_to_nvs(
    const espsol_keypair_t *keypair, // Keypair to save
    const char *nvs_key              // Storage key (max 15 chars)
);
```

**Example:**
```c
// Initialize NVS first
nvs_flash_init();

// Generate and save keypair
espsol_keypair_t keypair;
espsol_keypair_generate(&keypair);
ESP_ERROR_CHECK(espsol_keypair_save_to_nvs(&keypair, "main_wallet"));
```

##### espsol_keypair_load_from_nvs

Load a keypair from NVS.

```c
esp_err_t espsol_keypair_load_from_nvs(
    const char *nvs_key,       // Storage key
    espsol_keypair_t *keypair  // Output keypair
);
```

**Example:**
```c
espsol_keypair_t keypair;
esp_err_t err = espsol_keypair_load_from_nvs("main_wallet", &keypair);
if (err == ESP_OK) {
    ESP_LOGI(TAG, "Loaded existing wallet");
} else if (err == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGI(TAG, "No wallet found, generating new one");
    espsol_keypair_generate(&keypair);
    espsol_keypair_save_to_nvs(&keypair, "main_wallet");
}
```

##### espsol_keypair_delete_from_nvs

Delete a keypair from NVS.

```c
esp_err_t espsol_keypair_delete_from_nvs(const char *nvs_key);
```

##### espsol_keypair_exists_in_nvs

Check if a keypair exists in NVS.

```c
esp_err_t espsol_keypair_exists_in_nvs(
    const char *nvs_key,  // Storage key
    bool *exists          // Output: true if exists
);
```

---

### Mnemonic/Seed Phrase (`espsol_mnemonic.h`)

BIP39 mnemonic seed phrase support for wallet backup and recovery. Mnemonics provide a human-readable way to backup and restore wallets across devices and applications.

> ⚠️ **Security Warning**: Mnemonic phrases give full access to your wallet. Treat them like private keys - never share, store securely, and clear from memory when done.

#### BIP39 Overview

- **12-word mnemonic**: 128-bit entropy (standard security)
- **24-word mnemonic**: 256-bit entropy (high security)
- **Passphrase**: Optional extra word for additional security
- **Compatibility**: Works with Phantom, Solflare, and other Solana wallets

#### Constants

```c
#define ESPSOL_MNEMONIC_12_WORDS      12    // 12-word mnemonic
#define ESPSOL_MNEMONIC_24_WORDS      24    // 24-word mnemonic
#define ESPSOL_MNEMONIC_12_MAX_LEN    128   // Max buffer for 12 words
#define ESPSOL_MNEMONIC_24_MAX_LEN    256   // Max buffer for 24 words
#define ESPSOL_ENTROPY_12_SIZE        16    // 128-bit entropy
#define ESPSOL_ENTROPY_24_SIZE        32    // 256-bit entropy
#define ESPSOL_MNEMONIC_SEED_SIZE     64    // BIP39 seed size
#define ESPSOL_BIP39_WORDLIST_SIZE    2048  // BIP39 wordlist
```

#### espsol_mnemonic_generate_12

Generate a random 12-word mnemonic phrase.

```c
esp_err_t espsol_mnemonic_generate_12(
    char *mnemonic,       // Output buffer (min 128 bytes)
    size_t mnemonic_len   // Buffer size
);
```

**Example:**
```c
char mnemonic[ESPSOL_MNEMONIC_12_MAX_LEN];
ESP_ERROR_CHECK(espsol_mnemonic_generate_12(mnemonic, sizeof(mnemonic)));
printf("Seed phrase: %s\n", mnemonic);
```

#### espsol_mnemonic_generate_24

Generate a random 24-word mnemonic phrase (recommended for high security).

```c
esp_err_t espsol_mnemonic_generate_24(
    char *mnemonic,       // Output buffer (min 256 bytes)
    size_t mnemonic_len   // Buffer size
);
```

#### espsol_mnemonic_from_entropy

Generate mnemonic from provided entropy (for deterministic generation).

```c
esp_err_t espsol_mnemonic_from_entropy(
    const uint8_t *entropy,   // Entropy bytes (16 or 32)
    size_t entropy_len,       // 16 for 12-word, 32 for 24-word
    char *mnemonic,           // Output buffer
    size_t mnemonic_len       // Buffer size
);
```

**Example:**
```c
// Generate deterministic mnemonic from known entropy
uint8_t entropy[16] = {0x00, 0x00, 0x00, /* ... all zeros ... */};
char mnemonic[ESPSOL_MNEMONIC_12_MAX_LEN];
espsol_mnemonic_from_entropy(entropy, 16, mnemonic, sizeof(mnemonic));
// Result: "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
```

#### espsol_mnemonic_validate

Validate a mnemonic phrase (checks words, count, and checksum).

```c
esp_err_t espsol_mnemonic_validate(const char *mnemonic);
```

**Returns:**
- `ESP_OK` if mnemonic is valid
- `ESP_ERR_ESPSOL_INVALID_MNEMONIC` if invalid words, wrong count, or bad checksum

**Example:**
```c
if (espsol_mnemonic_validate(user_input) == ESP_OK) {
    printf("Valid mnemonic!\n");
} else {
    printf("Invalid mnemonic - check your words\n");
}
```

#### espsol_mnemonic_word_valid

Check if a single word is in the BIP39 wordlist.

```c
bool espsol_mnemonic_word_valid(
    const char *word,     // Word to check
    int *index            // Optional: output word index (0-2047)
);
```

**Example:**
```c
int idx;
if (espsol_mnemonic_word_valid("abandon", &idx)) {
    printf("'abandon' is at index %d\n", idx);  // Output: 0
}
```

#### espsol_mnemonic_word_count

Get the number of words in a mnemonic string.

```c
int espsol_mnemonic_word_count(const char *mnemonic);
```

**Returns:** Word count (12 or 24 for valid mnemonics), or 0 on error

#### espsol_mnemonic_to_seed

Convert mnemonic to 64-byte BIP39 seed using PBKDF2-SHA512.

```c
esp_err_t espsol_mnemonic_to_seed(
    const char *mnemonic,                   // Mnemonic phrase
    const char *passphrase,                 // Optional passphrase (NULL for none)
    uint8_t seed[ESPSOL_MNEMONIC_SEED_SIZE] // Output 64-byte seed
);
```

> **Note**: Different passphrases produce different seeds (and wallets) from the same mnemonic.

#### espsol_keypair_from_mnemonic

Derive a Solana keypair from mnemonic phrase.

```c
esp_err_t espsol_keypair_from_mnemonic(
    const char *mnemonic,         // Mnemonic phrase (12 or 24 words)
    const char *passphrase,       // Optional BIP39 passphrase (NULL for none)
    espsol_keypair_t *keypair     // Output keypair
);
```

**Example: Wallet Recovery**
```c
// User enters their saved seed phrase
const char *saved_mnemonic = "abandon ability able about above absent absorb abstract absurd abuse access accident";

espsol_keypair_t wallet;
ESP_ERROR_CHECK(espsol_keypair_from_mnemonic(saved_mnemonic, NULL, &wallet));

char address[ESPSOL_ADDRESS_MAX_LEN];
espsol_keypair_get_address(&wallet, address, sizeof(address));
printf("Recovered wallet: %s\n", address);
```

#### espsol_keypair_generate_with_mnemonic

Generate a new keypair with mnemonic in one call (convenience function).

```c
esp_err_t espsol_keypair_generate_with_mnemonic(
    int word_count,           // 12 or 24
    char *mnemonic,           // Output mnemonic buffer
    size_t mnemonic_len,      // Buffer size
    espsol_keypair_t *keypair // Output keypair
);
```

**Example: Create New Wallet with Backup**
```c
char mnemonic[ESPSOL_MNEMONIC_12_MAX_LEN];
espsol_keypair_t wallet;

ESP_ERROR_CHECK(espsol_keypair_generate_with_mnemonic(
    12, mnemonic, sizeof(mnemonic), &wallet
));

// Show user their seed phrase for backup
printf("\n╔════════════════════════════════════╗\n");
printf("║ ⚠️  BACKUP YOUR SEED PHRASE ⚠️      ║\n");
printf("╠════════════════════════════════════╣\n");
printf("║ %s\n", mnemonic);
printf("╚════════════════════════════════════╝\n");

char address[ESPSOL_ADDRESS_MAX_LEN];
espsol_keypair_get_address(&wallet, address, sizeof(address));
printf("Wallet: %s\n", address);

// Clear mnemonic after user confirms backup
espsol_mnemonic_clear(mnemonic, sizeof(mnemonic));
```

#### espsol_mnemonic_get_word

Get a word from the BIP39 wordlist by index.

```c
const char *espsol_mnemonic_get_word(int index);  // 0-2047
```

**Returns:** Word string, or NULL if index out of range

#### espsol_mnemonic_print

Print mnemonic with numbered words for easy backup.

```c
void espsol_mnemonic_print(
    const char *mnemonic,  // Mnemonic to print
    const char *label      // Optional label (e.g., "BACKUP")
);
```

**Output:**
```
=== BACKUP ===
 1. abandon
 2. ability
 3. able
...
```

#### espsol_mnemonic_clear

Securely zero out mnemonic from memory.

```c
void espsol_mnemonic_clear(char *mnemonic, size_t len);
```

**Always call this after the user has backed up their mnemonic.**

---

### RPC Client (`espsol_rpc.h`)

JSON-RPC 2.0 client for Solana network communication.

#### espsol_rpc_config_t

RPC client configuration.

```c
typedef struct {
    const char *endpoint;          // RPC URL
    uint32_t timeout_ms;           // Request timeout (default: 30000)
    espsol_commitment_t commitment; // Commitment level
    size_t buffer_size;            // HTTP buffer size (default: 4096)
    uint8_t max_retries;           // Retry attempts (default: 3)
    uint32_t retry_delay_ms;       // Initial retry delay (default: 500)
} espsol_rpc_config_t;

// Default configuration
#define ESPSOL_RPC_CONFIG_DEFAULT() { \
    .endpoint = ESPSOL_DEVNET_RPC, \
    .timeout_ms = 30000, \
    .commitment = ESPSOL_COMMITMENT_CONFIRMED, \
    .buffer_size = 4096, \
    .max_retries = 3, \
    .retry_delay_ms = 500 \
}
```

#### espsol_rpc_init

Initialize RPC client with default configuration.

```c
esp_err_t espsol_rpc_init(
    espsol_rpc_handle_t *handle,  // Output handle
    const char *endpoint          // RPC URL
);
```

**Example:**
```c
espsol_rpc_handle_t rpc;
ESP_ERROR_CHECK(espsol_rpc_init(&rpc, ESPSOL_DEVNET_RPC));
```

#### espsol_rpc_init_with_config

Initialize RPC client with custom configuration.

```c
esp_err_t espsol_rpc_init_with_config(
    espsol_rpc_handle_t *handle,      // Output handle
    const espsol_rpc_config_t *config // Configuration
);
```

**Example:**
```c
espsol_rpc_config_t config = ESPSOL_RPC_CONFIG_DEFAULT();
config.timeout_ms = 60000;
config.max_retries = 5;

espsol_rpc_handle_t rpc;
ESP_ERROR_CHECK(espsol_rpc_init_with_config(&rpc, &config));
```

#### espsol_rpc_deinit

Deinitialize RPC client and free resources.

```c
esp_err_t espsol_rpc_deinit(espsol_rpc_handle_t handle);
```

#### espsol_rpc_get_version

Get Solana node version.

```c
esp_err_t espsol_rpc_get_version(
    espsol_rpc_handle_t handle,  // RPC handle
    char *version,               // Output buffer
    size_t len                   // Buffer size
);
```

**Example:**
```c
char version[64];
ESP_ERROR_CHECK(espsol_rpc_get_version(rpc, version, sizeof(version)));
ESP_LOGI(TAG, "Solana version: %s", version);
```

#### espsol_rpc_get_slot

Get current slot.

```c
esp_err_t espsol_rpc_get_slot(
    espsol_rpc_handle_t handle,  // RPC handle
    uint64_t *slot               // Output slot
);
```

#### espsol_rpc_get_block_height

Get current block height.

```c
esp_err_t espsol_rpc_get_block_height(
    espsol_rpc_handle_t handle,  // RPC handle
    uint64_t *height             // Output height
);
```

#### espsol_rpc_get_health

Check if node is healthy.

```c
esp_err_t espsol_rpc_get_health(
    espsol_rpc_handle_t handle,  // RPC handle
    bool *is_healthy             // Output health status
);
```

#### espsol_rpc_get_balance

Get account balance in lamports.

```c
esp_err_t espsol_rpc_get_balance(
    espsol_rpc_handle_t handle,  // RPC handle
    const char *pubkey,          // Base58 address
    uint64_t *lamports           // Output balance
);
```

**Example:**
```c
uint64_t balance;
ESP_ERROR_CHECK(espsol_rpc_get_balance(rpc, "Your...Address", &balance));
ESP_LOGI(TAG, "Balance: %.9f SOL", espsol_lamports_to_sol(balance));
```

#### espsol_rpc_get_account_info

Get detailed account information.

```c
esp_err_t espsol_rpc_get_account_info(
    espsol_rpc_handle_t handle,    // RPC handle
    const char *pubkey,            // Base58 address
    espsol_account_info_t *info    // Output account info
);
```

**Response structure:**
```c
typedef struct {
    char owner[45];       // Owner program
    uint64_t lamports;    // Balance
    uint8_t *data;        // Account data (caller allocates)
    size_t data_len;      // Data length
    size_t data_capacity; // Buffer capacity
    bool executable;      // Is program
    uint64_t rent_epoch;  // Rent epoch
} espsol_account_info_t;
```

#### espsol_rpc_get_latest_blockhash

Get the latest blockhash (required for transactions).

```c
esp_err_t espsol_rpc_get_latest_blockhash(
    espsol_rpc_handle_t handle,       // RPC handle
    uint8_t blockhash[32],            // Output blockhash
    uint64_t *last_valid_block_height // Output (can be NULL)
);
```

**Example:**
```c
uint8_t blockhash[ESPSOL_BLOCKHASH_SIZE];
uint64_t last_valid;
ESP_ERROR_CHECK(espsol_rpc_get_latest_blockhash(rpc, blockhash, &last_valid));
```

#### espsol_rpc_send_transaction

Send a signed transaction to the network.

```c
esp_err_t espsol_rpc_send_transaction(
    espsol_rpc_handle_t handle,  // RPC handle
    const char *tx_base64,       // Base64-encoded transaction
    char *signature,             // Output signature
    size_t sig_len               // Signature buffer size (>= 90)
);
```

#### espsol_rpc_confirm_transaction

Wait for transaction confirmation.

```c
esp_err_t espsol_rpc_confirm_transaction(
    espsol_rpc_handle_t handle,  // RPC handle
    const char *signature,       // Transaction signature
    uint32_t timeout_ms,         // Timeout in milliseconds
    bool *confirmed              // Output confirmation status
);
```

**Example:**
```c
bool confirmed = false;
esp_err_t err = espsol_rpc_confirm_transaction(rpc, signature, 30000, &confirmed);
if (err == ESP_OK && confirmed) {
    ESP_LOGI(TAG, "Transaction confirmed!");
}
```

#### espsol_rpc_request_airdrop

Request SOL airdrop (devnet/testnet only).

```c
esp_err_t espsol_rpc_request_airdrop(
    espsol_rpc_handle_t handle,  // RPC handle
    const char *pubkey,          // Recipient address
    uint64_t lamports,           // Amount (max 2 SOL)
    char *signature,             // Output signature
    size_t sig_len               // Signature buffer size
);
```

**Example:**
```c
char sig[ESPSOL_SIGNATURE_MAX_LEN];
uint64_t amount = espsol_sol_to_lamports(1.0);  // 1 SOL
ESP_ERROR_CHECK(espsol_rpc_request_airdrop(rpc, address, amount, sig, sizeof(sig)));
ESP_LOGI(TAG, "Airdrop signature: %s", sig);
```

#### espsol_rpc_get_token_balance

Get SPL token account balance.

```c
esp_err_t espsol_rpc_get_token_balance(
    espsol_rpc_handle_t handle,    // RPC handle
    const char *token_account,     // Token account address
    uint64_t *amount,              // Output raw amount
    uint8_t *decimals              // Output decimals (can be NULL)
);
```

#### espsol_rpc_call

Make a generic JSON-RPC call (for unsupported methods).

```c
esp_err_t espsol_rpc_call(
    espsol_rpc_handle_t handle,  // RPC handle
    const char *method,          // RPC method name
    const char *params_json,     // JSON params or NULL
    char *response,              // Output buffer
    size_t response_len          // Buffer size
);
```

**Example:**
```c
char response[1024];
ESP_ERROR_CHECK(espsol_rpc_call(
    rpc,
    "getMinimumBalanceForRentExemption",
    "[165]",  // Token account size
    response,
    sizeof(response)
));
```

---

### Transactions (`espsol_tx.h`)

Transaction building, signing, and serialization.

#### Transaction Lifecycle

1. **Create** - `espsol_tx_create()`
2. **Configure** - Set fee payer and blockhash
3. **Add Instructions** - Transfer, create account, etc.
4. **Sign** - Sign with required keypairs
5. **Serialize** - Convert to Base64 for submission
6. **Destroy** - Clean up resources

#### espsol_tx_create

Create a new transaction.

```c
esp_err_t espsol_tx_create(espsol_tx_handle_t *tx);
```

#### espsol_tx_destroy

Destroy a transaction and free resources.

```c
esp_err_t espsol_tx_destroy(espsol_tx_handle_t tx);
```

#### espsol_tx_set_fee_payer

Set the transaction fee payer.

```c
esp_err_t espsol_tx_set_fee_payer(
    espsol_tx_handle_t tx,       // Transaction
    const uint8_t pubkey[32]     // Fee payer public key
);
```

#### espsol_tx_set_recent_blockhash

Set the recent blockhash (from `espsol_rpc_get_latest_blockhash`).

```c
esp_err_t espsol_tx_set_recent_blockhash(
    espsol_tx_handle_t tx,       // Transaction
    const uint8_t blockhash[32]  // Recent blockhash
);
```

#### espsol_tx_add_transfer

Add a SOL transfer instruction.

```c
esp_err_t espsol_tx_add_transfer(
    espsol_tx_handle_t tx,       // Transaction
    const uint8_t from[32],      // Sender (must sign)
    const uint8_t to[32],        // Recipient
    uint64_t lamports            // Amount in lamports
);
```

**Complete Example:**
```c
// Create transaction
espsol_tx_handle_t tx;
ESP_ERROR_CHECK(espsol_tx_create(&tx));

// Configure
ESP_ERROR_CHECK(espsol_tx_set_fee_payer(tx, keypair.public_key));
ESP_ERROR_CHECK(espsol_tx_set_recent_blockhash(tx, blockhash));

// Add transfer (0.001 SOL)
uint8_t recipient[32];
espsol_address_to_pubkey("Recipient...Address", recipient);
ESP_ERROR_CHECK(espsol_tx_add_transfer(
    tx, 
    keypair.public_key, 
    recipient, 
    espsol_sol_to_lamports(0.001)
));

// Sign
ESP_ERROR_CHECK(espsol_tx_sign(tx, &keypair));

// Serialize to Base64
char tx_base64[2048];
ESP_ERROR_CHECK(espsol_tx_to_base64(tx, tx_base64, sizeof(tx_base64)));

// Send
char signature[ESPSOL_SIGNATURE_MAX_LEN];
ESP_ERROR_CHECK(espsol_rpc_send_transaction(rpc, tx_base64, signature, sizeof(signature)));

// Cleanup
espsol_tx_destroy(tx);
```

#### espsol_tx_add_memo

Add a memo to the transaction.

```c
esp_err_t espsol_tx_add_memo(
    espsol_tx_handle_t tx,  // Transaction
    const char *memo        // Memo string (UTF-8)
);
```

#### espsol_tx_add_instruction

Add a custom instruction (for advanced use).

```c
esp_err_t espsol_tx_add_instruction(
    espsol_tx_handle_t tx,               // Transaction
    const uint8_t program_id[32],        // Program ID
    const espsol_account_meta_t *accounts, // Account metadata
    size_t account_count,                // Number of accounts
    const uint8_t *data,                 // Instruction data
    size_t data_len                      // Data length
);
```

**Account metadata structure:**
```c
typedef struct {
    uint8_t pubkey[32];  // Account public key
    bool is_signer;      // Must sign?
    bool is_writable;    // Can modify?
} espsol_account_meta_t;
```

#### espsol_tx_sign

Sign the transaction.

```c
esp_err_t espsol_tx_sign(
    espsol_tx_handle_t tx,           // Transaction
    const espsol_keypair_t *keypair  // Signing keypair
);
```

#### espsol_tx_to_base64

Serialize transaction to Base64 for RPC submission.

```c
esp_err_t espsol_tx_to_base64(
    espsol_tx_handle_t tx,  // Transaction
    char *output,           // Output buffer
    size_t output_len       // Buffer size
);
```

#### espsol_tx_get_signature_base58

Get the transaction signature (transaction ID).

```c
esp_err_t espsol_tx_get_signature_base58(
    espsol_tx_handle_t tx,  // Transaction
    char *output,           // Output buffer
    size_t output_len       // Buffer size (>= 90)
);
```

---

### SPL Tokens (`espsol_token.h`)

SPL Token operations for token transfers and account management.

#### espsol_token_get_ata_address

Derive the Associated Token Account (ATA) address.

```c
esp_err_t espsol_token_get_ata_address(
    const uint8_t wallet[32],      // Wallet owner
    const uint8_t mint[32],        // Token mint
    uint8_t ata_address[32]        // Output ATA address
);
```

**Example:**
```c
uint8_t usdc_mint[32];
espsol_address_to_pubkey("EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v", usdc_mint);

uint8_t my_ata[32];
espsol_token_get_ata_address(keypair.public_key, usdc_mint, my_ata);

char ata_address[ESPSOL_ADDRESS_MAX_LEN];
espsol_pubkey_to_address(my_ata, ata_address, sizeof(ata_address));
ESP_LOGI(TAG, "My USDC ATA: %s", ata_address);
```

#### espsol_tx_add_create_ata

Add instruction to create an Associated Token Account.

```c
esp_err_t espsol_tx_add_create_ata(
    espsol_tx_handle_t tx,       // Transaction
    const uint8_t payer[32],     // Fee payer (pays rent)
    const uint8_t wallet[32],    // Token account owner
    const uint8_t mint[32]       // Token mint
);
```

#### espsol_tx_add_create_ata_idempotent

Create ATA (won't fail if already exists) - **recommended**.

```c
esp_err_t espsol_tx_add_create_ata_idempotent(
    espsol_tx_handle_t tx,       // Transaction
    const uint8_t payer[32],     // Fee payer
    const uint8_t wallet[32],    // Token account owner
    const uint8_t mint[32]       // Token mint
);
```

#### espsol_tx_add_token_transfer

Add a basic token transfer instruction.

```c
esp_err_t espsol_tx_add_token_transfer(
    espsol_tx_handle_t tx,       // Transaction
    const uint8_t source[32],    // Source token account
    const uint8_t dest[32],      // Destination token account
    const uint8_t owner[32],     // Source owner (must sign)
    uint64_t amount              // Token amount (raw units)
);
```

#### espsol_tx_add_token_transfer_checked

Add a checked token transfer (recommended, validates decimals).

```c
esp_err_t espsol_tx_add_token_transfer_checked(
    espsol_tx_handle_t tx,       // Transaction
    const uint8_t source[32],    // Source token account
    const uint8_t mint[32],      // Token mint
    const uint8_t dest[32],      // Destination token account
    const uint8_t owner[32],     // Source owner (must sign)
    uint64_t amount,             // Token amount (raw units)
    uint8_t decimals             // Expected decimals
);
```

**Example: Transfer 1 USDC (6 decimals)**
```c
// 1 USDC = 1 * 10^6 = 1000000 raw units
uint64_t amount = 1000000;
ESP_ERROR_CHECK(espsol_tx_add_token_transfer_checked(
    tx,
    source_ata,          // Your token account
    usdc_mint,           // USDC mint
    dest_ata,            // Recipient's token account
    keypair.public_key,  // Your wallet (signer)
    amount,              // 1 USDC
    6                    // USDC has 6 decimals
));
```

#### espsol_tx_add_token_close_account

Close a token account and recover rent.

```c
esp_err_t espsol_tx_add_token_close_account(
    espsol_tx_handle_t tx,       // Transaction
    const uint8_t account[32],   // Token account to close
    const uint8_t dest[32],      // Rent destination
    const uint8_t owner[32]      // Account owner (must sign)
);
```

#### espsol_is_on_curve

Check if a public key is on the Ed25519 curve (not a PDA).

```c
bool espsol_is_on_curve(const uint8_t pubkey[32]);
```

---

## Examples

### Query Network Information

```c
#include "espsol.h"

void query_network(void) {
    espsol_rpc_handle_t rpc;
    ESP_ERROR_CHECK(espsol_rpc_init(&rpc, ESPSOL_DEVNET_RPC));
    
    // Get version
    char version[64];
    ESP_ERROR_CHECK(espsol_rpc_get_version(rpc, version, sizeof(version)));
    ESP_LOGI(TAG, "Solana version: %s", version);
    
    // Get current slot
    uint64_t slot;
    ESP_ERROR_CHECK(espsol_rpc_get_slot(rpc, &slot));
    ESP_LOGI(TAG, "Current slot: %llu", slot);
    
    // Get blockhash
    uint8_t blockhash[32];
    ESP_ERROR_CHECK(espsol_rpc_get_latest_blockhash(rpc, blockhash, NULL));
    
    char blockhash_b58[64];
    espsol_base58_encode(blockhash, 32, blockhash_b58, sizeof(blockhash_b58));
    ESP_LOGI(TAG, "Latest blockhash: %s", blockhash_b58);
    
    espsol_rpc_deinit(rpc);
}
```

### Generate and Store Keypair

```c
#include "espsol.h"
#include "nvs_flash.h"

void wallet_demo(void) {
    // Initialize NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    
    espsol_keypair_t keypair;
    bool exists = false;
    
    // Check if wallet already exists
    espsol_keypair_exists_in_nvs("wallet", &exists);
    
    if (exists) {
        // Load existing wallet
        ESP_ERROR_CHECK(espsol_keypair_load_from_nvs("wallet", &keypair));
        ESP_LOGI(TAG, "Loaded existing wallet");
    } else {
        // Generate new wallet
        ESP_ERROR_CHECK(espsol_keypair_generate(&keypair));
        ESP_ERROR_CHECK(espsol_keypair_save_to_nvs(&keypair, "wallet"));
        ESP_LOGI(TAG, "Generated and saved new wallet");
    }
    
    // Display address
    char address[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&keypair, address, sizeof(address));
    ESP_LOGI(TAG, "Wallet address: %s", address);
    
    // Sign a message
    const char *message = "Hello, Solana!";
    uint8_t signature[64];
    ESP_ERROR_CHECK(espsol_sign(
        (uint8_t*)message, strlen(message), 
        &keypair, signature
    ));
    ESP_LOGI(TAG, "Message signed successfully");
    
    // Clear sensitive data when done
    espsol_keypair_clear(&keypair);
}
```

### Create Wallet with Seed Phrase

```c
#include "espsol.h"

/**
 * Create a new wallet with seed phrase backup
 */
void create_wallet_with_mnemonic(void) {
    char mnemonic[ESPSOL_MNEMONIC_12_MAX_LEN];
    espsol_keypair_t wallet;
    
    // Generate new wallet with 12-word seed phrase
    ESP_ERROR_CHECK(espsol_keypair_generate_with_mnemonic(
        12, mnemonic, sizeof(mnemonic), &wallet
    ));
    
    // Display seed phrase for user to backup
    printf("\n");
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║         ⚠️  BACKUP YOUR SEED PHRASE ⚠️            ║\n");
    printf("║  Write these words down and store them safely!   ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    espsol_mnemonic_print(mnemonic, NULL);
    printf("╚══════════════════════════════════════════════════╝\n");
    
    // Show wallet address
    char address[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&wallet, address, sizeof(address));
    ESP_LOGI(TAG, "Wallet address: %s", address);
    
    // Clear mnemonic from memory after user confirms backup
    espsol_mnemonic_clear(mnemonic, sizeof(mnemonic));
    
    // Optionally save keypair to NVS for future use
    espsol_keypair_save_to_nvs(&wallet, "main_wallet");
    espsol_keypair_clear(&wallet);
}

/**
 * Recover wallet from existing seed phrase
 */
void recover_wallet_from_mnemonic(const char *user_mnemonic) {
    // Validate the mnemonic first
    esp_err_t err = espsol_mnemonic_validate(user_mnemonic);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Invalid seed phrase!");
        return;
    }
    
    // Recover the keypair
    espsol_keypair_t wallet;
    ESP_ERROR_CHECK(espsol_keypair_from_mnemonic(user_mnemonic, NULL, &wallet));
    
    char address[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&wallet, address, sizeof(address));
    ESP_LOGI(TAG, "Recovered wallet: %s", address);
    
    // Save to NVS
    espsol_keypair_save_to_nvs(&wallet, "main_wallet");
    espsol_keypair_clear(&wallet);
}

/**
 * Use passphrase for extra security (creates different wallet)
 */
void wallet_with_passphrase(void) {
    const char *mnemonic = "abandon ability able about above absent absorb abstract absurd abuse access accident";
    
    // Without passphrase
    espsol_keypair_t wallet1;
    espsol_keypair_from_mnemonic(mnemonic, NULL, &wallet1);
    
    // With passphrase - produces DIFFERENT wallet
    espsol_keypair_t wallet2;
    espsol_keypair_from_mnemonic(mnemonic, "my_secret_passphrase", &wallet2);
    
    char addr1[ESPSOL_ADDRESS_MAX_LEN], addr2[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&wallet1, addr1, sizeof(addr1));
    espsol_keypair_get_address(&wallet2, addr2, sizeof(addr2));
    
    ESP_LOGI(TAG, "Without passphrase: %s", addr1);
    ESP_LOGI(TAG, "With passphrase:    %s", addr2);  // Different address!
    
    espsol_keypair_clear(&wallet1);
    espsol_keypair_clear(&wallet2);
}
```

### Transfer SOL

```c
#include "espsol.h"

void transfer_sol(espsol_keypair_t *sender, const char *recipient_addr, double sol_amount) {
    espsol_rpc_handle_t rpc;
    ESP_ERROR_CHECK(espsol_rpc_init(&rpc, ESPSOL_DEVNET_RPC));
    
    // Get latest blockhash
    uint8_t blockhash[32];
    ESP_ERROR_CHECK(espsol_rpc_get_latest_blockhash(rpc, blockhash, NULL));
    
    // Decode recipient address
    uint8_t recipient[32];
    ESP_ERROR_CHECK(espsol_address_to_pubkey(recipient_addr, recipient));
    
    // Create transaction
    espsol_tx_handle_t tx;
    ESP_ERROR_CHECK(espsol_tx_create(&tx));
    
    // Configure
    ESP_ERROR_CHECK(espsol_tx_set_fee_payer(tx, sender->public_key));
    ESP_ERROR_CHECK(espsol_tx_set_recent_blockhash(tx, blockhash));
    
    // Add transfer
    uint64_t lamports = espsol_sol_to_lamports(sol_amount);
    ESP_ERROR_CHECK(espsol_tx_add_transfer(tx, sender->public_key, recipient, lamports));
    
    // Sign
    ESP_ERROR_CHECK(espsol_tx_sign(tx, sender));
    
    // Serialize
    char tx_base64[2048];
    ESP_ERROR_CHECK(espsol_tx_to_base64(tx, tx_base64, sizeof(tx_base64)));
    
    // Send
    char signature[ESPSOL_SIGNATURE_MAX_LEN];
    ESP_ERROR_CHECK(espsol_rpc_send_transaction(rpc, tx_base64, signature, sizeof(signature)));
    ESP_LOGI(TAG, "Transaction sent: %s", signature);
    
    // Wait for confirmation
    bool confirmed = false;
    espsol_rpc_confirm_transaction(rpc, signature, 30000, &confirmed);
    if (confirmed) {
        ESP_LOGI(TAG, "Transfer confirmed!");
    }
    
    // Cleanup
    espsol_tx_destroy(tx);
    espsol_rpc_deinit(rpc);
}
```

### Transfer SPL Tokens

```c
#include "espsol.h"

void transfer_token(
    espsol_keypair_t *sender,
    const char *recipient_addr,
    const char *mint_addr,
    uint64_t amount,
    uint8_t decimals
) {
    espsol_rpc_handle_t rpc;
    ESP_ERROR_CHECK(espsol_rpc_init(&rpc, ESPSOL_DEVNET_RPC));
    
    // Decode addresses
    uint8_t recipient[32], mint[32];
    espsol_address_to_pubkey(recipient_addr, recipient);
    espsol_address_to_pubkey(mint_addr, mint);
    
    // Derive token accounts
    uint8_t source_ata[32], dest_ata[32];
    espsol_token_get_ata_address(sender->public_key, mint, source_ata);
    espsol_token_get_ata_address(recipient, mint, dest_ata);
    
    // Get blockhash
    uint8_t blockhash[32];
    espsol_rpc_get_latest_blockhash(rpc, blockhash, NULL);
    
    // Create transaction
    espsol_tx_handle_t tx;
    espsol_tx_create(&tx);
    espsol_tx_set_fee_payer(tx, sender->public_key);
    espsol_tx_set_recent_blockhash(tx, blockhash);
    
    // Create recipient's ATA if needed (idempotent)
    espsol_tx_add_create_ata_idempotent(tx, sender->public_key, recipient, mint);
    
    // Add token transfer
    espsol_tx_add_token_transfer_checked(
        tx, source_ata, mint, dest_ata,
        sender->public_key, amount, decimals
    );
    
    // Sign and send
    espsol_tx_sign(tx, sender);
    
    char tx_base64[2048];
    espsol_tx_to_base64(tx, tx_base64, sizeof(tx_base64));
    
    char signature[ESPSOL_SIGNATURE_MAX_LEN];
    esp_err_t err = espsol_rpc_send_transaction(rpc, tx_base64, signature, sizeof(signature));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Token transfer sent: %s", signature);
    }
    
    espsol_tx_destroy(tx);
    espsol_rpc_deinit(rpc);
}
```

---

## Configuration

### Kconfig Options

Configure via `idf.py menuconfig` → Component config → ESPSOL:

| Option | Default | Description |
|--------|---------|-------------|
| `CONFIG_ESPSOL_DEFAULT_RPC_ENDPOINT` | devnet | Default RPC URL |
| `CONFIG_ESPSOL_RPC_TIMEOUT_MS` | 30000 | Request timeout |
| `CONFIG_ESPSOL_MAX_TX_SIZE` | 1232 | Max transaction size |
| `CONFIG_ESPSOL_USE_LIBSODIUM` | y | Use libsodium for crypto |
| `CONFIG_ESPSOL_SECURE_STORAGE` | y | Enable NVS storage |
| `CONFIG_ESPSOL_DEBUG_LOGGING` | n | Verbose logging |

### Commitment Levels

```c
typedef enum {
    ESPSOL_COMMITMENT_PROCESSED,  // Fastest, least reliable
    ESPSOL_COMMITMENT_CONFIRMED,  // Default - good balance
    ESPSOL_COMMITMENT_FINALIZED   // Slowest, most reliable
} espsol_commitment_t;
```

---

## Troubleshooting

### Common Issues

#### "RPC request failed"
- Check Wi-Fi connection
- Verify RPC endpoint is correct
- Try increasing timeout
- Check if rate limited (use retry with backoff)

#### "Keypair not initialized"
- Ensure `espsol_keypair_generate()` was called
- Check if `espsol_keypair_load_from_nvs()` succeeded

#### "Transaction build error"
- Set fee payer before adding instructions
- Set blockhash before signing
- Ensure all required signers have signed

#### "Buffer too small"
- Use larger buffer sizes
- Check return value for required size

#### "Insufficient funds"
- Use airdrop on devnet: `espsol_rpc_request_airdrop()`
- Check balance before transfer

### Debug Logging

Enable verbose logging in your code:

```c
esp_log_level_set("espsol", ESP_LOG_DEBUG);
esp_log_level_set("espsol_rpc", ESP_LOG_DEBUG);
esp_log_level_set("espsol_tx", ESP_LOG_DEBUG);
```

### Memory Usage Tips

1. **Use stack buffers** - Most ESPSOL operations work with stack-allocated buffers
2. **Clear keypairs** - Always call `espsol_keypair_clear()` when done
3. **Reuse RPC handle** - Create once, use multiple times
4. **Destroy transactions** - Always call `espsol_tx_destroy()` after use

---

## Support

- **GitHub Issues**: https://github.com/SkyRizzGames/espsol/issues
- **Documentation**: https://github.com/SkyRizzGames/espsol/docs

---

*Copyright © 2025 SkyRizz. Licensed under Apache 2.0.*
