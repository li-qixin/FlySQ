tarox_add_module(
  MODULE as5600
  MAIN as5600_main
  STACK 2048
  PRIORITY 100
  SRCS
    as5600.cpp
  INCLUDES
    ${NUTTX_KERNEL_DIR}/include
  DEPENDS
    tarox_platform
    tarox_uorb
    nuttx_context
)
