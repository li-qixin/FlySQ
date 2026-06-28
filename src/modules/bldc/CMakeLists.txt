tarox_add_module(
  MODULE bldc
  MAIN bldc_main
  STACK 2048
  PRIORITY 100
  SRCS
    bldc.c
    bldc_ctrl.c
    ${TAROX_SOURCE_DIR}/src/libs/as5600/as5600.c
  INCLUDES
    ${TAROX_SOURCE_DIR}/src/libs/motor/include
    ${TAROX_SOURCE_DIR}/src/libs/as5600/include
    ${NUTTX_KERNEL_DIR}/include
  DEPENDS
    tarox_platform
    tarox_motor
    nuttx_context
)
