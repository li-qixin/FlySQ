#include <motor/park.h>

#include <math.h>

void motor_inv_park(float vd, float vq, float theta, float *va, float *vb)
{
  float c = cosf(theta);
  float s = sinf(theta);

  *va = vd * c - vq * s;
  *vb = vd * s + vq * c;
}
