cmake_minimum_required(VERSION 3.16)

# Add path to ESPSOL component
set(EXTRA_COMPONENT_DIRS "../../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(basic_rpc)
