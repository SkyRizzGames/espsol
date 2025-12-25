# ESPSOL - ESP-IDF Solana Component

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.0+-blue.svg)](https://docs.espressif.com/projects/esp-idf/)
[![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](LICENSE)
[![Solana](https://img.shields.io/badge/Solana-Devnet%20%7C%20Mainnet-purple.svg)](https://solana.com)

A production-grade ESP-IDF component for Solana blockchain integration on ESP32 devices. Written in pure C with native ESP-IDF APIs.

## ✨ Features

- **Pure C Implementation** - No Arduino, no C++ overhead
- **Native ESP-IDF** - Uses esp_http_client, cJSON, mbedTLS
- **Ed25519 Cryptography** - Via libsodium (hardware-accelerated where available)
- **BIP39 Seed Phrases** - 12/24-word mnemonic backup and recovery
- **Full RPC Support** - All common Solana JSON-RPC 2.0 methods
- **Transaction Building** - System Program transfers, SPL Token operations
- **Secure Key Storage** - NVS with optional encryption
- **Minimal Footprint** - Component is only ~8KB compiled

## 📦 Installation

### Option 1: As a Local Component

```bash
cd your_project
mkdir -p components
git clone https://github.com/SkyRizzGames/espsol.git components/espsol
```

### Option 2: Via ESP Component Registry (coming soon)

```bash
idf.py add-dependency "espsol"
```

### Dependencies

Add to your project's `idf_component.yml`:

```yaml
dependencies:
  espressif/libsodium: "^1.0.20"
  idf: ">=5.0"
```

## 🚀 Quick Start

### Basic Example

```c
#include "espsol.h"

void app_main(void)
{
    // Initialize ESPSOL with devnet
    espsol_config_t config = ESPSOL_CONFIG_DEFAULT();
    config.rpc_url = ESPSOL_DEVNET_RPC;
    ESP_ERROR_CHECK(espsol_init(&config));
    
    // Create RPC client
    espsol_rpc_handle_t rpc;
    ESP_ERROR_CHECK(espsol_rpc_init(&rpc, config.rpc_url));
    
    // Generate a new wallet
    espsol_keypair_t wallet;
    ESP_ERROR_CHECK(espsol_keypair_generate(&wallet));
    
    // Get wallet address
    char address[ESPSOL_ADDRESS_MAX_LEN];
    espsol_keypair_get_address(&wallet, address, sizeof(address));
    printf("Wallet: %s\n", address);
    
    // Check balance
    uint64_t balance;
    espsol_rpc_get_balance(rpc, address, &balance);
    printf("Balance: %.9f SOL\n", espsol_lamports_to_sol(balance));
    
    // Cleanup
    espsol_keypair_clear(&wallet);
    espsol_rpc_deinit(rpc);
    espsol_deinit();
}
```

### Transfer SOL

```c
// Create transaction
espsol_transaction_t tx;
espsol_tx_init(&tx);

// Get blockhash
uint8_t blockhash[32];
uint64_t last_valid;
espsol_rpc_get_latest_blockhash(rpc, blockhash, &last_valid);

// Build transfer
espsol_tx_set_fee_payer(&tx, sender.public_key);
espsol_tx_set_recent_blockhash(&tx, blockhash);
espsol_tx_add_transfer(&tx, sender.public_key, receiver.public_key, 
                       espsol_sol_to_lamports(0.1));

// Sign and send
espsol_tx_sign(&tx, &sender);

char tx_base64[2048];
espsol_tx_to_base64(&tx, tx_base64, sizeof(tx_base64));

char signature[ESPSOL_SIGNATURE_MAX_LEN];
espsol_rpc_send_transaction(rpc, tx_base64, signature, sizeof(signature));
```

### Create Wallet with Seed Phrase (BIP39)

```c
// Generate new wallet with 12-word seed phrase backup
char mnemonic[ESPSOL_MNEMONIC_12_MAX_LEN];
espsol_keypair_t wallet;
espsol_keypair_generate_with_mnemonic(12, mnemonic, sizeof(mnemonic), &wallet);

// Display seed phrase for user to backup
printf("⚠️ BACKUP YOUR SEED PHRASE:\n%s\n", mnemonic);

// Later: recover wallet from seed phrase
espsol_keypair_from_mnemonic(mnemonic, NULL, &wallet);

// Clear sensitive data
espsol_mnemonic_clear(mnemonic, sizeof(mnemonic));
espsol_keypair_clear(&wallet);
```

## 📚 API Reference

### Core Module (`espsol.h`)

| Function | Description |
|----------|-------------|
| `espsol_init()` | Initialize SDK with configuration |
| `espsol_deinit()` | Cleanup and release resources |
| `espsol_get_version()` | Get SDK version string |

### Mnemonic Module (`espsol_mnemonic.h`)

| Function | Description |
|----------|-------------|
| `espsol_mnemonic_generate_12()` | Generate 12-word seed phrase |
| `espsol_mnemonic_generate_24()` | Generate 24-word seed phrase |
| `espsol_mnemonic_validate()` | Validate mnemonic phrase |
| `espsol_mnemonic_to_seed()` | Convert mnemonic to 64-byte seed |
| `espsol_keypair_from_mnemonic()` | Derive keypair from mnemonic |
| `espsol_keypair_generate_with_mnemonic()` | Generate keypair + mnemonic |
| `espsol_mnemonic_print()` | Print numbered words for backup |
| `espsol_mnemonic_clear()` | Securely clear mnemonic from memory |

### RPC Client (`espsol_rpc.h`)

| Function | Description |
|----------|-------------|
| `espsol_rpc_init()` | Create RPC client |
| `espsol_rpc_deinit()` | Destroy RPC client |
| `espsol_rpc_get_balance()` | Get account balance in lamports |
| `espsol_rpc_get_slot()` | Get current slot |
| `espsol_rpc_get_block_height()` | Get block height |
| `espsol_rpc_get_version()` | Get Solana node version |
| `espsol_rpc_get_health()` | Check node health |
| `espsol_rpc_get_latest_blockhash()` | Get recent blockhash |
| `espsol_rpc_request_airdrop()` | Request devnet airdrop |
| `espsol_rpc_send_transaction()` | Submit signed transaction |
| `espsol_rpc_confirm_transaction()` | Wait for confirmation |
| `espsol_rpc_get_account_info()` | Get account details |
| `espsol_rpc_get_token_accounts_by_owner()` | List token accounts |
| `espsol_rpc_get_token_balance()` | Get SPL token balance |

### Crypto Module (`espsol_crypto.h`)

| Function | Description |
|----------|-------------|
| `espsol_crypto_init()` | Initialize crypto subsystem |
| `espsol_keypair_generate()` | Generate random keypair |
| `espsol_keypair_from_seed()` | Derive keypair from seed |
| `espsol_keypair_from_base58()` | Import from Base58 private key |
| `espsol_keypair_get_address()` | Get Base58 address |
| `espsol_keypair_to_base58()` | Export private key |
| `espsol_keypair_clear()` | Securely clear keypair |
| `espsol_sign()` | Sign message |
| `espsol_verify()` | Verify signature |
| `espsol_keypair_save_to_nvs()` | Store keypair in NVS |
| `espsol_keypair_load_from_nvs()` | Load keypair from NVS |
| `espsol_keypair_delete_from_nvs()` | Delete from NVS |

### Transaction Module (`espsol_tx.h`)

| Function | Description |
|----------|-------------|
| `espsol_tx_init()` | Initialize transaction |
| `espsol_tx_set_fee_payer()` | Set fee payer |
| `espsol_tx_set_recent_blockhash()` | Set blockhash |
| `espsol_tx_add_transfer()` | Add SOL transfer |
| `espsol_tx_add_instruction()` | Add custom instruction |
| `espsol_tx_sign()` | Sign transaction |
| `espsol_tx_to_base64()` | Serialize to Base64 |

### Token Module (`espsol_token.h`)

| Function | Description |
|----------|-------------|
| `espsol_tx_add_token_transfer()` | SPL token transfer |
| `espsol_tx_add_token_mint_to()` | Mint tokens |
| `espsol_tx_add_token_burn()` | Burn tokens |
| `espsol_tx_add_token_initialize_mint()` | Create mint |
| `espsol_tx_add_token_initialize_account()` | Create token account |
| `espsol_tx_add_create_ata()` | Create ATA |
| `espsol_derive_ata()` | Derive ATA address |

### Utilities (`espsol_utils.h`)

| Function | Description |
|----------|-------------|
| `espsol_base58_encode()` | Encode bytes to Base58 |
| `espsol_base58_decode()` | Decode Base58 to bytes |
| `espsol_base64_encode()` | Encode bytes to Base64 |
| `espsol_base64_decode()` | Decode Base64 to bytes |
| `espsol_pubkey_to_address()` | Convert pubkey to address |
| `espsol_lamports_to_sol()` | Convert lamports to SOL |
| `espsol_sol_to_lamports()` | Convert SOL to lamports |

## 🔧 Configuration

### Kconfig Options

Configure via `idf.py menuconfig` → ESPSOL:

| Option | Default | Description |
|--------|---------|-------------|
| `ESPSOL_DEFAULT_RPC_ENDPOINT` | devnet | Default RPC URL |
| `ESPSOL_RPC_TIMEOUT_MS` | 30000 | Request timeout |
| `ESPSOL_MAX_TX_SIZE` | 1232 | Max transaction size |
| `ESPSOL_USE_LIBSODIUM` | y | Use libsodium for crypto |
| `ESPSOL_SECURE_STORAGE` | y | Enable NVS encryption |
| `ESPSOL_DEBUG_LOGGING` | n | Verbose logging |

### Recommended sdkconfig.defaults

```ini
# Stack size for crypto operations
CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384

# Size optimizations
CONFIG_COMPILER_OPTIMIZATION_SIZE=y
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN=y
CONFIG_MBEDTLS_TLS_CLIENT_ONLY=y
CONFIG_LIBC_NEWLIB_NANO_FORMAT=y
```

## 📁 Examples

| Example | Description |
|---------|-------------|
| [basic_rpc](examples/basic_rpc/) | Query Solana network information |
| [wallet_demo](examples/wallet_demo/) | Keypair management and NVS storage |
| [mnemonic_wallet](examples/mnemonic_wallet/) | Seed phrase backup and wallet recovery |
| [transfer_sol](examples/transfer_sol/) | Complete SOL transfer workflow |
| [token_operations](examples/token_operations/) | SPL Token operations |

## 📊 Resource Usage

| Resource | Usage |
|----------|-------|
| **Flash (ESPSOL only)** | ~8 KB |
| **Flash (with deps)** | ~860 KB |
| **RAM (static)** | ~21 bytes |
| **RAM (per RPC client)** | ~8 KB |
| **Stack (signing)** | ~8 KB recommended |

## 🔐 Security Considerations

1. **NVS Encryption**: Enable `CONFIG_NVS_ENCRYPTION` for secure key storage
2. **Clear Keys**: Always call `espsol_keypair_clear()` when done
3. **Clear Mnemonics**: Always call `espsol_mnemonic_clear()` after backup
4. **TLS Verification**: Uses ESP certificate bundle for HTTPS
5. **Hardware RNG**: Uses ESP32's true random number generator
6. **No Logging Keys**: Private keys and mnemonics are never logged

## 🧪 Testing

### Host-based Unit Tests

```bash
cd test/host
./run_tests.sh
```

### Device Integration Tests

```bash
cd test
idf.py set-target esp32c3
idf.py build flash monitor
```

## 📄 License

Apache License 2.0 - see [LICENSE](LICENSE) for details.

## 🙏 Acknowledgments

- [Solduino](https://github.com/torrey-xyz/solduino) - Arduino Solana SDK inspiration
- [libsodium](https://libsodium.org) - Ed25519 cryptography
- [Solana](https://solana.com) - The blockchain platform

## 📞 Support

- **Issues**: GitHub Issues
- **Discussions**: GitHub Discussions
- **Solana Docs**: https://docs.solana.com
