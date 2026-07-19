add_library(tarox_uorb STATIC uORBTopics.cpp)

target_include_directories(
	tarox_uorb
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${TAROX_SOURCE_DIR}/src/libs/work_queue
)

target_link_libraries(tarox_uorb PUBLIC tarox_msg)
