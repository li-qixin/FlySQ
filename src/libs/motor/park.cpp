#include "park.hpp"

namespace tarox
{

AlphaBeta Park::inverse(float vd, float vq, float theta)
{
  float c = __builtin_cosf(theta);
  float s = __builtin_sinf(theta);

  return {
    vd * c - vq * s,
    vd * s + vq * c,
  };
}

} // namespace tarox
