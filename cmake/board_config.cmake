# =============================================================================
# This file is used to configure board based on board name ${TAROX_CONFIG_NAME} provided
# from Makefile. 
#
# 1. find board configure file path based on ${TAROX_CONFIG_NAME}
# 2. create kconfig python environment and command for the following usage
# 3. create ${TAROX_BINARY_DIR}/${TAROX_CONFIG_NAME}/tarox_boardconfig and ${TAROX_BINARY_DIR}/
#    tarox_boardconfig.h through tarox config file and Kconfig system
# 4. parse the config variables and store into cmake variables
# 5. configure based on parsed cmake variables
# =============================================================================

# step 1
# -----------------------------------------------------------------------------
if(NOT TAROX_CONFIG_NAME)
    set(TAROX_CONFIG_NAME "flysq_v1_default" CACHE STRING "desired configuration")
endif()

if(NOT TAROX_CONFIG_FILE)
    file(GLOB_RECURSE board_configs
        RELATIVE "${TAROX_SOURCE_DIR}/boards"
        "${TAROX_SOURCE_DIR}/boards/*.taroxconfig")

    foreach(filename ${board_configs})
        string(REPLACE ".taroxconfig" "" filename_stripped ${filename})
        string(REPLACE "/" ";" config ${filename_stripped})
        list(LENGTH config config_len)
        if(${config_len} EQUAL 3)
            list(GET config 0 vendor)
            list(GET config 1 model)
            list(GET config 2 label)

            if((${TAROX_CONFIG_NAME} MATCHES "${vendor}_${model}_${label}") 
                OR ((${label} STREQUAL "default") AND (${TAROX_CONFIG_NAME} STREQUAL "${vendor}_${model}")))

                set(TAROX_CONFIG_FILE "${TAROX_SOURCE_DIR}/boards/${filename}" 
                CACHE FILEPATH "path to tarox TAROX_CONFIG_NAME file" FORCE)

                set(TAROX_CONFIG_DIR "${TAROX_SOURCE_DIR}/boards/${vendor}/${model}"
                CACHE STRING "tarox board directory" FORCE)            
                set(MODEL "${model}" CACHE STRING "tarox board model" FORCE)
                set(VENDOR "${vendor}" CACHE STRING "tarox board vendor" FORCE)
                set(LABEL "${label}" CACHE STRING "tarox board vendor" FORCE)
                break()
            endif()
        endif()
    endforeach()

endif()

if(NOT EXISTS ${TAROX_CONFIG_FILE})
    message(FATAL_ERROR "doesn't find tarox config file of ${TAROX_CONFIG_FILE}")
endif()

message(STATUS "found tarox config file: ${TAROX_CONFIG_FILE}")
message(STATUS "tarox board path: ${TAROX_CONFIG_DIR}")


set(TAROX_BOARD_VENDOR
    ${VENDOR}
    CACHE STRING "TAROX board vendor" FORCE)
set(TAROX_BOARD_MODEL
    ${MODEL}
    CACHE STRING "TAROX board model" FORCE)
set(TAROX_BOARD_LABEL
    ${LABEL}
    CACHE STRING "TAROX board label" FORCE)


# step 2
# -----------------------------------------------------------------------------
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import menuconfig" RESULT_VARIABLE ret)
if(ret EQUAL "1")
    message(FATAL_ERROR "kconfiglib is not installed or not in PATH\n"
            "please install using \"pip3 install kconfiglib\"\n")
endif()

set(MENUCONFIG_PATH ${PYTHON_EXECUTABLE} -m menuconfig CACHE INTERNAL "menuconfig program" FORCE)
set(GUICONFIG_PATH ${PYTHON_EXECUTABLE} -m guiconfig CACHE INTERNAL "guiconfig program" FORCE)
set(DEFCONFIG_PATH ${PYTHON_EXECUTABLE} -m defconfig CACHE INTERNAL "defconfig program" FORCE)
set(SAVEDEFCONFIG_PATH ${PYTHON_EXECUTABLE} -m savedefconfig CACHE INTERNAL "savedefconfig program" FORCE)
set(GENCONFIG_PATH ${PYTHON_EXECUTABLE} -m genconfig CACHE INTERNAL "genconfig program" FORCE)

set(TAROX_BOARD_CONFIG ${TAROX_BINARY_DIR}/tarox_boardconfig CACHE FILEPATH "path to config" FORCE)

# tarox_boardconfig file path is passed through environment variabl
set(COMMON_KCONFIG_ENV_SETTINGS
    PYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
    KCONFIG_CONFIG=${TAROX_BOARD_CONFIG}
    # Set environment variables so that Kconfig can prune Kconfig source files
    # for other architectures
    PLATFORM=${TAROX_PLATFORM}
    VENDOR=${TAROX_BOARD_VENDOR}
    MODEL=${TAROX_BOARD_MODEL}
    LABEL=${TAROX_BOARD_LABEL}
    TOOLCHAIN=${CMAKE_TOOLCHAIN_FILE}
    ARCHITECTURE=${CMAKE_SYSTEM_PROCESSOR}
    ROMFSROOT=${config_romfs_root}
    BASE_DEFCONFIG=${TAROX_BOARD_CONFIG})

