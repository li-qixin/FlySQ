#pragma once

#include <stdbool.h>
#include <stdint.h>

#define TAROX_PWM_FD_INVALID (-1)

#ifdef __cplusplus
extern "C" {
#endif

/* Device paths are board-specific (see board.h, e.g. TAROX_PWM_DEMO). */

int  tarox_pwm_open(const char *path);
void tarox_pwm_close(int fd);
int  tarox_pwm_run(int fd);
int  tarox_pwm_halt(int fd);

/* duty_permille: 0..1000 (thousandths of high time within one period). */

int tarox_pwm_apply(int fd, uint32_t freq_hz, uint32_t duty_permille);

/* Three-phase duties are 0.0..1.0 on TIM1 CH1/2/3. */

int tarox_pwm_apply_3(int fd, uint32_t freq_hz,
                      float duty_u, float duty_v, float duty_w);
int tarox_pwm_set_duties_3(int fd, float duty_u, float duty_v, float duty_w);

/* MS8313 EN and similar board hooks keyed by PWM device path. */

int tarox_pwm_driver_enable(const char *path, bool on);

#ifdef __cplusplus
}
#endif
