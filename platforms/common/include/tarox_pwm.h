#pragma once

#include <stdint.h>

#define TAROX_PWM_FD_INVALID (-1)

#ifdef __cplusplus
extern "C" {
#endif

/* Logical device path for the board’s PWM export (e.g. /dev/pwm0). */

const char *tarox_pwm_default_device_path(void);

int  tarox_pwm_open(const char *path);
void tarox_pwm_close(int fd);

/* duty_permille: 0..1000 (thousandths of high time within one period). */

int tarox_pwm_apply(int fd, uint32_t freq_hz, uint32_t duty_permille);
int tarox_pwm_run(int fd);
int tarox_pwm_halt(int fd);

#ifdef __cplusplus
}
#endif
