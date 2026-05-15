add_subdirectory(${TAROX_CONFIG_DIR}/src ${TAROX_BINARY_DIR}/board/src)

if(NOT DEFINED TAROX_PLATFORM_OS_INCLUDES)
	set(TAROX_PLATFORM_OS_INCLUDES "")
endif()

set(TAROX_MODULE_OS_INCLUDES ${TAROX_PLATFORM_OS_INCLUDES})

add_subdirectory(${TAROX_SOURCE_DIR}/src/modules ${TAROX_BINARY_DIR}/modules)

add_subdirectory(Nuttx Nuttx)

add_library(tarox_platform INTERFACE)
target_include_directories(tarox_platform INTERFACE ${TAROX_MODULE_OS_INCLUDES})
target_link_libraries(tarox_platform INTERFACE nuttx_arch nuttx_drivers)

get_property(TAROX_MODULE_LIBRARIES GLOBAL PROPERTY TAROX_MODULE_LIBRARIES)
if(TAROX_MODULE_LIBRARIES)
	foreach(module_target ${TAROX_MODULE_LIBRARIES})
		target_link_libraries(${module_target} PRIVATE tarox_platform)
	endforeach()
endif()

add_executable(tarox ${TAROX_SOURCE_DIR}/platforms/common/empty.c)
set(nuttx_libs)
list(APPEND nuttx_libs
	nuttx_boards
	nuttx_drivers
	nuttx_fs
	nuttx_sched
	nuttx_crypto
	nuttx_binfmt
	nuttx_xx
	)
list(APPEND nuttx_libs
    nuttx_apps
    nuttx_arch
    nuttx_mm
    nuttx_c
)

get_property(TAROX_MODULE_LIBRARIES GLOBAL PROPERTY TAROX_MODULE_LIBRARIES)
if(TAROX_MODULE_LIBRARIES)
	list(APPEND nuttx_libs ${TAROX_MODULE_LIBRARIES})
endif()

target_link_libraries(nuttx_c INTERFACE nuttx_sched) # nxsched_get_streams

target_link_libraries(nuttx_arch
	INTERFACE
		drivers_board
		# arch_hrt
		# arch_board_reset
)

target_link_libraries(nuttx_c INTERFACE nuttx_drivers)
target_link_libraries(nuttx_drivers INTERFACE nuttx_c)
target_link_libraries(nuttx_xx INTERFACE nuttx_c)
target_link_libraries(nuttx_fs INTERFACE nuttx_c)

tarox_message(STATUS "TAROX_CONFIG_DIR: ${TAROX_CONFIG_DIR}" )
target_link_libraries(tarox PRIVATE
	-nostartfiles
	-nodefaultlibs
	-nostdlib
	-nostdinc++

	-fno-exceptions
	-fno-rtti
	-Wl,--script=${TAROX_CONFIG_DIR}/nuttx-config/scripts/script.ld
	-L${TAROX_CONFIG_DIR}/nuttx-config/scripts
	-Wl,-Map=${TAROX_CONFIG}.map
	-Wl,--warn-common
	-Wl,--gc-sections

	-Wl,--start-group
		${nuttx_libs}
	-Wl,--end-group

	m
	gcc
)

set(TAROX_BINARY_OUTPUT ${TAROX_BINARY_DIR}/tarox.bin)
add_custom_command(
	OUTPUT ${TAROX_BINARY_OUTPUT}
	COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:tarox> ${TAROX_BINARY_OUTPUT}
	DEPENDS tarox
	COMMENT "Creating ${TAROX_BINARY_OUTPUT}"
)
add_custom_target(tarox_bin ALL DEPENDS ${TAROX_BINARY_OUTPUT})