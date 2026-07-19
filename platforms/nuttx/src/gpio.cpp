#include <tarox/arch_gpio.h>
#include <tarox/board_control.h>

#include <board.h>
#include <stm32_gpio.h>

extern "C"
{

int tarox_arch_configgpio(tarox_gpio_t gpio)
{
	return stm32_configgpio(gpio);
}

void tarox_arch_gpiowrite(tarox_gpio_t gpio, bool value)
{
	stm32_gpiowrite(gpio, value);
}

bool tarox_arch_gpioread(tarox_gpio_t gpio)
{
	return stm32_gpioread(gpio);
}

int tarox_board_control_set(enum tarox_board_control_id id, bool value)
{
	switch (id) {
	case TAROX_BOARD_CONTROL_BLDC_ENABLE:
		return tarox_arch_configgpio(BOARD_GPIO_BLDC_EN) == 0
		       ? (tarox_arch_gpiowrite(BOARD_GPIO_BLDC_EN, value), 0)
		       : -1;

	default:
		return -1;
	}
}

bool tarox_board_control_get(enum tarox_board_control_id id)
{
	switch (id) {
	case TAROX_BOARD_CONTROL_BLDC_ENABLE:
		return tarox_arch_gpioread(BOARD_GPIO_BLDC_EN);

	default:
		return false;
	}
}

}
