#include "led_demo.h"

#include <board.h>
#include <tarox_gpio.h>

#include <string.h>
#include <stdbool.h>

int led_demo_main(int argc, char *argv[])
{
  if (argc < 2)
    {
      return -1;
    }

  if (strcmp(argv[1], "on") == 0)
    {
      return tarox_gpio_set(GPIO_DEMO_LED, true);
    }

  if (strcmp(argv[1], "off") == 0)
    {
      return tarox_gpio_set(GPIO_DEMO_LED, false);
    }

  return -1;
}
