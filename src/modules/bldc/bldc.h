#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int  bldc_init(void);
int  bldc_start(float elec_hz, float vq_pu);
int  bldc_stop(void);
int  bldc_set_elec_hz(float elec_hz);
int  bldc_reverse(void);
bool bldc_is_running(void);

#ifdef __cplusplus
}
#endif
