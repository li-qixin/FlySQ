tarox_add_module(
  MODULE pwm_demo
  MAIN pwm_demo_main
  STACK 2048
  PRIORITY 100
  SRCS
    pwm_demo.c
  DEPENDS
    tarox_platform
)
