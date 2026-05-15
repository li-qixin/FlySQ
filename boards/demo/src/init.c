#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <debug.h>
#include <errno.h>
#include <syslog.h>

#include <nuttx/board.h>
#include <nuttx/spi/spi.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/sdio.h>
#include <nuttx/mmcsd.h>
#include <nuttx/analog/adc.h>
#include <board.h>
#include <stm32.h>

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
	stm32_configgpio(GPIO_DEMO_LED);
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
	return 0;
}