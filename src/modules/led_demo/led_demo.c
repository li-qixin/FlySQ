#include "led_demo.h"

#include <board.h>
#include <tarox_gpio.h>

#include <string.h>

int led_demo_main(int argc, char *argv[])
{
  int fd;
  int ret;
  bool on;

  if (argc < 2)
    {
      return -1;
    }

  if (strcmp(argv[1], "on") == 0)
    {
      on = true;
    }
  else if (strcmp(argv[1], "off") == 0)
    {
      on = false;
    }
  else
    {
      return -1;
    }

  fd = tarox_gpio_open(TAROX_GPIO_DEMO_LED);
  if (fd < 0)
    {
      return fd;
    }

  ret = tarox_gpio_write(fd, on);
  tarox_gpio_close(fd);
  return ret;
}
