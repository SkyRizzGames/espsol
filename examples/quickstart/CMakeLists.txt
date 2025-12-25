# ESPSOL Quickstart Example

cmake_minimum_required(VERSION 3.16)

# Add ESPSOL component directory
set(EXTRA_COMPONENT_DIRS "../../../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(espsol_quickstart)
