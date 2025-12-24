# ESPSOL Mnemonic Wallet Example

This example demonstrates how to use BIP39 seed phrases (mnemonic) with ESPSOL to create and restore Solana wallets on ESP32.

## Features

- **Generate New Wallet** - Create a new 12 or 24 word seed phrase with keypair
- **Display Seed Phrase** - Show numbered words for easy backup
- **Restore Wallet** - Recover a wallet from an existing seed phrase
- **Passphrase Protection** - Add extra security with optional passphrase
- **Word Validation** - Verify words are valid BIP39 words
- **Checksum Validation** - Detect typos or invalid phrases

## Why Seed Phrases?

Seed phrases (also called "recovery phrases" or "mnemonic") are a human-readable way to backup a cryptocurrency wallet:

1. **Easy to Write Down** - 12 or 24 English words are easy to write on paper
2. **Universal Standard** - BIP39 is used by almost all wallets (Phantom, Solflare, Ledger, etc.)
3. **Secure Backup** - Paper can't be hacked like digital files
4. **Cross-Platform** - Same phrase works on any BIP39-compatible wallet

## Security Warning

⚠️ **NEVER SHARE YOUR SEED PHRASE!** ⚠️

- Anyone with your seed phrase has FULL ACCESS to your wallet
- Never store seed phrases in digital files, emails, or cloud storage
- Write it on paper and store in a safe place
- Consider using a metal backup for fire/water resistance
- Never type your real seed phrase on websites

## Building

```bash
cd /Users/riz/esp/projects/espsol/examples/mnemonic_wallet

# Source ESP-IDF environment
source ~/esp/v5.5.1/esp-idf/export.sh

# Set target and build
idf.py set-target esp32
idf.py build

# Flash and monitor
idf.py -p /dev/tty.usbserial-* flash monitor
```

## Expected Output

```
╔══════════════════════════════════════════════════════════╗
║            ESPSOL MNEMONIC WALLET EXAMPLE                 ║
╚══════════════════════════════════════════════════════════╝

========================================
   CREATE NEW WALLET WITH SEED PHRASE   
========================================

╔══════════════════════════════════════════════════════════╗
║          WRITE DOWN YOUR SEED PHRASE!                    ║
╠══════════════════════════════════════════════════════════╣
   1. word1      7. word7
   2. word2      8. word8
   3. word3      9. word9
   4. word4     10. word10
   5. word5     11. word11
   6. word6     12. word12
╚══════════════════════════════════════════════════════════╝

✓ Wallet Address: <base58 address>
✓ Wallet saved to NVS (key: main_wallet)
✓ Sensitive data cleared from memory

========================================
   RESTORE WALLET FROM SEED PHRASE      
========================================
Restoring wallet from seed phrase:
"abandon abandon abandon ... about"
✓ Mnemonic is valid
✓ Restored Wallet Address: <base58 address>
```

## API Usage

### Generate Wallet with Seed Phrase

```c
#include "espsol.h"

char mnemonic[ESPSOL_MNEMONIC_12_MAX_LEN];
espsol_keypair_t wallet;

// Generate new 12-word wallet
espsol_keypair_generate_with_mnemonic(
    ESPSOL_MNEMONIC_12_WORDS,
    mnemonic, sizeof(mnemonic),
    &wallet
);

// Display for backup
espsol_mnemonic_print(mnemonic, "YOUR SEED PHRASE");

// Get address
char address[ESPSOL_ADDRESS_MAX_LEN];
espsol_keypair_get_address(&wallet, address, sizeof(address));

// Clear sensitive data when done
espsol_mnemonic_clear(mnemonic, sizeof(mnemonic));
espsol_keypair_clear(&wallet);
```

### Restore Wallet from Seed Phrase

```c
const char *mnemonic = "abandon abandon abandon abandon abandon abandon "
                       "abandon abandon abandon abandon abandon about";

// Validate first
if (espsol_mnemonic_validate(mnemonic) == ESP_OK) {
    espsol_keypair_t wallet;
    espsol_keypair_from_mnemonic(mnemonic, NULL, &wallet);
    // Use wallet...
}
```

### With Passphrase (Extra Security)

```c
// Same mnemonic with passphrase creates different wallet
espsol_keypair_from_mnemonic(mnemonic, "my-secret-passphrase", &wallet);
```

## BIP39 Word List

ESPSOL uses the official BIP39 English word list with 2048 words. You can validate individual words:

```c
int index;
if (espsol_mnemonic_word_valid("abandon", &index)) {
    printf("Valid word at index %d\n", index);
}

// Get word by index
const char *word = espsol_mnemonic_get_word(0);  // "abandon"
```

## Compatibility

Wallets created with ESPSOL are compatible with:

- **Phantom** - Import seed phrase to recover wallet
- **Solflare** - Import seed phrase to recover wallet
- **Backpack** - Import seed phrase to recover wallet
- **Ledger** - Same seed phrase works on hardware wallet

Note: Uses standard Solana derivation (first 32 bytes of BIP39 seed as Ed25519 seed).

## Memory Considerations

- 12-word mnemonic buffer: 128 bytes
- 24-word mnemonic buffer: 256 bytes
- BIP39 wordlist: ~16KB in flash (read-only)
- Seed derivation uses PBKDF2 with 2048 iterations

## Related Examples

- `keypair_demo` - Basic keypair generation without seed phrase
- `wallet_manager` - Full wallet management with NVS persistence
