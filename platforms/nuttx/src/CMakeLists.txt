add_library(
	tarox_nuttx
	STATIC
		log.cpp
		time.cpp
		tasks.cpp
		gpio.cpp
		io_timer.cpp
		pwm_servo.cpp
)

set_source_files_properties(
	log.cpp
	time.cpp
	tasks.cpp
	gpio.cpp
	io_timer.cpp
	pwm_servo.cpp
	PROPERTIES
	COMPILE_FLAGS "-fno-exceptions -fno-rtti -fno-unwind-tables"
)

target_include_directories(
	tarox_nuttx
	PRIVATE
		${TAROX_SOURCE_DIR}/platforms/common
		${TAROX_CONFIG_DIR}/nuttx-config/include
		${TAROX_PLATFORM_OS_INCLUDES}
)

target_link_libraries(
	tarox_nuttx
	PRIVATE
		nuttx_arch
		nuttx_drivers
)
