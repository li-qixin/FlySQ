add_library(tarox_parameters STATIC param.cpp)

target_include_directories(
	tarox_parameters
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

