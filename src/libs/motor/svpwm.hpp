#pragma once

namespace tarox
{

struct PhaseDuty
{
  float u;
  float v;
  float w;
};

class Svpwm
{
public:
  static PhaseDuty modulate(float alpha, float beta);
};

} // namespace tarox
