#include "svpwm.hpp"

namespace tarox
{

PhaseDuty Svpwm::modulate(float alpha, float beta)
{
  const float k = 0.8660254037844386f; /* sqrt(3)/2 */
  float vu = alpha;
  float vv = -0.5f * alpha + k * beta;
  float vw = -0.5f * alpha - k * beta;
  float v_max;
  float v_min;
  float offset;
  PhaseDuty duty;

  v_max = __builtin_fmaxf(vu, __builtin_fmaxf(vv, vw));
  v_min = __builtin_fminf(vu, __builtin_fminf(vv, vw));
  offset = 0.5f * (v_max + v_min);

  duty.u = vu - offset + 0.5f;
  duty.v = vv - offset + 0.5f;
  duty.w = vw - offset + 0.5f;

  if (duty.u < 0.0f)
    {
      duty.u = 0.0f;
    }
  else if (duty.u > 1.0f)
    {
      duty.u = 1.0f;
    }

  if (duty.v < 0.0f)
    {
      duty.v = 0.0f;
    }
  else if (duty.v > 1.0f)
    {
      duty.v = 1.0f;
    }

  if (duty.w < 0.0f)
    {
      duty.w = 0.0f;
    }
  else if (duty.w > 1.0f)
    {
      duty.w = 1.0f;
    }

  return duty;
}

} // namespace tarox
