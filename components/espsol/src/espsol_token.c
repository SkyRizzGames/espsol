/**
 * @file espsol_token.c
 * @brief ESPSOL SPL Token Implementation
 *
 * Implementation of SPL Token operations including:
 * - Associated Token Account (ATA) derivation
 * - Token transfers
 * - Account creation
 * - Mint/Burn operations
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include "espsol_token.h"
#include "espsol_tx.h"
#include "espsol_crypto.h"
#include <string.h>

#if defined(ESP_PLATFORM) && ESP_PLATFORM
#include "esp_log.h"
#include "sodium.h"
static const char *TAG = "espsol_token";
#else
/* Host testing - use portable implementation */
#include <stdio.h>
#define ESP_LOGI(tag, ...)
#define ESP_LOGE(tag, ...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
/* SHA256 from portable implementation */
extern void espsol_sha256(const uint8_t *data, size_t len, uint8_t out[32]);
#endif

/* ============================================================================
 * Well-Known Addresses
 * ========================================================================== */

/**
 * @brief Native (Wrapped) SOL Mint address
 * So11111111111111111111111111111111111111112
 */
static const uint8_t NATIVE_MINT[ESPSOL_PUBKEY_SIZE] = {
    0x06, 0x9b, 0x88, 0x57, 0xfe, 0xab, 0x81, 0x84,
    0xfb, 0x68, 0x7f, 0x63, 0x46, 0x18, 0xc0, 0x35,
    0xda, 0xc4, 0x39, 0xdc, 0x1a, 0xeb, 0x3b, 0x55,
    0x98, 0xa0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x01
};

/* ============================================================================
 * SHA256 Helper (Platform-Specific)
 * ========================================================================== */

/**
 * @brief Compute SHA256 hash
 */
static void sha256_hash(const uint8_t *data, size_t len, uint8_t out[32])
{
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    crypto_hash_sha256(out, data, len);
#else
    espsol_sha256(data, len, out);
#endif
}

/* ============================================================================
 * PDA Derivation
 * ========================================================================== */

/**
 * @brief Check if a point is on the Ed25519 curve
 *
 * This is a simplified check - we attempt to decompress the y-coordinate
 * and verify if it's a valid curve point.
 */
bool espsol_is_on_curve(const uint8_t pubkey[ESPSOL_PUBKEY_SIZE])
{
    if (pubkey == NULL) {
        return false;
    }
    
#if defined(ESP_PLATFORM) && ESP_PLATFORM
    /* Use libsodium to check if valid Ed25519 point */
    /* Try to use the public key - if it's invalid, operations fail */
    /* For libsodium, we can check by attempting core_ed25519_is_valid_point */
    return crypto_core_ed25519_is_valid_point(pubkey) == 1;
#else
    /* Simplified host check - just check if it's all zeros (definitely not on curve) */
    uint8_t zeros[ESPSOL_PUBKEY_SIZE] = {0};
    if (memcmp(pubkey, zeros, ESPSOL_PUBKEY_SIZE) == 0) {
        return false;
    }
    /* For host testing, we'll assume it's on curve unless it's a known PDA */
    /* A proper implementation would do the full curve check */
    return true;
#endif
}

