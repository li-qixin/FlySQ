/****************************************************************************
 * Demo board: register named PWM devices (e.g. /dev/demo_pwm, /dev/bldc_pwm).
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <debug.h>

#include <nuttx/timers/pwm.h>
#include <arch/board/board.h>

#include "chip.h"
#include "stm32_pwm.h"

struct board_pwm_output_s
{
  uint8_t       timer;
  uint8_t       ngpios;
  uint32_t      gpios[3];
  const char   *devpath;
};

#define HAVE_PWM_DEMO 0
#define HAVE_PWM_BLDC 0

#ifndef CONFIG_PWM
#  undef HAVE_PWM_DEMO
#  undef HAVE_PWM_BLDC
#endif

#if defined(CONFIG_PWM) && defined(CONFIG_STM32_TIM4) && defined(CONFIG_STM32_TIM4_PWM)
#  undef HAVE_PWM_DEMO
#  define HAVE_PWM_DEMO 1
#endif

#if defined(CONFIG_PWM) && defined(CONFIG_STM32_TIM1) && defined(CONFIG_STM32_TIM1_PWM)
#  undef HAVE_PWM_BLDC
#  define HAVE_PWM_BLDC 1
#endif

#if defined(HAVE_PWM_DEMO)
#  ifndef CONFIG_STM32_PWM_MULTICHAN
#    if !defined(CONFIG_STM32_TIM4_CHANNEL) || CONFIG_STM32_TIM4_CHANNEL != BOARD_PWM_DEMO_CHANNEL
#      undef HAVE_PWM_DEMO
#      define HAVE_PWM_DEMO 0
#    endif
#  else
#    ifndef CONFIG_STM32_TIM4_CHANNEL2
#      undef HAVE_PWM_DEMO
#      define HAVE_PWM_DEMO 0
#    endif
#  endif
#endif

#if defined(HAVE_PWM_BLDC)
#  ifndef CONFIG_STM32_PWM_MULTICHAN
#    undef HAVE_PWM_BLDC
#    define HAVE_PWM_BLDC 0
#  endif
#  if !defined(CONFIG_STM32_TIM1_CHANNEL1) || !defined(CONFIG_STM32_TIM1_CHANNEL2) || !defined(CONFIG_STM32_TIM1_CHANNEL3)
#    undef HAVE_PWM_BLDC
#    define HAVE_PWM_BLDC 0
#  endif
#endif

#if HAVE_PWM_DEMO
static const struct board_pwm_output_s g_pwm_demo =
{
  BOARD_PWM_DEMO_TIMER,
  1,
  { BOARD_PWM_DEMO_GPIO, 0, 0 },
  TAROX_PWM_DEMO,
};
#endif

#if HAVE_PWM_BLDC
static const struct board_pwm_output_s g_pwm_bldc =
{
  BOARD_BLDC_TIMER,
  3,
  { BOARD_BLDC_GPIO_U, BOARD_BLDC_GPIO_V, BOARD_BLDC_GPIO_W },
  TAROX_BLDC_PWM,
};
#endif

static int board_pwm_register(const struct board_pwm_output_s *entry, bool *initialized)
{
  struct pwm_lowerhalf_s *pwm;
  unsigned int i;
  int ret;

  if (*initialized)
    {
      return OK;
    }

  for (i = 0; i < entry->ngpios; i++)
    {
      if (entry->gpios[i] != 0)
        {
          stm32_configgpio(entry->gpios[i]);
        }
    }

  pwm = stm32_pwminitialize(entry->timer);
  if (pwm == NULL)
    {
      aerr("ERROR: stm32_pwminitialize(%u) failed\n", entry->timer);
      return -ENODEV;
    }

  ret = pwm_register(entry->devpath, pwm);
  if (ret < 0)
    {
      aerr("ERROR: pwm_register(%s) failed: %d\n", entry->devpath, ret);
      return ret;
    }

  *initialized = true;
  return OK;
}

int stm32_pwm_setup(void)
{
  int ret = OK;

#if HAVE_PWM_DEMO
  static bool demo_initialized;

  ret = board_pwm_register(&g_pwm_demo, &demo_initialized);
  if (ret < 0)
    {
      return ret;
    }
#endif

#if HAVE_PWM_BLDC
  static bool bldc_initialized;

  ret = board_pwm_register(&g_pwm_bldc, &bldc_initialized);
  if (ret < 0)
    {
      return ret;
    }
#endif

#if !HAVE_PWM_DEMO && !HAVE_PWM_BLDC
  return -ENODEV;
#else
  return ret;
#endif
}
