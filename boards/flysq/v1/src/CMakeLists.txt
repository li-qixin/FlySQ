add_library(drivers_board init.c)

target_link_libraries(
  drivers_board
  PRIVATE
    nuttx_arch
    nuttx_drivers 
)