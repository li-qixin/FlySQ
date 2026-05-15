tarox_add_module(
  MODULE led_demo
  MAIN led_demo_main
  STACK 2048
  PRIORITY 100
  SRCS
    led_demo.c
  DEPENDS
    tarox_platform
)
