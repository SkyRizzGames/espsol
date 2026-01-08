# WebSocket Subscription Example

This example demonstrates how to use ESPSOL's WebSocket client to subscribe to real-time Solana blockchain events.

## Features Demonstrated

- WebSocket connection to Solana RPC
- Account state change subscriptions
- Transaction log subscriptions  
- Slot notification subscriptions
- Event callback handling
- Connection management and auto-reconnect

## What This Example Does

1. Connects to WiFi
2. Initializes WebSocket client to Solana devnet
3. Subscribes to:
   - Wrapped SOL token mint account changes
   - All transaction logs on the network
   - Slot notifications (new blocks)
4. Prints notifications as they arrive in real-time
5. Runs for 60 seconds then unsubscribes and exits

## Prerequisites

- ESP32/ESP32-C3/ESP32-S3 board
- WiFi network connection
- Solana devnet RPC endpoint (default: `wss://api.devnet.solana.com`)

## Configuration

Before building, configure your WiFi credentials:

```bash
idf.py menuconfig
```

Navigate to: `Example Configuration`

Set:
- `WiFi SSID`: Your WiFi network name
- `WiFi Password`: Your WiFi password

You can also configure WebSocket settings in:
`Component config → ESPSOL Solana Component → Network Configuration`

## Building and Running

```bash
# Set target (ESP32-C3 example)
idf.py set-target esp32c3

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

## Expected Output

```
I (1234) websocket_sub: ╔════════════════════════════════════════╗
I (1234) websocket_sub: ║  ESPSOL WebSocket Subscription Demo   ║
I (1234) websocket_sub: ╚════════════════════════════════════════╝
I (1234) websocket_sub: 
I (1234) websocket_sub: 📡 Connecting to WiFi...
I (2345) websocket_sub: ✓ WiFi connected
I (2345) websocket_sub: 🔌 Initializing WebSocket client...
I (2456) websocket_sub: ✓ WebSocket connected!
I (2456) websocket_sub: 📝 Subscribing to account: So11111111111111111111111111111111111111112
I (2567) websocket_sub: ✓ Account subscription ID: 1
I (2567) websocket_sub: 📝 Subscribing to transaction logs (all)
I (2678) websocket_sub: ✓ Logs subscription ID: 2
I (2678) websocket_sub: 📝 Subscribing to slot notifications
I (2789) websocket_sub: ✓ Slot subscription ID: 3
I (2789) websocket_sub: 🎧 Listening for notifications...
I (3000) websocket_sub: 📬 Received notification (type=4, sub_id=3)
I (3000) websocket_sub:   Slot Update:
I (3000) websocket_sub:     Current: 123456789
I (3000) websocket_sub:     Parent: 123456788
I (3000) websocket_sub:     Root: 123456700
...
```

## Subscription Types Explained

### Account Subscribe
Monitors an account for:
- Lamport balance changes
- Data changes
- Owner changes
- Executable flag changes

**Use Cases:**
- Watch wallet balances
- Monitor token accounts
- Track program data accounts

### Logs Subscribe
Receives transaction logs for:
- All transactions (`"all"`)
- Transactions mentioning a specific address
- Specific program invocations

**Use Cases:**
- Transaction monitoring
- DApp event tracking
- Error detection

### Slot Subscribe
Notifies on new slots:
- Current slot number
- Parent slot
- Root slot (finalized)

**Use Cases:**
- Block production monitoring
- Network health tracking
- Time-based triggers

### Signature Subscribe (Not shown)
Tracks transaction confirmation:
- Transaction status
- Confirmation level
- Error messages

**Use Cases:**
- Transaction confirmation tracking
- Payment verification
- Transaction retries

## Customization

### Monitor Your Own Account

Change `WRAPPED_SOL_MINT` to your account address:

```c
#define MY_ACCOUNT "YourBase58AddressHere..."
ret = espsol_ws_account_subscribe(ws_handle, MY_ACCOUNT, &account_sub_id);
```

### Filter Logs by Program

Subscribe to logs mentioning a specific address:

```c
#define MY_PROGRAM "YourProgramIDHere..."
ret = espsol_ws_logs_subscribe(ws_handle, MY_PROGRAM, &logs_sub_id);
```

### Run Forever

Remove the 60-second timer and run indefinitely:

```c
while (1) {
    vTaskDelay(pdMS_TO_TICKS(10000));
    // Your application logic here
}
```

## WebSocket Auto-Reconnect

The WebSocket client automatically reconnects if the connection drops. Configure in menuconfig:

- `ESPSOL_WS_AUTO_RECONNECT`: Enable/disable (default: enabled)
- `ESPSOL_WS_RECONNECT_DELAY_MS`: Delay before reconnect (default: 5000ms)

## Performance Notes

- Each subscription uses ~64 bytes RAM
- WebSocket client uses ~12KB RAM total
- Slot notifications arrive ~every 400ms on devnet
- Transaction logs frequency depends on network activity

## Troubleshooting

### "WebSocket connection timeout"
- Check WiFi connection
- Verify RPC endpoint in menuconfig
- Try increasing timeout: `ESPSOL_RPC_TIMEOUT_MS`

### "Failed to subscribe"
- WebSocket must be connected first
- Check network connectivity
- Verify account address is valid Base58

### No notifications received
- Account may not be changing (normal for some accounts)
- Logs subscription may be too specific
- Wait longer for activity

## Related Examples

- `basic_rpc/` - HTTP RPC queries
- `transfer_sol/` - Send SOL transfers
- `token_operations/` - SPL token operations

## Documentation

Full API documentation: [docs/api-reference.md](../../docs/api-reference.md)

WebSocket API: [espsol_ws.h](../../components/espsol/include/espsol_ws.h)

## License

Apache-2.0
