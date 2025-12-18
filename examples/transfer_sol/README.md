# Transfer SOL Example

This example demonstrates a complete SOL transfer workflow on Solana devnet.

## What It Does

1. Creates two wallets (sender and receiver)
2. Requests 1 SOL airdrop to sender (devnet only)
3. Creates a transfer transaction (0.1 SOL)
4. Signs the transaction with sender's keypair
5. Sends the transaction to Solana
6. Waits for confirmation
7. Verifies final balances

## Setup

1. Configure WiFi in `main.c`:
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
I (xxx) transfer_sol: ╔════════════════════════════════════════╗
I (xxx) transfer_sol: ║    ESPSOL Transfer SOL Example         ║
I (xxx) transfer_sol: ╚════════════════════════════════════════╝
I (xxx) transfer_sol: Connecting to WiFi...
I (xxx) transfer_sol: WiFi connected
I (xxx) transfer_sol: 
I (xxx) transfer_sol: === Step 1: Create Wallets ===
I (xxx) transfer_sol: Sender:   7xKXtg2CW87d97TXJSDpbD5jBkheTqA83TZRuJosgAsU
I (xxx) transfer_sol: Receiver: Hh8kQN9FLdC1Xp4kGzZYPnWE2N7nXJFZ6YmJyW8Kt7xK
I (xxx) transfer_sol: 
I (xxx) transfer_sol: === Step 2: Request Airdrop (1 SOL) ===
I (xxx) transfer_sol: Airdrop signature: 5ZZ...
I (xxx) transfer_sol: Waiting for confirmation...
I (xxx) transfer_sol: ✓ Airdrop confirmed!
I (xxx) transfer_sol: Sender balance: 1.000000000 SOL
I (xxx) transfer_sol: 
I (xxx) transfer_sol: === Step 3: Create Transfer Transaction ===
I (xxx) transfer_sol: Transferring 0.10 SOL (100000000 lamports)
I (xxx) transfer_sol: Transaction created with 1 instruction(s)
I (xxx) transfer_sol: 
I (xxx) transfer_sol: === Step 4: Sign Transaction ===
I (xxx) transfer_sol: Signature: 3xP7B2...
I (xxx) transfer_sol: 
I (xxx) transfer_sol: === Step 5: Send Transaction ===
I (xxx) transfer_sol: Transaction sent!
I (xxx) transfer_sol: Signature: 5wwKuh327t65M8Z9z1zMhDEQW6cqi6ns...
I (xxx) transfer_sol: 
I (xxx) transfer_sol: === Step 6: Wait for Confirmation ===
I (xxx) transfer_sol: ✓ Transaction confirmed!
I (xxx) transfer_sol: 
I (xxx) transfer_sol: === Step 7: Verify Final Balances ===
I (xxx) transfer_sol: Sender:   0.899995000 SOL (was 1.000000000)
I (xxx) transfer_sol: Receiver: 0.100000000 SOL
I (xxx) transfer_sol: 
I (xxx) transfer_sol: === Solana Explorer ===
I (xxx) transfer_sol: View transaction:
I (xxx) transfer_sol:   https://explorer.solana.com/tx/5wwKuh...?cluster=devnet
I (xxx) transfer_sol: 
I (xxx) transfer_sol: ╔════════════════════════════════════════╗
I (xxx) transfer_sol: ║       🎉 Transfer Complete! 🎉         ║
I (xxx) transfer_sol: ╚════════════════════════════════════════╝
```

## Notes

- The sender pays a small transaction fee (~0.000005 SOL)
- Devnet airdrops may be rate-limited during high usage
- This example uses devnet - no real SOL is involved
