tarox_add_module(
  MODULE bldc
  MAIN bldc_main
  STACK 2048
  PRIORITY 100
  SRCS
    bldc.c
    bldc_ctrl.c
  INCLUDES
    ${TAROX_SOURCE_DIR}/src/libs/motor/include
  DEPENDS
    tarox_platform
    tarox_motor
)
