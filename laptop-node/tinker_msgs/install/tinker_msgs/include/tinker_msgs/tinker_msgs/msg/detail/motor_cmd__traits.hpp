// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from tinker_msgs:msg/MotorCmd.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/motor_cmd.hpp"


#ifndef TINKER_MSGS__MSG__DETAIL__MOTOR_CMD__TRAITS_HPP_
#define TINKER_MSGS__MSG__DETAIL__MOTOR_CMD__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "tinker_msgs/msg/detail/motor_cmd__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace tinker_msgs
{

namespace msg
{

inline void to_flow_style_yaml(
  const MotorCmd & msg,
  std::ostream & out)
{
  out << "{";
  // member: position
  {
    out << "position: ";
    rosidl_generator_traits::value_to_yaml(msg.position, out);
    out << ", ";
  }

  // member: velocity
  {
    out << "velocity: ";
    rosidl_generator_traits::value_to_yaml(msg.velocity, out);
    out << ", ";
  }

  // member: torque
  {
    out << "torque: ";
    rosidl_generator_traits::value_to_yaml(msg.torque, out);
    out << ", ";
  }

  // member: kp
  {
    out << "kp: ";
    rosidl_generator_traits::value_to_yaml(msg.kp, out);
    out << ", ";
  }

  // member: kd
  {
    out << "kd: ";
    rosidl_generator_traits::value_to_yaml(msg.kd, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const MotorCmd & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: position
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "position: ";
    rosidl_generator_traits::value_to_yaml(msg.position, out);
    out << "\n";
  }

  // member: velocity
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "velocity: ";
    rosidl_generator_traits::value_to_yaml(msg.velocity, out);
    out << "\n";
  }

  // member: torque
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "torque: ";
    rosidl_generator_traits::value_to_yaml(msg.torque, out);
    out << "\n";
  }

  // member: kp
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "kp: ";
    rosidl_generator_traits::value_to_yaml(msg.kp, out);
    out << "\n";
  }

  // member: kd
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "kd: ";
    rosidl_generator_traits::value_to_yaml(msg.kd, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const MotorCmd & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace msg

}  // namespace tinker_msgs

namespace rosidl_generator_traits
{

[[deprecated("use tinker_msgs::msg::to_block_style_yaml() instead")]]
inline void to_yaml(
  const tinker_msgs::msg::MotorCmd & msg,
  std::ostream & out, size_t indentation = 0)
{
  tinker_msgs::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use tinker_msgs::msg::to_yaml() instead")]]
inline std::string to_yaml(const tinker_msgs::msg::MotorCmd & msg)
{
  return tinker_msgs::msg::to_yaml(msg);
}

template<>
inline const char * data_type<tinker_msgs::msg::MotorCmd>()
{
  return "tinker_msgs::msg::MotorCmd";
}

template<>
inline const char * name<tinker_msgs::msg::MotorCmd>()
{
  return "tinker_msgs/msg/MotorCmd";
}

template<>
struct has_fixed_size<tinker_msgs::msg::MotorCmd>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<tinker_msgs::msg::MotorCmd>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<tinker_msgs::msg::MotorCmd>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // TINKER_MSGS__MSG__DETAIL__MOTOR_CMD__TRAITS_HPP_
