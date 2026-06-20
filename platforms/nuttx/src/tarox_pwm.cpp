#include <nuttx/config.h>

#ifndef CONFIG_PWM
#  error "tarox_pwm requires NuttX CONFIG_PWM"
#endif

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <fixedmath.h>
#include <nuttx/timers/pwm.h>
#include <tarox_pwm.h>

#include <board.h>

extern "C"
{

static uint32_t g_pwm3_freq_hz;

static ub16_t permille_to_driver_duty(uint32_t permille)
{
  if (permille == 0)
    {
      return 1;
    }

  return b16divi(uitoub16(permille), 1000);
}

static ub16_t float_to_driver_duty(float duty)
{
  if (duty <= 0.0f)
    {
      return 1;
    }

  if (duty >= 1.0f)
    {
      return b16ONE;
    }

  return (ub16_t)(duty * 65536.0f);
}

static void pwm_set_demo_channel(struct pwm_info_s *info, uint32_t duty)
{
#ifdef CONFIG_PWM_MULTICHAN
  info->channels[0].channel = BOARD_PWM_DEMO_CHANNEL;
  info->channels[0].duty    = duty;
  info->channels[0].cpol    = PWM_CPOL_LOW;
  info->channels[0].dcpol   = PWM_DCPOL_LOW;
#else
  info->duty  = duty;
  info->cpol  = PWM_CPOL_LOW;
  info->dcpol = PWM_DCPOL_LOW;
#endif
}

static void pwm_set_3channels(struct pwm_info_s *info,
                              float duty_u, float duty_v, float duty_w)
{
#ifdef CONFIG_PWM_MULTICHAN
  info->channels[0].channel = 1;
  info->channels[0].duty    = float_to_driver_duty(duty_u);
  info->channels[0].cpol    = PWM_CPOL_LOW;
  info->channels[0].dcpol   = PWM_DCPOL_LOW;

  info->channels[1].channel = 2;
  info->channels[1].duty    = float_to_driver_duty(duty_v);
  info->channels[1].cpol    = PWM_CPOL_LOW;
  info->channels[1].dcpol   = PWM_DCPOL_LOW;

  info->channels[2].channel = 3;
  info->channels[2].duty    = float_to_driver_duty(duty_w);
  info->channels[2].cpol    = PWM_CPOL_LOW;
  info->channels[2].dcpol   = PWM_DCPOL_LOW;
#else
  (void)duty_u;
  (void)duty_v;
  (void)duty_w;
#endif
}

int tarox_pwm_open(const char *path)
{
  return open(path, O_RDONLY);
}

void tarox_pwm_close(int fd)
{
  if (fd >= 0)
    {
      close(fd);
    }
}

int tarox_pwm_apply(int fd, uint32_t freq_hz, uint32_t duty_permille)
{
  struct pwm_info_s info;

  if (fd < 0)
    {
      return -EINVAL;
    }

  if (duty_permille > 1000)
    {
      return -EINVAL;
    }

  memset(&info, 0, sizeof(info));
  info.frequency = freq_hz;
  pwm_set_demo_channel(&info, permille_to_driver_duty(duty_permille));

  return ioctl(fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)(uintptr_t)&info);
}

int tarox_pwm_apply_3(int fd, uint32_t freq_hz,
                      float duty_u, float duty_v, float duty_w)
{
  struct pwm_info_s info;

  if (fd < 0)
    {
      return -EINVAL;
    }

#ifndef CONFIG_PWM_MULTICHAN
  return -ENOTSUP;
#else
  g_pwm3_freq_hz = freq_hz;

  memset(&info, 0, sizeof(info));
  info.frequency = freq_hz;
  pwm_set_3channels(&info, duty_u, duty_v, duty_w);

  return ioctl(fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)(uintptr_t)&info);
#endif
}

int tarox_pwm_set_duties_3(int fd, float duty_u, float duty_v, float duty_w)
{
  struct pwm_info_s info;

  if (fd < 0)
    {
      return -EINVAL;
    }

#ifndef CONFIG_PWM_MULTICHAN
  return -ENOTSUP;
#else
  if (g_pwm3_freq_hz == 0)
    {
      return -EINVAL;
    }

  memset(&info, 0, sizeof(info));
  info.frequency = g_pwm3_freq_hz;
  pwm_set_3channels(&info, duty_u, duty_v, duty_w);

  return ioctl(fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)(uintptr_t)&info);
#endif
}

int tarox_pwm_run(int fd)
{
  if (fd < 0)
    {
      return -EINVAL;
    }

  return ioctl(fd, PWMIOC_START, 0);
}

int tarox_pwm_halt(int fd)
{
  if (fd < 0)
    {
      return -EINVAL;
    }

  return ioctl(fd, PWMIOC_STOP, 0);
}

} /* extern "C" */
