add_subdirectory(${TAROX_CONFIG_DIR}/src ${TAROX_BINARY_DIR}/board/src)

if(NOT DEFINED TAROX_PLATFORM_OS_INCLUDES)
	set(TAROX_PLATFORM_OS_INCLUDES "")
endif()

set(TAROX_MODULE_OS_INCLUDES ${TAROX_PLATFORM_OS_INCLUDES})

add_subdirectory(${TAROX_SOURCE_DIR}/src/modules ${TAROX_BINARY_DIR}/modules)

add_subdirectory(Nuttx Nuttx)

add_subdirectory(${TAROX_SOURCE_DIR}/platforms/nuttx/src ${TAROX_BINARY_DIR}/nuttx-platform/src)

set(TAROX_PLATFORM_API_INCLUDES
	${TAROX_SOURCE_DIR}/platforms/common/include
	${TAROX_CONFIG_DIR}/nuttx-config/include
)
add_library(tarox_platform INTERFACE)
target_include_directories(
	tarox_platform
	INTERFACE
		${TAROX_PLATFORM_API_INCLUDES}
		${TAROX_MODULE_OS_INCLUDES}
)
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
		drivers_board
		tarox_nuttx
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

set(TAROX_FLASH_ADDR "0x08000000" CACHE STRING "Load address for tarox.bin (MCU internal flash, hex or 0x...)")

find_program(TAROX_ST_FLASH st-flash)
if(NOT TAROX_ST_FLASH)
	set(TAROX_ST_FLASH st-flash)
endif()

set(TAROX_ST_FLASH_CONNECT_UNDER_RESET ON CACHE BOOL
	"Pass --connect-under-reset to st-flash (helps when SWD is blocked by running firmware)")
set(TAROX_ST_FLASH_FLAGS "")
if(TAROX_ST_FLASH_CONNECT_UNDER_RESET)
	list(APPEND TAROX_ST_FLASH_FLAGS --connect-under-reset)
endif()

add_custom_target(
	flash
	COMMAND bash -c "if [ ! -f '${TAROX_BINARY_OUTPUT}' ]; then echo 'tarox.bin not found; run: make <config>'; exit 1; fi"
	COMMAND ${TAROX_ST_FLASH} ${TAROX_ST_FLASH_FLAGS} write ${TAROX_BINARY_OUTPUT} ${TAROX_FLASH_ADDR}
	WORKING_DIRECTORY ${TAROX_BINARY_DIR}
	COMMENT "Flash tarox.bin using ST-Link (st-flash)"
	VERBATIM
)

add_custom_target(
	build-flash
	DEPENDS tarox_bin
	COMMENT "Build tarox.bin if needed, then flash"
)
add_custom_command(
	TARGET build-flash POST_BUILD
	COMMAND ${TAROX_ST_FLASH} ${TAROX_ST_FLASH_FLAGS} write ${TAROX_BINARY_OUTPUT} ${TAROX_FLASH_ADDR}
	WORKING_DIRECTORY ${TAROX_BINARY_DIR}
)

find_program(TAROX_OPENOCD openocd)
set(TAROX_OPENOCD_INTERFACE "interface/stlink.cfg" CACHE STRING "OpenOCD interface script name or path")
set(TAROX_OPENOCD_TARGET "target/stm32f4x.cfg" CACHE STRING "OpenOCD target script name or path")
if(TAROX_OPENOCD)
	add_custom_target(
		flash-openocd
		COMMAND bash -c "if [ ! -f '${TAROX_BINARY_OUTPUT}' ]; then echo 'tarox.bin not found; run: make <config>'; exit 1; fi"
		COMMAND
			${TAROX_OPENOCD} -f ${TAROX_OPENOCD_INTERFACE} -f ${TAROX_OPENOCD_TARGET} -c
			"program ${TAROX_BINARY_OUTPUT} ${TAROX_FLASH_ADDR} verify reset exit"
		WORKING_DIRECTORY ${TAROX_BINARY_DIR}
		COMMENT "Flash tarox.bin using OpenOCD"
		VERBATIM
	)

	add_custom_target(
		build-flash-openocd
		DEPENDS tarox_bin
		COMMENT "Build tarox.bin if needed, then flash via OpenOCD"
	)
	add_custom_command(
		TARGET build-flash-openocd POST_BUILD
		COMMAND
			${TAROX_OPENOCD} -f ${TAROX_OPENOCD_INTERFACE} -f ${TAROX_OPENOCD_TARGET} -c
			"program ${TAROX_BINARY_OUTPUT} ${TAROX_FLASH_ADDR} verify reset exit"
		WORKING_DIRECTORY ${TAROX_BINARY_DIR}
	)
endif()