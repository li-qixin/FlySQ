/****************************************************************************
 * Demo board: register named GPIO outputs (e.g. /dev/demo_led).
 ****************************************************************************/

#include <nuttx/config.h>

#if defined(CONFIG_DEV_GPIO) && !defined(CONFIG_GPIO_LOWER_HALF)

#include <stdbool.h>
#include <debug.h>

#include <nuttx/ioexpander/gpio.h>

#include <arch/board/board.h>

#include "chip.h"
#include "stm32_gpio.h"

struct board_gpio_output_s
{
  uint32_t      cfg;
  uint32_t      port;
  uint32_t      pin;
  const char   *name;
};

#define BOARD_GPIO_PINSET(c, p, n)  ((c) | (p) | (n))

struct stm32gpio_dev_s
{
  struct gpio_dev_s gpio;
  uint8_t id;
};

static int gpout_read(struct gpio_dev_s *dev, bool *value);
static int gpout_write(struct gpio_dev_s *dev, bool value);

static const struct gpio_operations_s gpout_ops =
{
  .go_read   = gpout_read,
  .go_write  = gpout_write,
  .go_attach = NULL,
  .go_enable = NULL,
};

static const struct board_gpio_output_s g_outputs[] =
{
  {
    GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_50MHz | GPIO_OUTPUT_CLEAR,
    GPIO_PORTA,
    GPIO_PIN5,
    "demo_led",
  },
};

#define BOARD_NGPIOOUT  (sizeof(g_outputs) / sizeof(g_outputs[0]))

static struct stm32gpio_dev_s g_gpout[BOARD_NGPIOOUT];

static uint32_t board_gpio_pinset(unsigned int index)
{
  DEBUGASSERT(index < BOARD_NGPIOOUT);

  return BOARD_GPIO_PINSET(g_outputs[index].cfg,
                           g_outputs[index].port,
                           g_outputs[index].pin);
}

static int gpout_read(struct gpio_dev_s *dev, bool *value)
{
  struct stm32gpio_dev_s *stm32gpio = (struct stm32gpio_dev_s *)dev;

  DEBUGASSERT(stm32gpio != NULL && value != NULL);
  DEBUGASSERT(stm32gpio->id < BOARD_NGPIOOUT);

  *value = stm32_gpioread(board_gpio_pinset(stm32gpio->id));
  return OK;
}

static int gpout_write(struct gpio_dev_s *dev, bool value)
{
  struct stm32gpio_dev_s *stm32gpio = (struct stm32gpio_dev_s *)dev;

  DEBUGASSERT(stm32gpio != NULL);
  DEBUGASSERT(stm32gpio->id < BOARD_NGPIOOUT);

  stm32_gpiowrite(board_gpio_pinset(stm32gpio->id), value);
  return OK;
}

int stm32_gpio_initialize(void)
{
  unsigned int i;
  uint32_t pinset;

  for (i = 0; i < BOARD_NGPIOOUT; i++)
    {
      pinset = board_gpio_pinset(i);

      g_gpout[i].gpio.gp_pintype = GPIO_OUTPUT_PIN;
      g_gpout[i].gpio.gp_ops     = &gpout_ops;
      g_gpout[i].id              = i;
      gpio_pin_register_byname(&g_gpout[i].gpio, g_outputs[i].name);

      stm32_gpiowrite(pinset, 0);
      stm32_configgpio(pinset);
    }

  return OK;
}

#endif /* CONFIG_DEV_GPIO && !CONFIG_GPIO_LOWER_HALF */
