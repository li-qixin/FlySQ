#include <board.h>
#include <stm32.h>

extern "C" {
#include <tarox_gpio.h>

int tarox_gpio_set(uint32_t pinset, bool value)
{
	stm32_gpiowrite(pinset, value);
	return 0;
}

}
