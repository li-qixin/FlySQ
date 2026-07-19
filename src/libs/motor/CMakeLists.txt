add_library(tarox_motor STATIC park.cpp svpwm.cpp)

target_include_directories(
  tarox_motor
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tarox_motor PUBLIC m)