# step 3
# -----------------------------------------------------------------------------
if(EXISTS ${TAROX_CONFIG_FILE})
    set_property(
        DIRECTORY
        APPEND
        PROPERTY CMAKE_CONFIGURE_DEPENDS ${TAROX_CONFIG_FILE})

    if(${LABEL} MATCHES "default")
        # Generate tarox_boardconfig from saved tarox config
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env ${COMMON_KCONFIG_ENV_SETTINGS}
                    ${DEFCONFIG_PATH} ${TAROX_CONFIG_FILE}
            WORKING_DIRECTORY ${TAROX_SOURCE_DIR}
            OUTPUT_VARIABLE DUMMY_RESULTS)
    else()
        # Generate tarox_boardconfig from default.taroxboard and {label}.taroxboard
        execute_process(
            COMMAND
                ${CMAKE_COMMAND} -E env ${COMMON_KCONFIG_ENV_SETTINGS}
                ${PYTHON_EXECUTABLE} ${TAROX_SOURCE_DIR}/Tools/kconfig/merge_config.py
                Kconfig ${TAROX_BOARD_CONFIG} ${TAROX_CONFIG_DIR}/default.taroxboard
                ${TAROX_CONFIG_FILE}
            WORKING_DIRECTORY ${TAROX_SOURCE_DIR}
            OUTPUT_VARIABLE DUMMY_RESULTS)
    endif()
    # Generate header file for C/C++ preprocessor
    execute_process(
        COMMAND
        ${CMAKE_COMMAND} -E env ${COMMON_KCONFIG_ENV_SETTINGS} ${GENCONFIG_PATH}
        --header-path ${TAROX_BINARY_DIR}/tarox_boardconfig.h
        WORKING_DIRECTORY ${TAROX_SOURCE_DIR}
        OUTPUT_VARIABLE DUMMY_RESULTS)
endif()

# step 4
# -----------------------------------------------------------------------------
if(NOT EXISTS ${TAROX_BOARD_CONFIG})
    message(FATAL_ERROR "doesn't find generated file ${TAROX_BOARD_CONFIG}")
endif()

file(STRINGS ${TAROX_BOARD_CONFIG} ConfigContents)
foreach(NameAndValue ${ConfigContents})
    # Strip leading spaces
    string(REGEX REPLACE "^[ ]+" "" NameAndValue ${NameAndValue})

    # Find variable name
    string(REGEX MATCH "^CONFIG[^=]+" Name ${NameAndValue})

    if(Name)
        # Find the value
        string(REPLACE "${Name}=" "" Value ${NameAndValue})

        # remove extra quotes
        string(REPLACE "\"" "" Value ${Value})

        # Set the variable
        set(${Name} ${Value} CACHE INTERNAL "tarox board config: ${Name}" FORCE)

    else()
        # Find boolean not set
        string(REGEX MATCH " (CONFIG[^ ]+) is not set" Name ${NameAndValue})

        if(${CMAKE_MATCH_1})
            set(${CMAKE_MATCH_1}
                ""
                CACHE INTERNAL "BOARD DEFCONFIG: ${CMAKE_MATCH_1}" FORCE)
        endif()
    endif()

    # Find variable name
    string(REGEX MATCH "^CONFIG_BOARD_" Board ${NameAndValue})

    if(Board)
        string(REPLACE "CONFIG_BOARD_" "" ConfigKey ${Name})
        if(Value)
            set(${ConfigKey} ${Value})
            tarox_message(STATUS "${ConfigKey}: ${Value}" ${CMAKE_CURRENT_LIST_LINE})
        endif()
    endif()

endforeach()

# step 5
# -----------------------------------------------------------------------------
if(PLATFORM)
    set(TAROX_PLATFORM ${PLATFORM} CACHE STRING "tarox board OS" FORCE)
    list(APPEND CMAKE_MODULE_PATH ${TAROX_SOURCE_DIR}/platforms/${TAROX_PLATFORM}/cmake)
endif()

# CMAKE_SYSTEM_PROCESSOR is used to tell cmake what's architecture of the target platform
# and set specific architecture flags used in compiling
if(ARCHITECTURE)
    set(CMAKE_SYSTEM_PROCESSOR ${ARCHITECTURE} CACHE INTERNAL "system processor" FORCE)
endif()

# CMAKE_SYSTEM_PROCESSOR will automatically search cmake file to configure toolchain
if(TOOLCHAIN)
    set(CMAKE_TOOLCHAIN_FILE Toolchain-${TOOLCHAIN} CACHE INTERNAL "toolchain file" FORCE)
endif()