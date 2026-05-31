#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void motor_svpwm(float va, float vb, float *du, float *dv, float *dw);

#ifdef __cplusplus
}
#endif
