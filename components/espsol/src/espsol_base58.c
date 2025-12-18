/**
 * @file espsol_base58.c
 * @brief Base58 Encoding/Decoding Implementation
 *
 * Implements Base58 encoding and decoding using the Bitcoin/Solana alphabet.
 * This is used for encoding Solana public keys, private keys, and signatures.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include "espsol_utils.h"
#include <string.h>

/* Base58 alphabet (Bitcoin/Solana) */
static const char BASE58_ALPHABET[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

/* Reverse lookup table for Base58 decoding (-1 = invalid character) */
static const int8_t BASE58_MAP[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8,-1,-1,-1,-1,-1,-1, /* 0-9 */
    -1, 9,10,11,12,13,14,15,16,-1,17,18,19,20,21,-1, /* A-O */
    22,23,24,25,26,27,28,29,30,31,32,-1,-1,-1,-1,-1, /* P-Z */
    -1,33,34,35,36,37,38,39,40,41,42,43,-1,44,45,46, /* a-o */
    47,48,49,50,51,52,53,54,55,56,57,-1,-1,-1,-1,-1, /* p-z */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

size_t espsol_base58_encoded_len(size_t data_len)
{
    /* Base58 encoded length is approximately data_len * 138 / 100 + 1 */
    /* We add extra for safety */
    return (data_len * 138 / 100) + 2;
}

size_t espsol_base58_decoded_len(size_t encoded_len)
{
    /* Base58 decoded length is approximately encoded_len * 733 / 1000 + 1 */
    return (encoded_len * 733 / 1000) + 2;
}

esp_err_t espsol_base58_encode(const uint8_t *data, size_t data_len,
                                char *output, size_t output_len)
{
    if (data == NULL || output == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (data_len == 0) {
        if (output_len < 1) {
            return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
        }
        output[0] = '\0';
        return ESP_OK;
    }

    /* Count leading zeros */
    size_t leading_zeros = 0;
    while (leading_zeros < data_len && data[leading_zeros] == 0) {
        leading_zeros++;
    }

    /* Allocate temporary buffer for base58 conversion */
    /* Max size: data_len * 138 / 100 + 1 */
    size_t max_encoded = espsol_base58_encoded_len(data_len);
    
    if (output_len < max_encoded) {
        /* Check if buffer might be too small */
        /* We'll verify the actual size after encoding */
    }

    /* Working buffer - we'll build the result in reverse */
    uint8_t temp[256];  /* Should be enough for most Solana use cases */
    if (max_encoded > sizeof(temp)) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    memset(temp, 0, sizeof(temp));
    size_t temp_len = 0;

    /* Process the input as a big number, converting to base58 */
    for (size_t i = leading_zeros; i < data_len; i++) {
        uint32_t carry = data[i];
        
        /* Multiply existing digits by 256 and add carry */
        for (size_t j = 0; j < temp_len || carry; j++) {
            if (j >= sizeof(temp)) {
                return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
            }
            
            carry += 256 * (j < temp_len ? temp[j] : 0);
            temp[j] = carry % 58;
            carry /= 58;
            
            if (j >= temp_len) {
                temp_len = j + 1;
            }
        }
    }

    /* Calculate total output length */
    size_t total_len = leading_zeros + temp_len;
    
    if (output_len < total_len + 1) {  /* +1 for null terminator */
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }

    /* Add leading '1's for each leading zero byte */
    for (size_t i = 0; i < leading_zeros; i++) {
        output[i] = '1';
    }

    /* Copy the base58 digits in reverse order */
    for (size_t i = 0; i < temp_len; i++) {
        output[leading_zeros + i] = BASE58_ALPHABET[temp[temp_len - 1 - i]];
    }

    /* Null terminate */
    output[total_len] = '\0';

    return ESP_OK;
}

esp_err_t espsol_base58_decode(const char *input,
                                uint8_t *output, size_t *output_len)
{
    if (input == NULL || output == NULL || output_len == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t input_len = strlen(input);
    
    if (input_len == 0) {
        *output_len = 0;
        return ESP_OK;
    }

    /* Count leading '1's (these represent leading zero bytes) */
    size_t leading_ones = 0;
    while (leading_ones < input_len && input[leading_ones] == '1') {
        leading_ones++;
    }

    /* Allocate temporary buffer for binary conversion */
    size_t max_decoded = espsol_base58_decoded_len(input_len);
    uint8_t temp[256];  /* Should be enough for most Solana use cases */
    
    if (max_decoded > sizeof(temp)) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }
    
    memset(temp, 0, sizeof(temp));
    size_t temp_len = 0;

    /* Process each Base58 character */
    for (size_t i = leading_ones; i < input_len; i++) {
        /* Get the value of this Base58 character */
        int8_t val = BASE58_MAP[(uint8_t)input[i]];
        
        if (val < 0) {
            return ESP_ERR_ESPSOL_INVALID_BASE58;
        }

        uint32_t carry = (uint32_t)val;

        /* Multiply existing digits by 58 and add carry */
        for (size_t j = 0; j < temp_len || carry; j++) {
            if (j >= sizeof(temp)) {
                return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
            }
            
            carry += 58 * (j < temp_len ? temp[j] : 0);
            temp[j] = carry & 0xFF;
            carry >>= 8;
            
            if (j >= temp_len) {
                temp_len = j + 1;
            }
        }
    }

    /* Calculate total output length */
    size_t total_len = leading_ones + temp_len;
    
    if (*output_len < total_len) {
        *output_len = total_len;  /* Report required size */
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }

    /* Write leading zeros */
    for (size_t i = 0; i < leading_ones; i++) {
        output[i] = 0;
    }

    /* Copy the binary data in reverse order */
    for (size_t i = 0; i < temp_len; i++) {
        output[leading_ones + i] = temp[temp_len - 1 - i];
    }

    *output_len = total_len;
    return ESP_OK;
}

esp_err_t espsol_pubkey_to_address(const uint8_t pubkey[ESPSOL_PUBKEY_SIZE],
                                    char *address, size_t len)
{
    if (pubkey == NULL || address == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (len < ESPSOL_ADDRESS_MAX_LEN) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }

    return espsol_base58_encode(pubkey, ESPSOL_PUBKEY_SIZE, address, len);
}

esp_err_t espsol_address_to_pubkey(const char *address,
                                    uint8_t pubkey[ESPSOL_PUBKEY_SIZE])
{
    if (address == NULL || pubkey == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t decoded_len = ESPSOL_PUBKEY_SIZE;
    esp_err_t err = espsol_base58_decode(address, pubkey, &decoded_len);
    
    if (err != ESP_OK) {
        return err;
    }

    if (decoded_len != ESPSOL_PUBKEY_SIZE) {
        return ESP_ERR_ESPSOL_INVALID_BASE58;
    }

    return ESP_OK;
}

bool espsol_is_valid_address(const char *address)
{
    if (address == NULL) {
        return false;
    }

    uint8_t pubkey[ESPSOL_PUBKEY_SIZE];
    return espsol_address_to_pubkey(address, pubkey) == ESP_OK;
}

esp_err_t espsol_hex_encode(const uint8_t *data, size_t data_len,
                             char *output, size_t output_len)
{
    if (data == NULL || output == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (output_len < (data_len * 2 + 1)) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }

    static const char hex_chars[] = "0123456789abcdef";
    
    for (size_t i = 0; i < data_len; i++) {
        output[i * 2] = hex_chars[(data[i] >> 4) & 0x0F];
        output[i * 2 + 1] = hex_chars[data[i] & 0x0F];
    }
    
    output[data_len * 2] = '\0';
    return ESP_OK;
}

esp_err_t espsol_hex_decode(const char *input,
                             uint8_t *output, size_t *output_len)
{
    if (input == NULL || output == NULL || output_len == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t input_len = strlen(input);
    
    if (input_len % 2 != 0) {
        return ESP_ERR_ESPSOL_ENCODING_FAILED;
    }

    size_t decoded_len = input_len / 2;
    
    if (*output_len < decoded_len) {
        *output_len = decoded_len;
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }

    for (size_t i = 0; i < decoded_len; i++) {
        char c1 = input[i * 2];
        char c2 = input[i * 2 + 1];
        
        uint8_t val = 0;
        
        if (c1 >= '0' && c1 <= '9') {
            val = (c1 - '0') << 4;
        } else if (c1 >= 'a' && c1 <= 'f') {
            val = (c1 - 'a' + 10) << 4;
        } else if (c1 >= 'A' && c1 <= 'F') {
            val = (c1 - 'A' + 10) << 4;
        } else {
            return ESP_ERR_ESPSOL_ENCODING_FAILED;
        }
        
        if (c2 >= '0' && c2 <= '9') {
            val |= (c2 - '0');
        } else if (c2 >= 'a' && c2 <= 'f') {
            val |= (c2 - 'a' + 10);
        } else if (c2 >= 'A' && c2 <= 'F') {
            val |= (c2 - 'A' + 10);
        } else {
            return ESP_ERR_ESPSOL_ENCODING_FAILED;
        }
        
        output[i] = val;
    }

    *output_len = decoded_len;
    return ESP_OK;
}
