/**
 * @file espsol_base64.c
 * @brief Base64 Encoding/Decoding Implementation
 *
 * Implements Base64 encoding and decoding for transaction serialization.
 * Uses standard Base64 alphabet (RFC 4648).
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#include "espsol_utils.h"
#include <string.h>

/* Standard Base64 alphabet */
static const char BASE64_ALPHABET[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Base64 padding character */
static const char BASE64_PAD = '=';

/* Reverse lookup table for Base64 decoding (-1 = invalid, -2 = padding) */
static const int8_t BASE64_MAP[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 0-15 */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 16-31 */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 32-47 (+, /) */
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,  /* 48-63 (0-9, =) */
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 64-79 (A-O) */
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 80-95 (P-Z) */
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 96-111 (a-o) */
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 112-127 (p-z) */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

size_t espsol_base64_encoded_len(size_t data_len)
{
    /* Base64 encoding: 3 bytes -> 4 characters, with padding */
    return ((data_len + 2) / 3) * 4 + 1;  /* +1 for null terminator */
}

size_t espsol_base64_decoded_len(size_t encoded_len)
{
    /* Base64 decoding: 4 characters -> 3 bytes (max, may be less with padding) */
    return (encoded_len / 4) * 3;
}

esp_err_t espsol_base64_encode(const uint8_t *data, size_t data_len,
                                char *output, size_t output_len)
{
    if (data == NULL || output == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t required_len = espsol_base64_encoded_len(data_len);
    
    if (output_len < required_len) {
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }

    if (data_len == 0) {
        output[0] = '\0';
        return ESP_OK;
    }

    size_t out_idx = 0;
    size_t i;

    /* Process full 3-byte blocks */
    for (i = 0; i + 2 < data_len; i += 3) {
        uint32_t triple = ((uint32_t)data[i] << 16) | 
                          ((uint32_t)data[i + 1] << 8) | 
                          (uint32_t)data[i + 2];
        
        output[out_idx++] = BASE64_ALPHABET[(triple >> 18) & 0x3F];
        output[out_idx++] = BASE64_ALPHABET[(triple >> 12) & 0x3F];
        output[out_idx++] = BASE64_ALPHABET[(triple >> 6) & 0x3F];
        output[out_idx++] = BASE64_ALPHABET[triple & 0x3F];
    }

    /* Handle remaining bytes (0, 1, or 2) */
    size_t remaining = data_len - i;
    
    if (remaining == 1) {
        uint32_t val = (uint32_t)data[i] << 16;
        output[out_idx++] = BASE64_ALPHABET[(val >> 18) & 0x3F];
        output[out_idx++] = BASE64_ALPHABET[(val >> 12) & 0x3F];
        output[out_idx++] = BASE64_PAD;
        output[out_idx++] = BASE64_PAD;
    } else if (remaining == 2) {
        uint32_t val = ((uint32_t)data[i] << 16) | ((uint32_t)data[i + 1] << 8);
        output[out_idx++] = BASE64_ALPHABET[(val >> 18) & 0x3F];
        output[out_idx++] = BASE64_ALPHABET[(val >> 12) & 0x3F];
        output[out_idx++] = BASE64_ALPHABET[(val >> 6) & 0x3F];
        output[out_idx++] = BASE64_PAD;
    }

    output[out_idx] = '\0';
    return ESP_OK;
}

esp_err_t espsol_base64_decode(const char *input,
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

    /* Input length must be multiple of 4 */
    if (input_len % 4 != 0) {
        return ESP_ERR_ESPSOL_INVALID_BASE64;
    }

    /* Count padding characters */
    size_t padding = 0;
    if (input_len >= 1 && input[input_len - 1] == BASE64_PAD) {
        padding++;
    }
    if (input_len >= 2 && input[input_len - 2] == BASE64_PAD) {
        padding++;
    }

    /* Calculate output length */
    size_t decoded_len = (input_len / 4) * 3 - padding;
    
    if (*output_len < decoded_len) {
        *output_len = decoded_len;
        return ESP_ERR_ESPSOL_BUFFER_TOO_SMALL;
    }

    size_t out_idx = 0;

    /* Process 4-character blocks */
    for (size_t i = 0; i < input_len; i += 4) {
        int8_t v0 = BASE64_MAP[(uint8_t)input[i]];
        int8_t v1 = BASE64_MAP[(uint8_t)input[i + 1]];
        int8_t v2 = BASE64_MAP[(uint8_t)input[i + 2]];
        int8_t v3 = BASE64_MAP[(uint8_t)input[i + 3]];

        /* Check for invalid characters */
        if (v0 < 0 || v1 < 0) {
            return ESP_ERR_ESPSOL_INVALID_BASE64;
        }

        /* Handle padding */
        bool pad2 = (v2 == -2);
        bool pad3 = (v3 == -2);
        
        if (v2 < 0 && !pad2) {
            return ESP_ERR_ESPSOL_INVALID_BASE64;
        }
        if (v3 < 0 && !pad3) {
            return ESP_ERR_ESPSOL_INVALID_BASE64;
        }

        /* Reconstruct bytes */
        uint32_t triple = ((uint32_t)v0 << 18) | 
                          ((uint32_t)v1 << 12) |
                          ((uint32_t)(pad2 ? 0 : v2) << 6) |
                          (uint32_t)(pad3 ? 0 : v3);

        output[out_idx++] = (triple >> 16) & 0xFF;
        
        if (!pad2) {
            output[out_idx++] = (triple >> 8) & 0xFF;
        }
        if (!pad3) {
            output[out_idx++] = triple & 0xFF;
        }
    }

    *output_len = decoded_len;
    return ESP_OK;
}
