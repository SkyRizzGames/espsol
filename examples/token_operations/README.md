# Token Operations Example

This example demonstrates SPL Token operations with ESPSOL.

## Features

- Query token accounts by owner
- Check token balances and decimals
- Build token instructions:
  - InitializeMint
  - Transfer
  - MintTo
  - Burn
  - CloseAccount
- Derive Associated Token Accounts (ATA)

## SPL Token Instruction Types

| Instruction | Description |
|------------|-------------|
| `InitializeMint` | Create a new token mint |
| `InitializeAccount` | Create a token account |
| `Transfer` | Transfer tokens between accounts |
| `MintTo` | Mint new tokens (requires mint authority) |
| `Burn` | Burn tokens (remove from circulation) |
| `CloseAccount` | Close a token account and recover rent |
| `SetAuthority` | Change mint/freeze authority |
| `Approve` / `Revoke` | Delegate token spending |

## Associated Token Accounts (ATA)

ATAs are deterministic token accounts derived from:
- Owner's public key
- Token mint address
- Token Program ID
- ATA Program ID

```c
uint8_t ata[32];
espsol_derive_ata(owner_pubkey, mint_pubkey, ata);
```

## Build

```bash
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Notes

- Token operations require SOL for rent
- Minting requires mint authority
- Transfers require token account ownership
- This example builds instructions but doesn't execute them
  (new wallets don't have tokens to transfer)
