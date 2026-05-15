add_library(drivers_board init.c board_drivers.cpp)

set_source_files_properties(board_drivers.cpp PROPERTIES
  COMPILE_FLAGS "-fno-exceptions -fno-rtti -fno-unwind-tables"
)

set(TAROX_BOARD_OS_INCLUDES
    ${NUTTX_KERNEL_DIR}/arch/${CONFIG_ARCH}/src
    ${NUTTX_KERNEL_DIR}/arch/${CONFIG_ARCH}/src/common
    ${NUTTX_KERNEL_DIR}/arch/${CONFIG_ARCH}/src/stm32
)
set(TAROX_PLATFORM_OS_INCLUDES ${TAROX_BOARD_OS_INCLUDES} PARENT_SCOPE)

target_include_directories(
  drivers_board
  PRIVATE
    ${TAROX_SOURCE_DIR}/platforms/common/include
    ${TAROX_CONFIG_DIR}/nuttx-config/include
    ${TAROX_BOARD_OS_INCLUDES}
)

target_link_libraries(
  drivers_board
  PRIVATE
    nuttx_arch
    nuttx_drivers 
)

get_property(TAROX_MODULE_LIBRARIES GLOBAL PROPERTY TAROX_MODULE_LIBRARIES)
if(TAROX_MODULE_LIBRARIES)
  target_link_libraries(drivers_board PRIVATE ${TAROX_MODULE_LIBRARIES})
endif()