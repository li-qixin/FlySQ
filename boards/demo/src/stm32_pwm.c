/****************************************************************************
 * Demo board: register /dev/pwm0 (TIM4 CH2, see board.h).
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <debug.h>

#include <nuttx/timers/pwm.h>
#include <arch/board/board.h>

#include "chip.h"
#include "stm32_pwm.h"

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

#if !defined(CONFIG_STM32_TIM4_CHANNEL) || CONFIG_STM32_TIM4_CHANNEL != DEMO_BOARD_PWMCHANNEL
#  undef HAVE_PWM
#endif

int stm32_pwm_setup(void)
{
#ifdef HAVE_PWM
  static bool initialized;
  struct pwm_lowerhalf_s *pwm;
  int ret;

  if (initialized)
    {
      return OK;
    }

  pwm = stm32_pwminitialize(DEMO_BOARD_PWMTIMER);
  if (pwm == NULL)
    {
      aerr("ERROR: stm32_pwminitialize(%d) failed\n", DEMO_BOARD_PWMTIMER);
      return -ENODEV;
    }

  ret = pwm_register("/dev/pwm0", pwm);
  if (ret < 0)
    {
      aerr("ERROR: pwm_register failed: %d\n", ret);
      return ret;
    }

  initialized = true;
  return OK;
#else
  return -ENODEV;
#endif
}
