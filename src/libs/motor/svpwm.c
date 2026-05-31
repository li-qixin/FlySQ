#include <motor/svpwm.h>

#include <math.h>

void motor_svpwm(float va, float vb, float *du, float *dv, float *dw)
{
  const float k = 0.8660254037844386f; /* sqrt(3)/2 */
  float vu = va;
  float vv = -0.5f * va + k * vb;
  float vw = -0.5f * va - k * vb;
  float v_max;
  float v_min;
  float offset;

  v_max = fmaxf(vu, fmaxf(vv, vw));
  v_min = fminf(vu, fminf(vv, vw));
  offset = 0.5f * (v_max + v_min);

  *du = vu - offset + 0.5f;
  *dv = vv - offset + 0.5f;
  *dw = vw - offset + 0.5f;

  if (*du < 0.0f)
    {
      *du = 0.0f;
    }
  else if (*du > 1.0f)
    {
      *du = 1.0f;
    }

  if (*dv < 0.0f)
    {
      *dv = 0.0f;
    }
  else if (*dv > 1.0f)
    {
      *dv = 1.0f;
    }

  if (*dw < 0.0f)
    {
      *dw = 0.0f;
    }
  else if (*dw > 1.0f)
    {
      *dw = 1.0f;
    }
}
