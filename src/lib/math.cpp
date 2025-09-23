#include "math.h"

namespace flysq {
float constrain(float val, float limit_low_bound, float limit_up_bound) {
  return min(limit_up_bound, max(val, limit_low_bound));
}
float max(float val1, float val2) {
  return val1 > val2 ? val1 : val2;
}
float min(float val1, float val2) {
  return val1 < val2 ? val1 : val2;
}

}  // namespace flysq
