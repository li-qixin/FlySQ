
# =============================================================================
# This file is used to configure nuttx 
# 
# 1. set cmake variables represents path and pull nuttx kernel, apps source code
# 2. copy defconfig into nuttx kernel source path
# 3. configure nuttx based on provided file: nuttx-config/*/defconfig and nuttx
#    Kconfig system
# 4. parse nuttx defconfig options for cmake
# 5. add CONFIG_ARCH_CHIP to boardconfig by merging
# =============================================================================

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS_EQUAL 7)
		message(FATAL_ERROR "GCC 7 or older no longer supported.")
	endif()
endif()

# step 1
# -----------------------------------------------------------------------------
if(NOT TAROX_CONFIG_DIR)
    message(FATAL_ERROR "TAROX_CONFIG_DIR must be set")
endif()

set(NUTTX_CONFIG_DIR ${TAROX_CONFIG_DIR}/nuttx-config CACHE FILEPATH "nuttx config path" FORCE)
if(NOT EXISTS ${NUTTX_CONFIG_DIR})
    message(FATAL_ERROR "not exist ${NUTTX_CONFIG_DIR}")
endif()

if(NOT NUTTX_CONFIG)
    set(NUTTX_CONFIG "nsh" CACHE STRING "config of nuttx" FORCE)
endif()

set(NUTTX_DEFCONFIG ${NUTTX_CONFIG_DIR}/${NUTTX_CONFIG}/defconfig CACHE FILEPATH "path to defconfig" FORCE)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${NUTTX_DEFCONFIG})

set(NUTTX_SRC_DIR ${TAROX_SOURCE_DIR}/platforms/nuttx/Nuttx)
set(NUTTX_KERNEL_DIR ${TAROX_SOURCE_DIR}/platforms/nuttx/Nuttx/nuttx CACHE FILEPATH "nuttx kernel directory" FORCE)
set(NUTTX_APPS_DIR ${TAROX_SOURCE_DIR}/platforms/nuttx/Nuttx/apps CACHE FILEPATH "nuttx apps directory" FORCE)

include(tarox_git) # define tarox_add_git_submodule function
tarox_add_git_submodule(TARGET git_nuttx PATH "${NUTTX_SRC_DIR}/nuttx")
tarox_add_git_submodule(TARGET git_nuttx_apps PATH "${NUTTX_SRC_DIR}/apps")

# step 2
# -----------------------------------------------------------------------------
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${NUTTX_CONFIG_DIR}/src)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NUTTX_DEFCONFIG} ${NUTTX_KERNEL_DIR}/.config)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TAROX_SOURCE_DIR}/platforms/nuttx/Nuttx/Make.defs.in ${NUTTX_KERNEL_DIR}/Make.defs)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${NUTTX_KERNEL_DIR}/defconfig)

# step 3
# -----------------------------------------------------------------------------
execute_process(
  COMMAND ${NUTTX_SRC_DIR}/tools/nuttx_make_olddefconfig.sh
  WORKING_DIRECTORY ${NUTTX_KERNEL_DIR}
  OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/nuttx_olddefconfig.log
)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NUTTX_KERNEL_DIR}/.config ${TAROX_BINARY_DIR}/.config)

# step 4
# -----------------------------------------------------------------------------
file(STRINGS ${NUTTX_KERNEL_DIR}/.config ConfigContents)
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
		set(${Name} ${Value} CACHE INTERNAL "NUTTX DEFCONFIG: ${Name}" FORCE)
	endif()
endforeach()

# step 5
# -----------------------------------------------------------------------------
execute_process(
	COMMAND ${CMAKE_COMMAND} -E env ${COMMON_KCONFIG_ENV_SETTINGS}
	${PYTHON_EXECUTABLE} ${TAROX_SOURCE_DIR}/tools/kconfig/merge_config.py Kconfig ${TAROX_BOARD_CONFIG} ${TAROX_BOARD_CONFIG} ${NUTTX_KERNEL_DIR}/.config
	WORKING_DIRECTORY ${TAROX_SOURCE_DIR}
	OUTPUT_VARIABLE DUMMY_RESULTS
)