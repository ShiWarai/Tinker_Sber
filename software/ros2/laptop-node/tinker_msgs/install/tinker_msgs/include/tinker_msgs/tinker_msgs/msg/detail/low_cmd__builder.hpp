// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from tinker_msgs:msg/LowCmd.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/low_cmd.hpp"


#ifndef TINKER_MSGS__MSG__DETAIL__LOW_CMD__BUILDER_HPP_
#define TINKER_MSGS__MSG__DETAIL__LOW_CMD__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "tinker_msgs/msg/detail/low_cmd__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace tinker_msgs
{

namespace msg
{

namespace builder
{

class Init_LowCmd_motor_cmd
{
public:
  explicit Init_LowCmd_motor_cmd(::tinker_msgs::msg::LowCmd & msg)
  : msg_(msg)
  {}
  ::tinker_msgs::msg::LowCmd motor_cmd(::tinker_msgs::msg::LowCmd::_motor_cmd_type arg)
  {
    msg_.motor_cmd = std::move(arg);
    return std::move(msg_);
  }

private:
  ::tinker_msgs::msg::LowCmd msg_;
};

class Init_LowCmd_timestamp_state
{
public:
  Init_LowCmd_timestamp_state()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_LowCmd_motor_cmd timestamp_state(::tinker_msgs::msg::LowCmd::_timestamp_state_type arg)
  {
    msg_.timestamp_state = std::move(arg);
    return Init_LowCmd_motor_cmd(msg_);
  }

private:
  ::tinker_msgs::msg::LowCmd msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::tinker_msgs::msg::LowCmd>()
{
  return tinker_msgs::msg::builder::Init_LowCmd_timestamp_state();
}

}  // namespace tinker_msgs

#endif  // TINKER_MSGS__MSG__DETAIL__LOW_CMD__BUILDER_HPP_
