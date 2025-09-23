#pragma once
#include "control_state.h"
namespace flysq {
namespace modules {
class Controller {
 public:
  Controller() = default;
  virtual ~Controller() = default;

  virtual ControlState ComputeControlSignal(const ControlState& target,
                                            const ControlState& meas) = 0;
};
}  // namespace modules
}  // namespace flysq