add_library(tarox_actuator INTERFACE)

target_include_directories(
	tarox_actuator
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(tarox_actuator INTERFACE tarox_msg tarox_uorb)

