add_library(tarox_motor STATIC park.c svpwm.c)

target_include_directories(
  tarox_motor
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tarox_motor PUBLIC m)
