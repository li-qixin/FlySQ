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
#define TAROX_DEV_BASENAME(devpath) ((devpath) + 5)

struct board_gpio_output_s
{
  uint32_t      pinset;
  const char   *devpath;
};

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
    BOARD_GPIO_DEMO_LED,
    TAROX_GPIO_DEMO_LED,
  },
#ifdef TAROX_GPIO_BLDC_EN
  {
    BOARD_GPIO_BLDC_EN,
    TAROX_GPIO_BLDC_EN,
  },
#endif
};

#define BOARD_NGPIOOUT  (sizeof(g_outputs) / sizeof(g_outputs[0]))

static struct stm32gpio_dev_s g_gpout[BOARD_NGPIOOUT];

static int gpout_read(struct gpio_dev_s *dev, bool *value)
{
  struct stm32gpio_dev_s *stm32gpio = (struct stm32gpio_dev_s *)dev;

  DEBUGASSERT(stm32gpio != NULL && value != NULL);
  DEBUGASSERT(stm32gpio->id < BOARD_NGPIOOUT);

  *value = stm32_gpioread(g_outputs[stm32gpio->id].pinset);
  return OK;
}

static int gpout_write(struct gpio_dev_s *dev, bool value)
{
  struct stm32gpio_dev_s *stm32gpio = (struct stm32gpio_dev_s *)dev;

  DEBUGASSERT(stm32gpio != NULL);
  DEBUGASSERT(stm32gpio->id < BOARD_NGPIOOUT);

  stm32_gpiowrite(g_outputs[stm32gpio->id].pinset, value);
  return OK;
}

int stm32_gpio_initialize(void)
{
  unsigned int i;
  uint32_t pinset;

  for (i = 0; i < BOARD_NGPIOOUT; i++)
    {
      pinset = g_outputs[i].pinset;

      g_gpout[i].gpio.gp_pintype = GPIO_OUTPUT_PIN;
      g_gpout[i].gpio.gp_ops     = &gpout_ops;
      g_gpout[i].id              = i;
      gpio_pin_register_byname(&g_gpout[i].gpio,
                               TAROX_DEV_BASENAME(g_outputs[i].devpath));

      stm32_gpiowrite(pinset, 0);
      stm32_configgpio(pinset);
    }

  return OK;
}

#endif /* CONFIG_DEV_GPIO && !CONFIG_GPIO_LOWER_HALF */
