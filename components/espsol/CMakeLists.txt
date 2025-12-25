# ESPSOL - ESP-IDF Solana Component
# Copyright (c) 2025 SkyRizz
# SPDX-License-Identifier: Apache-2.0

idf_component_register(
    SRCS
        "src/espsol.c"
        "src/espsol_base58.c"
        "src/espsol_base64.c"
        "src/espsol_crypto.c"
        "src/espsol_mnemonic.c"
        "src/espsol_bip39_wordlist.c"
        "src/espsol_rpc.c"
        "src/espsol_tx.c"
        "src/espsol_token.c"
    INCLUDE_DIRS
        "include"
    PRIV_INCLUDE_DIRS
        "priv_include"
    REQUIRES
        esp_common
        log
        libsodium
        esp_http_client
        json
        nvs_flash
)

# Enable strict warnings
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Werror=implicit-function-declaration
)
