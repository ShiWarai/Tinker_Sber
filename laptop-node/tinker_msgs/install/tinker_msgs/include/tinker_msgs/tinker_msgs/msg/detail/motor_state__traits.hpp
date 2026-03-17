// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from tinker_msgs:msg/MotorState.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/motor_state.hpp"


#ifndef TINKER_MSGS__MSG__DETAIL__MOTOR_STATE__TRAITS_HPP_
#define TINKER_MSGS__MSG__DETAIL__MOTOR_STATE__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "tinker_msgs/msg/detail/motor_state__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'timestamp_state'
#include "builtin_interfaces/msg/detail/time__traits.hpp"

namespace tinker_msgs
{

namespace msg
{

inline void to_flow_style_yaml(
  const MotorState & msg,
  std::ostream & out)
{
  out << "{";
  // member: timestamp_state
  {
    out << "timestamp_state: ";
    to_flow_style_yaml(msg.timestamp_state, out);
    out << ", ";
  }

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

  // member: temperature_mosfet
  {
    out << "temperature_mosfet: ";
    rosidl_generator_traits::value_to_yaml(msg.temperature_mosfet, out);
    out << ", ";
  }

  // member: temperature_rotor
  {
    out << "temperature_rotor: ";
    rosidl_generator_traits::value_to_yaml(msg.temperature_rotor, out);
    out << ", ";
  }

  // member: error
  {
    out << "error: ";
    rosidl_generator_traits::value_to_yaml(msg.error, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const MotorState & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: timestamp_state
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "timestamp_state:\n";
    to_block_style_yaml(msg.timestamp_state, out, indentation + 2);
  }

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

  // member: temperature_mosfet
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "temperature_mosfet: ";
    rosidl_generator_traits::value_to_yaml(msg.temperature_mosfet, out);
    out << "\n";
  }

  // member: temperature_rotor
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "temperature_rotor: ";
    rosidl_generator_traits::value_to_yaml(msg.temperature_rotor, out);
    out << "\n";
  }

  // member: error
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "error: ";
    rosidl_generator_traits::value_to_yaml(msg.error, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const MotorState & msg, bool use_flow_style = false)
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
  const tinker_msgs::msg::MotorState & msg,
  std::ostream & out, size_t indentation = 0)
{
  tinker_msgs::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use tinker_msgs::msg::to_yaml() instead")]]
inline std::string to_yaml(const tinker_msgs::msg::MotorState & msg)
{
  return tinker_msgs::msg::to_yaml(msg);
}

template<>
inline const char * data_type<tinker_msgs::msg::MotorState>()
{
  return "tinker_msgs::msg::MotorState";
}

template<>
inline const char * name<tinker_msgs::msg::MotorState>()
{
  return "tinker_msgs/msg/MotorState";
}

template<>
struct has_fixed_size<tinker_msgs::msg::MotorState>
  : std::integral_constant<bool, has_fixed_size<builtin_interfaces::msg::Time>::value> {};

template<>
struct has_bounded_size<tinker_msgs::msg::MotorState>
  : std::integral_constant<bool, has_bounded_size<builtin_interfaces::msg::Time>::value> {};

template<>
struct is_message<tinker_msgs::msg::MotorState>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // TINKER_MSGS__MSG__DETAIL__MOTOR_STATE__TRAITS_HPP_
