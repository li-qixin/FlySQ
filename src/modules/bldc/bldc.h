#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  BLDC_MODE_NONE = 0,
  BLDC_MODE_SPEED,
  BLDC_MODE_POSITION,
} bldc_mode_e;

int         bldc_init(void);
int         bldc_start(float elec_hz, float vq_pu);
int         bldc_goto_mech_deg(float mech_deg, float vq_pu);
int         bldc_hold(float vq_pu);
int         bldc_stop(void);
int         bldc_set_elec_hz(float elec_hz);
int         bldc_reverse(void);
bool        bldc_is_running(void);
bldc_mode_e bldc_get_mode(void);
float       bldc_get_target_mech_deg(void);
int         bldc_encoder_read(uint16_t *raw);
int         bldc_align(float vq_pu);
float       bldc_get_elec_offset_rad(void);
int         bldc_get_vq_sign(void);
bool        bldc_is_foc_calibrated(void);

#ifdef __cplusplus
}
#endif
