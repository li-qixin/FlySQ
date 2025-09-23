#pragma once
#include <Eigen/Dense>

namespace flysq {
namespace modules {

class ControlState {
 public:
  ControlState() = default;
  ControlState(float data);
  ControlState(double data);
  ControlState(const Eigen::VectorXf& data);
  ControlState(const Eigen::VectorXd& data);
  ~ControlState() = default;

  ControlState& operator=(float data);
  ControlState& operator=(double data);
  ControlState& operator=(Eigen::VectorXf data);
  ControlState& operator=(Eigen::VectorXd data);
  ControlState operator-(const ControlState& state) const;

  operator float() const;
  operator double() const;
  operator Eigen::VectorXf() const;
  operator Eigen::VectorXd() const;

 private:
  float f_data_;
  double d_data_;
  Eigen::VectorXf eigen_f_data_;
  Eigen::VectorXd eigen_d_data_;
};
}  // namespace modules
}  // namespace flysq