#include <nuttx/config.h>

#ifndef CONFIG_DEV_GPIO
#  error "tarox_gpio requires NuttX CONFIG_DEV_GPIO"
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <nuttx/ioexpander/gpio.h>
#include <tarox_gpio.h>

extern "C"
{

int tarox_gpio_open(const char *path)
{
  return open(path, O_RDWR);
}

void tarox_gpio_close(int fd)
{
  if (fd >= 0)
    {
      close(fd);
    }
}

int tarox_gpio_write(int fd, bool value)
{
  if (fd < 0)
    {
      return -EINVAL;
    }

  return ioctl(fd, GPIOC_WRITE, (unsigned long)value);
}

} /* extern "C" */
