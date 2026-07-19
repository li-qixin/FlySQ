add_library(tarox_perf STATIC perf_counter.cpp)

target_include_directories(
	tarox_perf
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${TAROX_SOURCE_DIR}/platforms/common
)
