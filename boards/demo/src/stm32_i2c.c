/****************************************************************************
 * Demo board: register I2C1 on PB8/PB9 for AS5600.
 ****************************************************************************/

#include <nuttx/config.h>

#if defined(CONFIG_I2C) && defined(CONFIG_I2C_DRIVER)

#include <debug.h>
#include <errno.h>
#include <syslog.h>

#include <nuttx/i2c/i2c_master.h>

#include "chip.h"
#include "stm32_i2c.h"

static int stm32_i2c_register_bus(int bus)
{
  struct i2c_master_s *i2c;
  int ret;

  i2c = stm32_i2cbus_initialize(bus);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "stm32_i2cbus_initialize(%d) failed\n", bus);
      return -ENODEV;
    }

  ret = i2c_register(i2c, bus);
  if (ret < 0)
    {
      syslog(LOG_ERR, "i2c_register(%d) failed: %d\n", bus, ret);
      stm32_i2cbus_uninitialize(i2c);
      return ret;
    }

  return OK;
}

int stm32_i2c_setup(void)
{
#ifdef CONFIG_STM32_I2C1
  {
    int ret;

    ret = stm32_i2c_register_bus(1);
    if (ret < 0)
      {
        return ret;
      }
  }
#endif

  return OK;
}

#endif /* CONFIG_I2C && CONFIG_I2C_DRIVER */
