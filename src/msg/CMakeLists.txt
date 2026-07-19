add_library(tarox_msg INTERFACE)

target_include_directories(
	tarox_msg
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}/generated
)

