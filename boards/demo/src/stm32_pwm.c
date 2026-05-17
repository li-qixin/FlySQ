/****************************************************************************
 * Demo board: register named PWM devices (e.g. /dev/demo_pwm).
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
  uint8_t       channel;
  uint32_t      gpio;
  const char   *devpath;
};

#define HAVE_PWM 1

#ifndef CONFIG_PWM
#  undef HAVE_PWM
#endif

#ifndef CONFIG_STM32_TIM4
#  undef HAVE_PWM
#endif

#ifndef CONFIG_STM32_TIM4_PWM
#  undef HAVE_PWM
#endif

static const struct board_pwm_output_s g_pwms[] =
{
  {
    4,
    2,
    GPIO_TIM4_CH2OUT_2,
    "/dev/demo_pwm",
  },
};

#if !defined(CONFIG_STM32_TIM4_CHANNEL) || CONFIG_STM32_TIM4_CHANNEL != 2
#  undef HAVE_PWM
#endif

#define BOARD_NPWM  (sizeof(g_pwms) / sizeof(g_pwms[0]))

int stm32_pwm_setup(void)
{
#ifdef HAVE_PWM
  static bool initialized[BOARD_NPWM];
  struct pwm_lowerhalf_s *pwm;
  unsigned int i;
  int ret;

  for (i = 0; i < BOARD_NPWM; i++)
    {
      if (initialized[i])
        {
          continue;
        }

      stm32_configgpio(g_pwms[i].gpio);

      pwm = stm32_pwminitialize(g_pwms[i].timer);
      if (pwm == NULL)
        {
          aerr("ERROR: stm32_pwminitialize(%u) failed\n",
               g_pwms[i].timer);
          return -ENODEV;
        }

      ret = pwm_register(g_pwms[i].devpath, pwm);
      if (ret < 0)
        {
          aerr("ERROR: pwm_register(%s) failed: %d\n",
               g_pwms[i].devpath, ret);
          return ret;
        }

      initialized[i] = true;
    }

  return OK;
#else
  return -ENODEV;
#endif
}
