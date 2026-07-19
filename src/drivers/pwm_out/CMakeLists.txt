tarox_add_module(
  MODULE pwm_out
  MAIN pwm_out_main
  STACK 2048
  PRIORITY 100
  SRCS
    PWMOut.cpp
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${NUTTX_KERNEL_DIR}/include
  DEPENDS
    tarox_platform
    tarox_uorb
    nuttx_context
)
