cmake_minimum_required(VERSION 3.16)

set(TAROX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(TAROX_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH ${TAROX_SOURCE_DIR}/cmake)

include(tarox_message)

# Find Python
find_package(PythonInterp 3)
if(NOT PYTHONINTERP_FOUND)
	message(FATAL_ERROR "Python 3 not found. Please install Python 3:\n"
		"    Ubuntu: sudo apt install python3 python3-dev python3-pip\n"
		"    macOS: brew install python")
endif()

tarox_message(STATUS "configuration name: ${TAROX_CONFIG_NAME}" ${CMAKE_CURRENT_LIST_LINE})

# board configuration based on ${TAROX_CONFIG_NAME}
include(board_config)

# os configuration
if(EXISTS "${TAROX_SOURCE_DIR}/platforms/${TAROX_PLATFORM}/cmake/os_init.cmake")
    include(os_init)
endif()

# =============================================================================
# construct project tree
# =============================================================================
project(tarox CXX C ASM)
add_subdirectory(${TAROX_SOURCE_DIR}/platforms/${TAROX_PLATFORM})