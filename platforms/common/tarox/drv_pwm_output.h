#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAROX_PWM_OUTPUT_MAX_CHANNELS 8

int tarox_pwm_servo_init(uint32_t freq_hz, uint32_t channel_count);
int tarox_pwm_servo_deinit(void);
int tarox_pwm_servo_arm(void);
int tarox_pwm_servo_disarm(void);
int tarox_pwm_duty_set(uint32_t channel, float duty);
int tarox_pwm_duty_set3(float duty_u, float duty_v, float duty_w);

#ifdef __cplusplus
}
#endif

