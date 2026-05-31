#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void motor_inv_park(float vd, float vq, float theta, float *va, float *vb);

#ifdef __cplusplus
}
#endif
