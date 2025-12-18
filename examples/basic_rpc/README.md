# Basic RPC Example

This example demonstrates how to use ESPSOL to query Solana RPC endpoints.

## Features

- Connect to Solana devnet
- Get Solana node version
- Get current slot and block height
- Check node health
- Query account balance
- Get latest blockhash

## Setup

1. Configure your WiFi credentials in `main.c`:
   ```c
   #define WIFI_SSID      "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD  "YOUR_WIFI_PASSWORD"
   ```

2. Build and flash:
   ```bash
   idf.py set-target esp32c3
   idf.py build
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

## Expected Output

```
I (xxx) basic_rpc: ESPSOL Basic RPC Example
I (xxx) basic_rpc: ========================
I (xxx) basic_rpc: Connecting to WiFi...
I (xxx) basic_rpc: WiFi connected!
I (xxx) basic_rpc: 
I (xxx) basic_rpc: --- Solana Version ---
I (xxx) basic_rpc: Solana version: 1.18.x
I (xxx) basic_rpc: 
I (xxx) basic_rpc: --- Current Slot ---
I (xxx) basic_rpc: Current slot: 123456789
I (xxx) basic_rpc: 
I (xxx) basic_rpc: --- Block Height ---
I (xxx) basic_rpc: Block height: 123456000
I (xxx) basic_rpc: 
I (xxx) basic_rpc: --- Node Health ---
I (xxx) basic_rpc: Node healthy: YES
I (xxx) basic_rpc: 
I (xxx) basic_rpc: --- Account Balance ---
I (xxx) basic_rpc: Address: So11111111111111111111111111111111111111112
I (xxx) basic_rpc: Balance: 0.000000000 SOL (0 lamports)
I (xxx) basic_rpc: 
I (xxx) basic_rpc: --- Latest Blockhash ---
I (xxx) basic_rpc: Blockhash: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
I (xxx) basic_rpc: Valid until block: 123456800
I (xxx) basic_rpc: ========================
I (xxx) basic_rpc: Example complete!
```
