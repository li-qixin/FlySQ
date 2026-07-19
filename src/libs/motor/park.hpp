#pragma once

namespace tarox
{

struct AlphaBeta
{
  float alpha;
  float beta;
};

class Park
{
public:
  static AlphaBeta inverse(float vd, float vq, float theta);
};

} // namespace tarox