esp_err_t espsol_token_find_pda(
    const uint8_t **seeds,
    const size_t *seed_lens,
    size_t seed_count,
    const uint8_t program_id[ESPSOL_PUBKEY_SIZE],
    uint8_t pda[ESPSOL_PUBKEY_SIZE],
    uint8_t *bump)
{
    if (seeds == NULL || seed_lens == NULL || program_id == NULL || pda == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Try bump seeds from 255 down to 0 */
    uint8_t hash_input[512];
    size_t offset;
    uint8_t candidate[32];
    
    /* PDA marker string */
    static const char PDA_MARKER[] = "ProgramDerivedAddress";
    
    for (int b = 255; b >= 0; b--) {
        offset = 0;
        
        /* Concatenate all seeds */
        for (size_t i = 0; i < seed_count; i++) {
            if (offset + seed_lens[i] > sizeof(hash_input) - 64) {
                return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
            }
            memcpy(hash_input + offset, seeds[i], seed_lens[i]);
            offset += seed_lens[i];
        }
        
        /* Add bump seed */
        hash_input[offset++] = (uint8_t)b;
        
        /* Add program ID */
        memcpy(hash_input + offset, program_id, ESPSOL_PUBKEY_SIZE);
        offset += ESPSOL_PUBKEY_SIZE;
        
        /* Add PDA marker */
        memcpy(hash_input + offset, PDA_MARKER, sizeof(PDA_MARKER) - 1);
        offset += sizeof(PDA_MARKER) - 1;
        
        /* Hash to get candidate */
        sha256_hash(hash_input, offset, candidate);
        
        /* Check if candidate is OFF the curve (valid PDA) */
        if (!espsol_is_on_curve(candidate)) {
            memcpy(pda, candidate, ESPSOL_PUBKEY_SIZE);
            if (bump != NULL) {
                *bump = (uint8_t)b;
            }
            return ESP_OK;
        }
    }
    
    /* No valid bump found (extremely rare) */
    return ESP_ERR_ESPSOL_CRYPTO_ERROR;
}

esp_err_t espsol_token_get_ata_address(
    const uint8_t wallet[ESPSOL_PUBKEY_SIZE],
    const uint8_t mint[ESPSOL_PUBKEY_SIZE],
    uint8_t ata_address[ESPSOL_PUBKEY_SIZE])
{
    if (wallet == NULL || mint == NULL || ata_address == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* ATA seeds: [wallet, token_program_id, mint] */
    const uint8_t *seeds[3] = {
        wallet,
        ESPSOL_TOKEN_PROGRAM_ID,
        mint
    };
    const size_t seed_lens[3] = {
        ESPSOL_PUBKEY_SIZE,
        ESPSOL_PUBKEY_SIZE,
        ESPSOL_PUBKEY_SIZE
    };
    
    return espsol_token_find_pda(
        seeds, seed_lens, 3,
        ESPSOL_ASSOCIATED_TOKEN_PROGRAM_ID,
        ata_address, NULL);
}

/* ============================================================================
 * Transaction Instructions - Associated Token Account
 * ========================================================================== */

esp_err_t espsol_tx_add_create_ata(
    espsol_tx_handle_t tx,
    const uint8_t payer[ESPSOL_PUBKEY_SIZE],
    const uint8_t wallet[ESPSOL_PUBKEY_SIZE],
    const uint8_t mint[ESPSOL_PUBKEY_SIZE])
{
    if (tx == NULL || payer == NULL || wallet == NULL || mint == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Derive ATA address */
    uint8_t ata[ESPSOL_PUBKEY_SIZE];
    esp_err_t err = espsol_token_get_ata_address(wallet, mint, ata);
    if (err != ESP_OK) {
        return err;
    }
    
    /* Build accounts for CreateAssociatedTokenAccount */
    espsol_account_meta_t accounts[7] = {
        /* 0: Payer (signer, writable) */
        {.is_signer = true, .is_writable = true},
        /* 1: Associated token account (writable) */
        {.is_signer = false, .is_writable = true},
        /* 2: Wallet owner (read-only) */
        {.is_signer = false, .is_writable = false},
        /* 3: Mint (read-only) */
        {.is_signer = false, .is_writable = false},
        /* 4: System Program (read-only) */
        {.is_signer = false, .is_writable = false},
        /* 5: Token Program (read-only) */
        {.is_signer = false, .is_writable = false},
    };
    
    memcpy(accounts[0].pubkey, payer, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[1].pubkey, ata, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[2].pubkey, wallet, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[3].pubkey, mint, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[4].pubkey, ESPSOL_SYSTEM_PROGRAM_ID, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[5].pubkey, ESPSOL_TOKEN_PROGRAM_ID, ESPSOL_PUBKEY_SIZE);
    
    /* No instruction data for standard CreateAssociatedTokenAccount */
    return espsol_tx_add_instruction(tx,
        ESPSOL_ASSOCIATED_TOKEN_PROGRAM_ID,
        accounts, 6,
        NULL, 0);
}

esp_err_t espsol_tx_add_create_ata_idempotent(
    espsol_tx_handle_t tx,
    const uint8_t payer[ESPSOL_PUBKEY_SIZE],
    const uint8_t wallet[ESPSOL_PUBKEY_SIZE],
    const uint8_t mint[ESPSOL_PUBKEY_SIZE])
{
    if (tx == NULL || payer == NULL || wallet == NULL || mint == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Derive ATA address */
    uint8_t ata[ESPSOL_PUBKEY_SIZE];
    esp_err_t err = espsol_token_get_ata_address(wallet, mint, ata);
    if (err != ESP_OK) {
        return err;
    }
    
    /* Build accounts for CreateAssociatedTokenAccountIdempotent */
    espsol_account_meta_t accounts[6] = {
        /* 0: Payer (signer, writable) */
        {.is_signer = true, .is_writable = true},
        /* 1: Associated token account (writable) */
        {.is_signer = false, .is_writable = true},
        /* 2: Wallet owner (read-only) */
        {.is_signer = false, .is_writable = false},
        /* 3: Mint (read-only) */
        {.is_signer = false, .is_writable = false},
        /* 4: System Program (read-only) */
        {.is_signer = false, .is_writable = false},
        /* 5: Token Program (read-only) */
        {.is_signer = false, .is_writable = false},
    };
    
    memcpy(accounts[0].pubkey, payer, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[1].pubkey, ata, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[2].pubkey, wallet, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[3].pubkey, mint, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[4].pubkey, ESPSOL_SYSTEM_PROGRAM_ID, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[5].pubkey, ESPSOL_TOKEN_PROGRAM_ID, ESPSOL_PUBKEY_SIZE);
    
    /* Idempotent uses instruction data byte = 1 */
    uint8_t data[1] = {1};
    return espsol_tx_add_instruction(tx,
        ESPSOL_ASSOCIATED_TOKEN_PROGRAM_ID,
        accounts, 6,
        data, 1);
}

/* ============================================================================
 * Transaction Instructions - Token Program
 * ========================================================================== */

esp_err_t espsol_tx_add_token_transfer(
    espsol_tx_handle_t tx,
    const uint8_t source[ESPSOL_PUBKEY_SIZE],
    const uint8_t dest[ESPSOL_PUBKEY_SIZE],
    const uint8_t owner[ESPSOL_PUBKEY_SIZE],
    uint64_t amount)
{
    if (tx == NULL || source == NULL || dest == NULL || owner == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Build accounts for Transfer */
    espsol_account_meta_t accounts[3] = {
        /* 0: Source token account (writable) */
        {.is_signer = false, .is_writable = true},
        /* 1: Destination token account (writable) */
        {.is_signer = false, .is_writable = true},
        /* 2: Source owner (signer) */
        {.is_signer = true, .is_writable = false},
    };
    
    memcpy(accounts[0].pubkey, source, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[1].pubkey, dest, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[2].pubkey, owner, ESPSOL_PUBKEY_SIZE);
    
    /* Instruction data: [instruction_type(1), amount(8)] = 9 bytes */
    uint8_t data[9];
    data[0] = ESPSOL_TOKEN_TRANSFER;
    /* Little-endian amount */
    data[1] = (uint8_t)(amount);
    data[2] = (uint8_t)(amount >> 8);
    data[3] = (uint8_t)(amount >> 16);
    data[4] = (uint8_t)(amount >> 24);
    data[5] = (uint8_t)(amount >> 32);
    data[6] = (uint8_t)(amount >> 40);
    data[7] = (uint8_t)(amount >> 48);
    data[8] = (uint8_t)(amount >> 56);
    
    return espsol_tx_add_instruction(tx,
        ESPSOL_TOKEN_PROGRAM_ID,
        accounts, 3,
        data, sizeof(data));
}

esp_err_t espsol_tx_add_token_transfer_checked(
    espsol_tx_handle_t tx,
    const uint8_t source[ESPSOL_PUBKEY_SIZE],
    const uint8_t mint[ESPSOL_PUBKEY_SIZE],
    const uint8_t dest[ESPSOL_PUBKEY_SIZE],
    const uint8_t owner[ESPSOL_PUBKEY_SIZE],
    uint64_t amount,
    uint8_t decimals)
{
    if (tx == NULL || source == NULL || mint == NULL || 
        dest == NULL || owner == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Build accounts for TransferChecked */
    espsol_account_meta_t accounts[4] = {
        /* 0: Source token account (writable) */
        {.is_signer = false, .is_writable = true},
        /* 1: Mint (read-only) */
        {.is_signer = false, .is_writable = false},
        /* 2: Destination token account (writable) */
        {.is_signer = false, .is_writable = true},
        /* 3: Source owner (signer) */
        {.is_signer = true, .is_writable = false},
    };
    
    memcpy(accounts[0].pubkey, source, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[1].pubkey, mint, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[2].pubkey, dest, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[3].pubkey, owner, ESPSOL_PUBKEY_SIZE);
    
    /* Instruction data: [instruction_type(1), amount(8), decimals(1)] = 10 bytes */
    uint8_t data[10];
    data[0] = ESPSOL_TOKEN_TRANSFER_CHECKED;
    /* Little-endian amount */
    data[1] = (uint8_t)(amount);
    data[2] = (uint8_t)(amount >> 8);
    data[3] = (uint8_t)(amount >> 16);
    data[4] = (uint8_t)(amount >> 24);
    data[5] = (uint8_t)(amount >> 32);
    data[6] = (uint8_t)(amount >> 40);
    data[7] = (uint8_t)(amount >> 48);
    data[8] = (uint8_t)(amount >> 56);
    data[9] = decimals;
    
    return espsol_tx_add_instruction(tx,
        ESPSOL_TOKEN_PROGRAM_ID,
        accounts, 4,
        data, sizeof(data));
}

esp_err_t espsol_tx_add_token_mint_to(
    espsol_tx_handle_t tx,
    const uint8_t mint[ESPSOL_PUBKEY_SIZE],
    const uint8_t dest[ESPSOL_PUBKEY_SIZE],
    const uint8_t mint_authority[ESPSOL_PUBKEY_SIZE],
    uint64_t amount)
{
    if (tx == NULL || mint == NULL || dest == NULL || mint_authority == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Build accounts for MintTo */
    espsol_account_meta_t accounts[3] = {
        /* 0: Mint (writable) */
        {.is_signer = false, .is_writable = true},
        /* 1: Destination token account (writable) */
        {.is_signer = false, .is_writable = true},
        /* 2: Mint authority (signer) */
        {.is_signer = true, .is_writable = false},
    };
    
    memcpy(accounts[0].pubkey, mint, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[1].pubkey, dest, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[2].pubkey, mint_authority, ESPSOL_PUBKEY_SIZE);
    
    /* Instruction data: [instruction_type(1), amount(8)] = 9 bytes */
    uint8_t data[9];
    data[0] = ESPSOL_TOKEN_MINT_TO;
    /* Little-endian amount */
    data[1] = (uint8_t)(amount);
    data[2] = (uint8_t)(amount >> 8);
    data[3] = (uint8_t)(amount >> 16);
    data[4] = (uint8_t)(amount >> 24);
    data[5] = (uint8_t)(amount >> 32);
    data[6] = (uint8_t)(amount >> 40);
    data[7] = (uint8_t)(amount >> 48);
    data[8] = (uint8_t)(amount >> 56);
    
    return espsol_tx_add_instruction(tx,
        ESPSOL_TOKEN_PROGRAM_ID,
        accounts, 3,
        data, sizeof(data));
}

esp_err_t espsol_tx_add_token_burn(
    espsol_tx_handle_t tx,
    const uint8_t account[ESPSOL_PUBKEY_SIZE],
    const uint8_t mint[ESPSOL_PUBKEY_SIZE],
    const uint8_t owner[ESPSOL_PUBKEY_SIZE],
    uint64_t amount)
{
    if (tx == NULL || account == NULL || mint == NULL || owner == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Build accounts for Burn */
    espsol_account_meta_t accounts[3] = {
        /* 0: Token account (writable) */
        {.is_signer = false, .is_writable = true},
        /* 1: Mint (writable) */
        {.is_signer = false, .is_writable = true},
        /* 2: Account owner (signer) */
        {.is_signer = true, .is_writable = false},
    };
    
    memcpy(accounts[0].pubkey, account, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[1].pubkey, mint, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[2].pubkey, owner, ESPSOL_PUBKEY_SIZE);
    
    /* Instruction data: [instruction_type(1), amount(8)] = 9 bytes */
    uint8_t data[9];
    data[0] = ESPSOL_TOKEN_BURN;
    /* Little-endian amount */
    data[1] = (uint8_t)(amount);
    data[2] = (uint8_t)(amount >> 8);
    data[3] = (uint8_t)(amount >> 16);
    data[4] = (uint8_t)(amount >> 24);
    data[5] = (uint8_t)(amount >> 32);
    data[6] = (uint8_t)(amount >> 40);
    data[7] = (uint8_t)(amount >> 48);
    data[8] = (uint8_t)(amount >> 56);
    
    return espsol_tx_add_instruction(tx,
        ESPSOL_TOKEN_PROGRAM_ID,
        accounts, 3,
        data, sizeof(data));
}

esp_err_t espsol_tx_add_token_close_account(
    espsol_tx_handle_t tx,
    const uint8_t account[ESPSOL_PUBKEY_SIZE],
    const uint8_t dest[ESPSOL_PUBKEY_SIZE],
    const uint8_t owner[ESPSOL_PUBKEY_SIZE])
{
    if (tx == NULL || account == NULL || dest == NULL || owner == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Build accounts for CloseAccount */
    espsol_account_meta_t accounts[3] = {
        /* 0: Token account (writable) */
        {.is_signer = false, .is_writable = true},
        /* 1: Destination (writable) - receives remaining lamports */
        {.is_signer = false, .is_writable = true},
        /* 2: Account owner (signer) */
        {.is_signer = true, .is_writable = false},
    };
    
    memcpy(accounts[0].pubkey, account, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[1].pubkey, dest, ESPSOL_PUBKEY_SIZE);
    memcpy(accounts[2].pubkey, owner, ESPSOL_PUBKEY_SIZE);
    
    /* Instruction data: just the instruction type */
    uint8_t data[1] = {ESPSOL_TOKEN_CLOSE_ACCOUNT};
    
    return espsol_tx_add_instruction(tx,
        ESPSOL_TOKEN_PROGRAM_ID,
        accounts, 3,
        data, sizeof(data));
}

esp_err_t espsol_tx_add_token_sync_native(
    espsol_tx_handle_t tx,
    const uint8_t account[ESPSOL_PUBKEY_SIZE])
{
    if (tx == NULL || account == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Build accounts for SyncNative */
    espsol_account_meta_t accounts[1] = {
        /* 0: Token account (writable) */
        {.is_signer = false, .is_writable = true},
    };
    
    memcpy(accounts[0].pubkey, account, ESPSOL_PUBKEY_SIZE);
    
    /* Instruction data: just the instruction type */
    uint8_t data[1] = {ESPSOL_TOKEN_SYNC_NATIVE};
    
    return espsol_tx_add_instruction(tx,
        ESPSOL_TOKEN_PROGRAM_ID,
        accounts, 1,
        data, sizeof(data));
}

/* ============================================================================
 * Utility Functions
 * ========================================================================== */

esp_err_t espsol_get_native_mint(uint8_t mint[ESPSOL_PUBKEY_SIZE])
{
    if (mint == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(mint, NATIVE_MINT, ESPSOL_PUBKEY_SIZE);
    return ESP_OK;
}
