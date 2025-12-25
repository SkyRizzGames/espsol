/**
 * @file integration_test.h
 * @brief ESPSOL Devnet Integration Test Header
 * @copyright Copyright (c) 2025 SkyRizz
 * @license Apache-2.0
 */

#ifndef INTEGRATION_TEST_H
#define INTEGRATION_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run the devnet integration test
 *
 * This function:
 * 1. Connects to WiFi
 * 2. Creates two wallets
 * 3. Airdrops 1 SOL to wallet 1
 * 4. Transfers 0.1 SOL from wallet 1 to wallet 2
 * 5. Verifies balances
 *
 * Before running, update WIFI_SSID and WIFI_PASSWORD in integration_test.c
 */
void run_integration_test(void);

#ifdef __cplusplus
}
#endif

#endif /* INTEGRATION_TEST_H */
