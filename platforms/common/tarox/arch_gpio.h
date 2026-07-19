#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t tarox_gpio_t;

int tarox_arch_configgpio(tarox_gpio_t gpio);
void tarox_arch_gpiowrite(tarox_gpio_t gpio, bool value);
bool tarox_arch_gpioread(tarox_gpio_t gpio);

#ifdef __cplusplus
}
#endif

