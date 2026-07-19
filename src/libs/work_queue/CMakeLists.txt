add_library(tarox_work_queue STATIC WorkQueue.cpp)

target_include_directories(
	tarox_work_queue
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${TAROX_SOURCE_DIR}/platforms/common
)
