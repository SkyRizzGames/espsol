# ESPSOL Quickstart Example

This is the recommended starting point for new ESPSOL projects. It demonstrates all core features of the SDK in a single, well-commented example.

## What This Example Demonstrates

1. **SDK Initialization** - Initialize ESPSOL with default configuration
2. **Keypair Management** - Generate, save, and load Ed25519 keypairs
3. **Encoding Utilities** - Base58/Base64 encoding and address validation
4. **Cryptographic Operations** - Message signing and signature verification
5. **RPC Client Operations** - Query network version, slot, balance, blockhash
6. **Transaction Building** - Create, sign, and serialize transactions
7. **SPL Token Operations** - Derive Associated Token Accounts (if enabled)

## Prerequisites

- ESP-IDF v5.0 or later
- ESP32, ESP32-S2, ESP32-S3, ESP32-C3, or ESP32-C6
- WiFi connection for RPC operations
- libsodium component (automatically managed)

## Hardware Requirements

- Any ESP32 development board with WiFi
- USB cable for programming and monitoring

## Configuration

### WiFi Settings

Edit `sdkconfig.defaults` or use `idf.py menuconfig`:

```
Component config → Example Connection Configuration
  - WiFi SSID: Your network name
  - WiFi Password: Your network password
```

### ESPSOL Settings

The example uses default ESPSOL configuration suitable for development:

- **RPC Endpoint**: Devnet (`https://api.devnet.solana.com`)
- **Commitment**: Confirmed (balanced speed/security)
- **Timeout**: 30 seconds (reliable for devnet)
- **NVS Encryption**: Enabled (secure key storage)
- **SPL Tokens**: Enabled (token operations)

To customize, run `idf.py menuconfig` and navigate to:
```
Component config → ESPSOL Solana Component
```

See [docs/kconfig-guide.md](../../../docs/kconfig-guide.md) for detailed configuration options.

## Build and Run

### 1. Set ESP-IDF Target

```bash
cd examples/quickstart
idf.py set-target esp32c3  # or esp32, esp32s3, etc.
```

### 2. Configure WiFi

```bash
idf.py menuconfig
# Navigate to: Example Connection Configuration
# Set your WiFi SSID and password
```

### 3. Build

```bash
idf.py build
```

### 4. Flash and Monitor

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Press `Ctrl+]` to exit monitor.

## Expected Output

```
========================================
 ESPSOL SDK Quickstart Example
========================================

[Demo 1: SDK Initialization]
✓ SDK initialized successfully
SDK is ready for Solana operations!

[Demo 2: Keypair Management]
✓ Keypair generated successfully
Public Key (Base58): 5Xg8QF...7mBz2
✓ Keypair saved to NVS
✓ Keypair loaded from NVS
✓ Loaded keypair matches original

[Demo 3: Encoding Utilities]
✓ Base58 encode: 5Xg8QF...7mBz2
✓ Base58 decode: 32 bytes
✓ Base58 round-trip successful
✓ Base64 encode: SGVsbG8gU29sYW5hIQ==
✓ Base64 decode: Hello Solana!

[Demo 4: Cryptographic Operations]
✓ Message signed successfully
Signature (hex): a3f8e1c4...9d2b7a6f
✓ Signature verified successfully!
✓ Invalid signature correctly detected

[Demo 5: RPC Client Operations]
✓ RPC client initialized
✓ Network version: 1.18.0
✓ Current slot: 285123456
✓ Balance: 0 lamports (0.000000000 SOL)
💡 Tip: Get free devnet SOL at https://faucet.solana.com
✓ Latest blockhash: 4sGjMW1s...ZHirAn

[Demo 6: Transaction Building]
✓ Transaction created
✓ Fee payer set
✓ Blockhash set
✓ Transfer instruction added (0.001 SOL)
✓ Transaction signed
✓ Transaction serialized to Base64

[Demo 7: SPL Token Operations]
✓ ATA Address: 7aK9Tx...Qp5Ym
✓ Token operations demo complete

========================================
 Quickstart Complete!
========================================

🎉 All demos completed successfully!

Next steps:
1. Get devnet SOL from https://faucet.solana.com
2. Modify this example for your use case
3. See other examples in examples/ directory
4. Read documentation at docs/

Happy building on Solana + ESP32! 🚀
```

## Next Steps

### Get Devnet SOL

1. Note your wallet address from the output
2. Visit https://faucet.solana.com
3. Request devnet SOL (free)
4. Run the example again - your balance will update!

### Modify the Example

The code is heavily commented and easy to customize:

- **Change RPC endpoint**: Modify `RPC_ENDPOINT` for mainnet/testnet
- **Add more operations**: See other examples in `examples/` directory
- **Integrate with your app**: Copy relevant functions to your project

### Explore Other Examples

- `basic_rpc/` - Simple RPC queries without transactions
- `wallet_demo/` - Advanced key management and NVS usage
- `transfer_sol/` - Complete SOL transfer with confirmation
- `token_operations/` - SPL token transfers and ATAs

### Read Documentation

- [API Reference](../../../docs/api-reference.md) - Complete function documentation
- [Kconfig Guide](../../../docs/kconfig-guide.md) - Configuration options
- [Grant Plan](../../../docs/plan-grants.md) - Future features roadmap

## Troubleshooting

### WiFi Connection Failed

```
✗ WiFi connection failed
```

**Solution**: Check WiFi credentials in menuconfig

### RPC Operations Fail

```
⚠ Get balance failed: ESP_ERR_ESPSOL_TIMEOUT
```

**Solutions**:
- Check internet connection
- Increase timeout in Kconfig (`ESPSOL_RPC_TIMEOUT_MS`)
- Try different RPC endpoint

### NVS Save/Load Failed

```
⚠ Keypair save failed: ESP_ERR_NVS_NOT_FOUND
```

**Solution**: NVS is optional. The demo continues without it.

### Compilation Errors

```
fatal error: espsol.h: No such file or directory
```

**Solution**: Ensure you're building from the correct directory and ESPSOL component is in `../../../components/`

## Advanced Usage

### Use Mainnet

Change RPC endpoint:
```c
#define RPC_ENDPOINT ESPSOL_MAINNET_RPC
```

⚠️ **Warning**: Use a paid RPC provider for production to avoid rate limiting.

### Enable More Features

Edit `sdkconfig.defaults`:
```
CONFIG_ESPSOL_ENABLE_WEBSOCKET=y
CONFIG_ESPSOL_ENABLE_VERSIONED_TX=y
CONFIG_ESPSOL_ENABLE_TOKEN_2022=y
```

Note: Some features are placeholders for future milestones (see plan-grants.md).

### Production Configuration

For production deployments, see configuration templates in [docs/kconfig-guide.md](../../../docs/kconfig-guide.md):
- Payment terminal configuration
- DeFi integration configuration
- Hardware wallet configuration
- Minimal memory configuration

## Performance Notes

- **Signing time**: < 30ms (Ed25519)
- **RPC latency**: 500ms - 2s (network dependent)
- **Memory usage**: ~12KB RAM (with default config)
- **Flash usage**: ~860KB (with libsodium)

## Security Notes

- Keypairs are stored in NVS with encryption enabled
- Private keys never leave the device
- Demo wallet key is for testing only - **DO NOT USE IN PRODUCTION**
- For production, implement proper key management

## Support

- GitHub Issues: https://github.com/ariestiyansyah/espsol/issues
- Documentation: https://github.com/ariestiyansyah/espsol/tree/main/docs

## License

Apache 2.0 - See LICENSE file for details.

Copyright (c) 2025 SkyRizz
