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

extern "C"
{

static ub16_t permille_to_driver_duty(uint32_t permille)
{
  if (permille == 0)
    {
      return 1;
    }

  /* duty = permille/1000 as ub16 fractional duty; avoid (permille-1)/ skew. */

  return b16divi(uitoub16(permille), 1000);
}

int tarox_pwm_open(const char *path)
{
  int fd;

  fd = open(path, O_RDONLY);
  return fd;
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
  info.duty = permille_to_driver_duty(duty_permille);
  info.cpol = PWM_CPOL_LOW;
  info.dcpol = PWM_DCPOL_LOW;

  return ioctl(fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)(uintptr_t)&info);
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
