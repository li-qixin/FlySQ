#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

#include <nuttx/board.h>
#include <nuttx/spi/spi.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/sdio.h>
#include <nuttx/mmcsd.h>
#include <nuttx/analog/adc.h>
#include <stm32.h>

#ifdef CONFIG_PWM
extern int stm32_pwm_setup(void);
#endif

#if defined(CONFIG_I2C) && defined(CONFIG_I2C_DRIVER)
extern int stm32_i2c_setup(void);
#endif

#ifdef CONFIG_DEV_GPIO
extern int stm32_gpio_initialize(void);
#endif

/************************************************************************************
 * Name: stm32_boardinitialize
 *
 * Description:
 *   All STM32 architectures must provide the following entry point.  This entry
 * point is called early in the initialization -- after all memory has been
 * configured and mapped but before any devices have been initialized.
 *
 ************************************************************************************/

void stm32_boardinitialize(void)
{
}

/****************************************************************************
 * Name: board_app_initialize
 *
 * Description:
 *   Perform application specific initialization.  This function is never
 *   called directly from application code, but only indirectly via the
 *   (non-standard) boardctl() interface using the command BOARDIOC_INIT.
 *
 * Input Parameters:
 *   arg - The boardctl() argument is passed to the board_app_initialize()
 *         implementation without modification.  The argument has no
 *         meaning to NuttX; the meaning of the argument is a contract
 *         between the board-specific initalization logic and the the
 *         matching application logic.  The value cold be such things as a
 *         mode enumeration value, a set of DIP switch switch settings, a
 *         pointer to configuration data read from a file or serial FLASH,
 *         or whatever you would like to do with it.  Every implementation
 *         should accept zero/NULL as a default configuration.
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   any failure to indicate the nature of the failure.
 *
 ****************************************************************************/

int board_app_initialize(uintptr_t arg)
{
	(void)arg;
#ifdef CONFIG_DEV_GPIO
	{
		int gr;

		gr = stm32_gpio_initialize();
		if (gr < 0)
			{
				syslog(LOG_WARNING, "stm32_gpio_initialize failed: %d\n", gr);
			}
	}
#endif
#ifdef CONFIG_PWM
	{
		int pr;

		pr = stm32_pwm_setup();
		if (pr < 0)
			{
				syslog(LOG_WARNING, "stm32_pwm_setup failed: %d\n", pr);
			}
	}
#endif
#if defined(CONFIG_I2C) && defined(CONFIG_I2C_DRIVER)
	{
		int ir;

		ir = stm32_i2c_setup();
		if (ir < 0)
			{
				syslog(LOG_WARNING, "stm32_i2c_setup failed: %d\n", ir);
			}
	}
#endif
	return 0;
}