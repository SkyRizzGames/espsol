# ESPSOL Mnemonic Wallet Example
cmake_minimum_required(VERSION 3.16)

# Add the espsol component from parent directory
set(EXTRA_COMPONENT_DIRS "../../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mnemonic_wallet_example)
