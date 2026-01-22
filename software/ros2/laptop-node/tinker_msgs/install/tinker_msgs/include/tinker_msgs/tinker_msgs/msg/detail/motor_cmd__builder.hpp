// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from tinker_msgs:msg/MotorCmd.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/motor_cmd.hpp"


#ifndef TINKER_MSGS__MSG__DETAIL__MOTOR_CMD__BUILDER_HPP_
#define TINKER_MSGS__MSG__DETAIL__MOTOR_CMD__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "tinker_msgs/msg/detail/motor_cmd__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace tinker_msgs
{

namespace msg
{

namespace builder
{

class Init_MotorCmd_kd
{
public:
  explicit Init_MotorCmd_kd(::tinker_msgs::msg::MotorCmd & msg)
  : msg_(msg)
  {}
  ::tinker_msgs::msg::MotorCmd kd(::tinker_msgs::msg::MotorCmd::_kd_type arg)
  {
    msg_.kd = std::move(arg);
    return std::move(msg_);
  }

private:
  ::tinker_msgs::msg::MotorCmd msg_;
};

class Init_MotorCmd_kp
{
public:
  explicit Init_MotorCmd_kp(::tinker_msgs::msg::MotorCmd & msg)
  : msg_(msg)
  {}
  Init_MotorCmd_kd kp(::tinker_msgs::msg::MotorCmd::_kp_type arg)
  {
    msg_.kp = std::move(arg);
    return Init_MotorCmd_kd(msg_);
  }

private:
  ::tinker_msgs::msg::MotorCmd msg_;
};

class Init_MotorCmd_torque
{
public:
  explicit Init_MotorCmd_torque(::tinker_msgs::msg::MotorCmd & msg)
  : msg_(msg)
  {}
  Init_MotorCmd_kp torque(::tinker_msgs::msg::MotorCmd::_torque_type arg)
  {
    msg_.torque = std::move(arg);
    return Init_MotorCmd_kp(msg_);
  }

private:
  ::tinker_msgs::msg::MotorCmd msg_;
};

class Init_MotorCmd_velocity
{
public:
  explicit Init_MotorCmd_velocity(::tinker_msgs::msg::MotorCmd & msg)
  : msg_(msg)
  {}
  Init_MotorCmd_torque velocity(::tinker_msgs::msg::MotorCmd::_velocity_type arg)
  {
    msg_.velocity = std::move(arg);
    return Init_MotorCmd_torque(msg_);
  }

private:
  ::tinker_msgs::msg::MotorCmd msg_;
};

class Init_MotorCmd_position
{
public:
  Init_MotorCmd_position()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_MotorCmd_velocity position(::tinker_msgs::msg::MotorCmd::_position_type arg)
  {
    msg_.position = std::move(arg);
    return Init_MotorCmd_velocity(msg_);
  }

private:
  ::tinker_msgs::msg::MotorCmd msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::tinker_msgs::msg::MotorCmd>()
{
  return tinker_msgs::msg::builder::Init_MotorCmd_position();
}

}  // namespace tinker_msgs

#endif  // TINKER_MSGS__MSG__DETAIL__MOTOR_CMD__BUILDER_HPP_
