# ESPSOL WebSocket Subscription Example

cmake_minimum_required(VERSION 3.16)

# Include the ESPSOL component from the project root
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(websocket_sub)
