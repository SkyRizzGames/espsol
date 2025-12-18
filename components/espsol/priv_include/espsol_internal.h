/**
 * @file espsol_internal.h
 * @brief ESPSOL Internal Utilities (Private Header)
 *
 * This file contains internal utilities and helpers not exposed in the public API.
 *
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#ifndef ESPSOL_INTERNAL_H
#define ESPSOL_INTERNAL_H

#include "espsol_types.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Internal Macros
 * ========================================================================== */

/**
 * @brief Check if a pointer is NULL and return error
 */
#define ESPSOL_CHECK_NULL(ptr, err) \
    do { \
        if ((ptr) == NULL) { \
            return (err); \
        } \
    } while (0)

/**
 * @brief Check if a pointer is NULL and return ESP_ERR_INVALID_ARG
 */
#define ESPSOL_CHECK_ARG(ptr) ESPSOL_CHECK_NULL(ptr, ESP_ERR_INVALID_ARG)

/**
 * @brief Return if error
 */
#define ESPSOL_RETURN_ON_ERROR(x) \
    do { \
        esp_err_t __err = (x); \
        if (__err != ESP_OK) { \
            return __err; \
        } \
    } while (0)

/**
 * @brief Secure memory zeroing (prevents compiler optimization)
 */
static inline void espsol_secure_zero(void *ptr, size_t len)
{
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--) {
        *p++ = 0;
    }
}

/**
 * @brief Constant-time memory comparison (for crypto operations)
 */
static inline int espsol_secure_compare(const void *a, const void *b, size_t len)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    uint8_t diff = 0;
    
    for (size_t i = 0; i < len; i++) {
        diff |= pa[i] ^ pb[i];
    }
    
    return diff == 0 ? 0 : 1;
}

#ifdef __cplusplus
}
#endif

#endif /* ESPSOL_INTERNAL_H */
