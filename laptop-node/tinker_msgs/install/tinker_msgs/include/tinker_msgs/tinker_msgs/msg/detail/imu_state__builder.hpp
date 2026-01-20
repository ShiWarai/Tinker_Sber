// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from tinker_msgs:msg/IMUState.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/imu_state.hpp"


#ifndef TINKER_MSGS__MSG__DETAIL__IMU_STATE__BUILDER_HPP_
#define TINKER_MSGS__MSG__DETAIL__IMU_STATE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "tinker_msgs/msg/detail/imu_state__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace tinker_msgs
{

namespace msg
{

namespace builder
{

class Init_IMUState_temperature
{
public:
  explicit Init_IMUState_temperature(::tinker_msgs::msg::IMUState & msg)
  : msg_(msg)
  {}
  ::tinker_msgs::msg::IMUState temperature(::tinker_msgs::msg::IMUState::_temperature_type arg)
  {
    msg_.temperature = std::move(arg);
    return std::move(msg_);
  }

private:
  ::tinker_msgs::msg::IMUState msg_;
};

class Init_IMUState_rpy
{
public:
  explicit Init_IMUState_rpy(::tinker_msgs::msg::IMUState & msg)
  : msg_(msg)
  {}
  Init_IMUState_temperature rpy(::tinker_msgs::msg::IMUState::_rpy_type arg)
  {
    msg_.rpy = std::move(arg);
    return Init_IMUState_temperature(msg_);
  }

private:
  ::tinker_msgs::msg::IMUState msg_;
};

class Init_IMUState_accelerometer
{
public:
  explicit Init_IMUState_accelerometer(::tinker_msgs::msg::IMUState & msg)
  : msg_(msg)
  {}
  Init_IMUState_rpy accelerometer(::tinker_msgs::msg::IMUState::_accelerometer_type arg)
  {
    msg_.accelerometer = std::move(arg);
    return Init_IMUState_rpy(msg_);
  }

private:
  ::tinker_msgs::msg::IMUState msg_;
};

class Init_IMUState_gyroscope
{
public:
  explicit Init_IMUState_gyroscope(::tinker_msgs::msg::IMUState & msg)
  : msg_(msg)
  {}
  Init_IMUState_accelerometer gyroscope(::tinker_msgs::msg::IMUState::_gyroscope_type arg)
  {
    msg_.gyroscope = std::move(arg);
    return Init_IMUState_accelerometer(msg_);
  }

private:
  ::tinker_msgs::msg::IMUState msg_;
};

class Init_IMUState_quaternion
{
public:
  explicit Init_IMUState_quaternion(::tinker_msgs::msg::IMUState & msg)
  : msg_(msg)
  {}
  Init_IMUState_gyroscope quaternion(::tinker_msgs::msg::IMUState::_quaternion_type arg)
  {
    msg_.quaternion = std::move(arg);
    return Init_IMUState_gyroscope(msg_);
  }

private:
  ::tinker_msgs::msg::IMUState msg_;
};

class Init_IMUState_timestamp_state
{
public:
  Init_IMUState_timestamp_state()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_IMUState_quaternion timestamp_state(::tinker_msgs::msg::IMUState::_timestamp_state_type arg)
  {
    msg_.timestamp_state = std::move(arg);
    return Init_IMUState_quaternion(msg_);
  }

private:
  ::tinker_msgs::msg::IMUState msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::tinker_msgs::msg::IMUState>()
{
  return tinker_msgs::msg::builder::Init_IMUState_timestamp_state();
}

}  // namespace tinker_msgs

#endif  // TINKER_MSGS__MSG__DETAIL__IMU_STATE__BUILDER_HPP_
