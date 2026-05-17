#pragma once

#include <stdbool.h>
#include <stdint.h>

#define TAROX_GPIO_FD_INVALID (-1)

#ifdef __cplusplus
extern "C" {
#endif

/* Device paths are board-specific (see board.h, e.g. TAROX_GPIO_DEMO_LED). */

int  tarox_gpio_open(const char *path);
void tarox_gpio_close(int fd);
int  tarox_gpio_write(int fd, bool value);

#ifdef __cplusplus
}
#endif
