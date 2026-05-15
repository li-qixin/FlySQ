#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <tarox_pwm.h>

#ifdef CONFIG_PWM

#include <fixedmath.h>
#include <nuttx/timers/pwm.h>

#endif

extern "C"
{

#ifdef CONFIG_PWM

static ub16_t permille_to_driver_duty(uint32_t permille)
{
  if (permille == 0)
    {
      return 1;
    }

  /* duty = permille/1000 as ub16 fractional duty; avoid (permille-1)/ skew. */

  return b16divi(uitoub16(permille), 1000);
}

const char *tarox_pwm_default_device_path(void)
{
  return "/dev/pwm0";
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

#else /* !CONFIG_PWM */

const char *tarox_pwm_default_device_path(void)
{
  return "/dev/pwm0";
}

int tarox_pwm_open(const char *path)
{
  (void)path;
  return TAROX_PWM_FD_INVALID;
}

void tarox_pwm_close(int fd)
{
  (void)fd;
}

int tarox_pwm_apply(int fd, uint32_t freq_hz, uint32_t duty_permille)
{
  (void)fd;
  (void)freq_hz;
  (void)duty_permille;
  return -ENODEV;
}

int tarox_pwm_run(int fd)
{
  (void)fd;
  return -ENODEV;
}

int tarox_pwm_halt(int fd)
{
  (void)fd;
  return -ENODEV;
}

#endif /* CONFIG_PWM */

} /* extern "C" */
