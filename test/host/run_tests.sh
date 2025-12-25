#!/bin/bash
# Host-based unit test runner for ESPSOL
# Run this script from the test/host directory

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
COMPONENT_DIR="$SCRIPT_DIR/../../components/espsol"

echo "=============================================="
echo "   ESPSOL Host-Based Unit Tests"
echo "=============================================="
echo ""

# Common source files
COMMON_SRCS=(
    "$COMPONENT_DIR/src/espsol_base58.c"
    "$COMPONENT_DIR/src/espsol_base64.c"
    "$COMPONENT_DIR/src/espsol_crypto.c"
    "$COMPONENT_DIR/src/espsol_ed25519.c"
    "$COMPONENT_DIR/src/espsol_tx.c"
    "$COMPONENT_DIR/src/espsol_token.c"
)

# Mnemonic source files (uses SHA-256 from espsol_ed25519.c)
MNEMONIC_SRCS=(
    "$COMPONENT_DIR/src/espsol_mnemonic.c"
    "$COMPONENT_DIR/src/espsol_bip39_wordlist.c"
)

# Common flags
CFLAGS="-Wall -Wextra -g -I$COMPONENT_DIR/include -I$COMPONENT_DIR/priv_include -I$COMPONENT_DIR/src -DESP_PLATFORM=0"

echo "Compiling encoding and crypto tests..."
gcc $CFLAGS \
    "$SCRIPT_DIR/test_encoding.c" \
    "${COMMON_SRCS[@]}" \
    -o "$SCRIPT_DIR/test_encoding"

echo "Compiling transaction tests..."
gcc $CFLAGS \
    "$SCRIPT_DIR/test_tx.c" \
    "${COMMON_SRCS[@]}" \
    -o "$SCRIPT_DIR/test_tx"

echo "Compiling SPL token tests..."
gcc $CFLAGS \
    "$SCRIPT_DIR/test_token.c" \
    "${COMMON_SRCS[@]}" \
    -o "$SCRIPT_DIR/test_token"

echo "Compiling error handling tests..."
gcc $CFLAGS \
    "$SCRIPT_DIR/test_errors.c" \
    "${COMMON_SRCS[@]}" \
    -o "$SCRIPT_DIR/test_errors"

echo "Compiling mnemonic tests..."
gcc $CFLAGS \
    "$SCRIPT_DIR/test_mnemonic.c" \
    "${COMMON_SRCS[@]}" \
    "${MNEMONIC_SRCS[@]}" \
    -o "$SCRIPT_DIR/test_mnemonic"

echo ""
echo "Running encoding and crypto tests..."
echo ""
"$SCRIPT_DIR/test_encoding"

echo ""
echo "Running transaction tests..."
echo ""
"$SCRIPT_DIR/test_tx"

echo ""
echo "Running SPL token tests..."
echo ""
"$SCRIPT_DIR/test_token"

echo ""
echo "Running error handling tests..."
echo ""
"$SCRIPT_DIR/test_errors"

echo ""
echo "Running mnemonic tests..."
echo ""
"$SCRIPT_DIR/test_mnemonic"

# Clean up
rm -f "$SCRIPT_DIR/test_encoding" "$SCRIPT_DIR/test_tx" "$SCRIPT_DIR/test_token" "$SCRIPT_DIR/test_errors" "$SCRIPT_DIR/test_mnemonic"

echo ""
echo "All tests completed!"
