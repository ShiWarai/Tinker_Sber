// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from tinker_msgs:msg/MotorState.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/motor_state.hpp"


#ifndef TINKER_MSGS__MSG__DETAIL__MOTOR_STATE__BUILDER_HPP_
#define TINKER_MSGS__MSG__DETAIL__MOTOR_STATE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "tinker_msgs/msg/detail/motor_state__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace tinker_msgs
{

namespace msg
{

namespace builder
{

class Init_MotorState_error
{
public:
  explicit Init_MotorState_error(::tinker_msgs::msg::MotorState & msg)
  : msg_(msg)
  {}
  ::tinker_msgs::msg::MotorState error(::tinker_msgs::msg::MotorState::_error_type arg)
  {
    msg_.error = std::move(arg);
    return std::move(msg_);
  }

private:
  ::tinker_msgs::msg::MotorState msg_;
};

class Init_MotorState_temperature_rotor
{
public:
  explicit Init_MotorState_temperature_rotor(::tinker_msgs::msg::MotorState & msg)
  : msg_(msg)
  {}
  Init_MotorState_error temperature_rotor(::tinker_msgs::msg::MotorState::_temperature_rotor_type arg)
  {
    msg_.temperature_rotor = std::move(arg);
    return Init_MotorState_error(msg_);
  }

private:
  ::tinker_msgs::msg::MotorState msg_;
};

class Init_MotorState_temperature_mosfet
{
public:
  explicit Init_MotorState_temperature_mosfet(::tinker_msgs::msg::MotorState & msg)
  : msg_(msg)
  {}
  Init_MotorState_temperature_rotor temperature_mosfet(::tinker_msgs::msg::MotorState::_temperature_mosfet_type arg)
  {
    msg_.temperature_mosfet = std::move(arg);
    return Init_MotorState_temperature_rotor(msg_);
  }

private:
  ::tinker_msgs::msg::MotorState msg_;
};

class Init_MotorState_torque
{
public:
  explicit Init_MotorState_torque(::tinker_msgs::msg::MotorState & msg)
  : msg_(msg)
  {}
  Init_MotorState_temperature_mosfet torque(::tinker_msgs::msg::MotorState::_torque_type arg)
  {
    msg_.torque = std::move(arg);
    return Init_MotorState_temperature_mosfet(msg_);
  }

private:
  ::tinker_msgs::msg::MotorState msg_;
};

class Init_MotorState_velocity
{
public:
  explicit Init_MotorState_velocity(::tinker_msgs::msg::MotorState & msg)
  : msg_(msg)
  {}
  Init_MotorState_torque velocity(::tinker_msgs::msg::MotorState::_velocity_type arg)
  {
    msg_.velocity = std::move(arg);
    return Init_MotorState_torque(msg_);
  }

private:
  ::tinker_msgs::msg::MotorState msg_;
};

class Init_MotorState_position
{
public:
  explicit Init_MotorState_position(::tinker_msgs::msg::MotorState & msg)
  : msg_(msg)
  {}
  Init_MotorState_velocity position(::tinker_msgs::msg::MotorState::_position_type arg)
  {
    msg_.position = std::move(arg);
    return Init_MotorState_velocity(msg_);
  }

private:
  ::tinker_msgs::msg::MotorState msg_;
};

class Init_MotorState_timestamp_state
{
public:
  Init_MotorState_timestamp_state()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_MotorState_position timestamp_state(::tinker_msgs::msg::MotorState::_timestamp_state_type arg)
  {
    msg_.timestamp_state = std::move(arg);
    return Init_MotorState_position(msg_);
  }

private:
  ::tinker_msgs::msg::MotorState msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::tinker_msgs::msg::MotorState>()
{
  return tinker_msgs::msg::builder::Init_MotorState_timestamp_state();
}

}  // namespace tinker_msgs

#endif  // TINKER_MSGS__MSG__DETAIL__MOTOR_STATE__BUILDER_HPP_
