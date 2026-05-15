#include "led_demo.h"

#include <string.h>
#include <stdbool.h>

int board_demo_led_set(bool on);

int led_demo_main(int argc, char *argv[])
{
  if (argc < 2)
    {
      return -1;
    }

  if (strcmp(argv[1], "on") == 0)
    {
      return board_demo_led_set(true);
    }

  if (strcmp(argv[1], "off") == 0)
    {
      return board_demo_led_set(false);
    }

  return -1;
}
