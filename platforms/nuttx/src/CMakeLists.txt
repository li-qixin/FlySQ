add_library(tarox_nuttx STATIC tarox_pwm.cpp)

set_source_files_properties(tarox_pwm.cpp PROPERTIES
	COMPILE_FLAGS "-fno-exceptions -fno-rtti -fno-unwind-tables"
)

target_include_directories(
	tarox_nuttx
	PRIVATE
		${TAROX_SOURCE_DIR}/platforms/common/include
		${TAROX_CONFIG_DIR}/nuttx-config/include
		${TAROX_PLATFORM_OS_INCLUDES}
)

target_link_libraries(
	tarox_nuttx
	PRIVATE
		nuttx_arch
		nuttx_drivers
)
