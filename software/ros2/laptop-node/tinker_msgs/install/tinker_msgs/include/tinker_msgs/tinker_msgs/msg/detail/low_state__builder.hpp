// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from tinker_msgs:msg/LowState.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/low_state.hpp"


#ifndef TINKER_MSGS__MSG__DETAIL__LOW_STATE__BUILDER_HPP_
#define TINKER_MSGS__MSG__DETAIL__LOW_STATE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "tinker_msgs/msg/detail/low_state__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace tinker_msgs
{

namespace msg
{

namespace builder
{

class Init_LowState_motor_state
{
public:
  explicit Init_LowState_motor_state(::tinker_msgs::msg::LowState & msg)
  : msg_(msg)
  {}
  ::tinker_msgs::msg::LowState motor_state(::tinker_msgs::msg::LowState::_motor_state_type arg)
  {
    msg_.motor_state = std::move(arg);
    return std::move(msg_);
  }

private:
  ::tinker_msgs::msg::LowState msg_;
};

class Init_LowState_imu_state
{
public:
  explicit Init_LowState_imu_state(::tinker_msgs::msg::LowState & msg)
  : msg_(msg)
  {}
  Init_LowState_motor_state imu_state(::tinker_msgs::msg::LowState::_imu_state_type arg)
  {
    msg_.imu_state = std::move(arg);
    return Init_LowState_motor_state(msg_);
  }

private:
  ::tinker_msgs::msg::LowState msg_;
};

class Init_LowState_tick
{
public:
  explicit Init_LowState_tick(::tinker_msgs::msg::LowState & msg)
  : msg_(msg)
  {}
  Init_LowState_imu_state tick(::tinker_msgs::msg::LowState::_tick_type arg)
  {
    msg_.tick = std::move(arg);
    return Init_LowState_imu_state(msg_);
  }

private:
  ::tinker_msgs::msg::LowState msg_;
};

class Init_LowState_timestamp_state
{
public:
  Init_LowState_timestamp_state()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_LowState_tick timestamp_state(::tinker_msgs::msg::LowState::_timestamp_state_type arg)
  {
    msg_.timestamp_state = std::move(arg);
    return Init_LowState_tick(msg_);
  }

private:
  ::tinker_msgs::msg::LowState msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::tinker_msgs::msg::LowState>()
{
  return tinker_msgs::msg::builder::Init_LowState_timestamp_state();
}

}  // namespace tinker_msgs

#endif  // TINKER_MSGS__MSG__DETAIL__LOW_STATE__BUILDER_HPP_
