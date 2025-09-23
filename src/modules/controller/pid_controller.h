#pragma once
#include "controller.h"
namespace flysq {
namespace modules {

class PIDController : public Controller {
 public:
  /**
       *  
       * @param P - Proportional gain 
       * @param I - Integral gain
       * @param D - Derivative gain 
       * @param ramp - Maximum speed of change of the output value
       * @param limit - Maximum output value
       */
  PIDController(float P, float I, float D, float ramp, float limit);
  ~PIDController() = default;

  virtual ControlState ComputeControlSignal(const ControlState& target,
                                            const ControlState& meas) override;

 private:
  float P_;
  float I_;
  float D_;
  float output_ramp_;
  float limit_;
  float prev_integral_;           //!< last integral component value
  float prev_error_;              //!< last tracking error value
  unsigned long prev_timestamp_;  //!< Last execution timestamp
  float prev_output_;             //!< last pid output value
};
}  // namespace modules
}  // namespace flysq