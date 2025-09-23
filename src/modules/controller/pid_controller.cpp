#include "pid_controller.h"
#include "math.h"
#include "time.h"
namespace flysq {
namespace modules {

PIDController::PIDController(float P, float I, float D, float output_ramp,
                             float limit)
    : P_(P),
      I_(I),
      D_(D),
      output_ramp_(output_ramp),
      limit_(limit),
      prev_integral_(0.0),
      prev_error_(0.0),
      prev_output_(0.0),
      prev_timestamp_(0) {}

ControlState PIDController::ComputeControlSignal(const ControlState& target,
                                                 const ControlState& meas) {

  microsecond curr_timestamp = clock_us();
  float dt = (curr_timestamp - prev_timestamp_) * 1e-6;
  float error = target - meas;

  float proportional = P_ * error;

  float integral = prev_integral_ + I_ * dt * 0.5f * (error + prev_error_);
  integral = constrain(integral, -limit_, limit_);

  float derivative = D_ * (error - prev_error_) / dt;

  float output = proportional + integral + derivative;
  output = constrain(output, -limit_, limit_);

  if (output_ramp_ > 0.0f) {
    float output_rate = (output - prev_output_) / dt;
    if (output_rate > output_ramp_) {
      output = prev_output_ + dt * output_ramp_;
    } else if (output_rate < -output_ramp_) {
      output = prev_output_ - dt * output_ramp_;
    }
  }
  prev_integral_ = integral;
  prev_error_ = error;
  prev_output_ = output;
  prev_timestamp_ = curr_timestamp;
  return output;
}

}  // namespace modules

}  // namespace flysq