# Wallet Demo

This example demonstrates ESPSOL wallet management features.

## Features

- Generate random Ed25519 keypairs
- Export/import keypairs as Base58
- Sign and verify messages
- Generate deterministic keypairs from seed
- Store keypairs securely in NVS
- Load and delete keypairs from NVS

## Security Notes

⚠️ **Important**: In production:
- Never log private keys
- Use NVS encryption for secure storage
- Clear keypairs from memory when done
- Consider hardware security modules for high-value applications

## Build

```bash
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Expected Output

```
I (xxx) wallet_demo: ESPSOL Wallet Demo
I (xxx) wallet_demo: ==================
I (xxx) wallet_demo: 
I (xxx) wallet_demo: --- Generate New Keypair ---
I (xxx) wallet_demo: New wallet address: 7xKXtg2CW87d97TXJSDpbD5jBkheTqA83TZRuJosgAsU
I (xxx) wallet_demo: Private key (keep secret!): 4FMC...
I (xxx) wallet_demo: 
I (xxx) wallet_demo: --- Sign Message ---
I (xxx) wallet_demo: Message: "Hello, Solana from ESP32!"
I (xxx) wallet_demo: Signature: 5ZZ...
I (xxx) wallet_demo: 
I (xxx) wallet_demo: --- Verify Signature ---
I (xxx) wallet_demo: Signature valid: YES
I (xxx) wallet_demo: Wrong message verification: INVALID (expected)
I (xxx) wallet_demo: 
I (xxx) wallet_demo: --- Save to NVS ---
I (xxx) wallet_demo: Wallet saved to NVS key: my_wallet
I (xxx) wallet_demo: Original keypair cleared from memory
I (xxx) wallet_demo: 
I (xxx) wallet_demo: --- Load from NVS ---
I (xxx) wallet_demo: Loaded wallet address: 7xKXtg2CW87d97TXJSDpbD5jBkheTqA83TZRuJosgAsU
I (xxx) wallet_demo: Addresses match: YES
I (xxx) wallet_demo: Signatures match: YES
```
