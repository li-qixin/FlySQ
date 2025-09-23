#include "control_state.h"

namespace flysq {
namespace modules {

ControlState::ControlState(float data) : f_data_(data) {}
ControlState::ControlState(double data) : d_data_(data) {}
ControlState::ControlState(const Eigen::VectorXf& data) : eigen_f_data_(data) {}
ControlState::ControlState(const Eigen::VectorXd& data) : eigen_d_data_(data) {}

ControlState& ControlState::operator=(float data) {
  this->f_data_ = data;
  return *this;
}
ControlState& ControlState::operator=(double data) {
  this->d_data_ = data;
  return *this;
}
ControlState& ControlState::operator=(Eigen::VectorXf data) {
  this->eigen_f_data_ = data;
  return *this;
}
ControlState& ControlState::operator=(Eigen::VectorXd data) {
  this->eigen_d_data_ = data;
  return *this;
}

ControlState ControlState::operator-(const ControlState& state) const {
  ControlState res;
  res.f_data_ = this->f_data_ - state.f_data_;
  res.d_data_ = this->d_data_ - state.d_data_;
  res.eigen_f_data_ = this->eigen_f_data_ - state.eigen_f_data_;
  res.eigen_d_data_ = this->eigen_d_data_ - state.eigen_d_data_;
  return res;
}

ControlState::operator float() const {
  return f_data_;
}
ControlState::operator double() const {
  return d_data_;
}
ControlState::operator Eigen::VectorXf() const {
  return eigen_f_data_;
}
ControlState::operator Eigen::VectorXd() const {
  return eigen_d_data_;
}

}  // namespace modules
}  // namespace flysq
