// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from tinker_msgs:msg/LowState.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/low_state.hpp"


#ifndef TINKER_MSGS__MSG__DETAIL__LOW_STATE__TRAITS_HPP_
#define TINKER_MSGS__MSG__DETAIL__LOW_STATE__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "tinker_msgs/msg/detail/low_state__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'timestamp_state'
#include "builtin_interfaces/msg/detail/time__traits.hpp"
// Member 'imu_state'
#include "tinker_msgs/msg/detail/imu_state__traits.hpp"
// Member 'motor_state'
#include "tinker_msgs/msg/detail/motor_state__traits.hpp"

namespace tinker_msgs
{

namespace msg
{

inline void to_flow_style_yaml(
  const LowState & msg,
  std::ostream & out)
{
  out << "{";
  // member: timestamp_state
  {
    out << "timestamp_state: ";
    to_flow_style_yaml(msg.timestamp_state, out);
    out << ", ";
  }

  // member: tick
  {
    out << "tick: ";
    rosidl_generator_traits::value_to_yaml(msg.tick, out);
    out << ", ";
  }

  // member: imu_state
  {
    out << "imu_state: ";
    to_flow_style_yaml(msg.imu_state, out);
    out << ", ";
  }

  // member: motor_state
  {
    if (msg.motor_state.size() == 0) {
      out << "motor_state: []";
    } else {
      out << "motor_state: [";
      size_t pending_items = msg.motor_state.size();
      for (auto item : msg.motor_state) {
        to_flow_style_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const LowState & msg,
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

  // member: tick
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "tick: ";
    rosidl_generator_traits::value_to_yaml(msg.tick, out);
    out << "\n";
  }

  // member: imu_state
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "imu_state:\n";
    to_block_style_yaml(msg.imu_state, out, indentation + 2);
  }

  // member: motor_state
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.motor_state.size() == 0) {
      out << "motor_state: []\n";
    } else {
      out << "motor_state:\n";
      for (auto item : msg.motor_state) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "-\n";
        to_block_style_yaml(item, out, indentation + 2);
      }
    }
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const LowState & msg, bool use_flow_style = false)
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
  const tinker_msgs::msg::LowState & msg,
  std::ostream & out, size_t indentation = 0)
{
  tinker_msgs::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use tinker_msgs::msg::to_yaml() instead")]]
inline std::string to_yaml(const tinker_msgs::msg::LowState & msg)
{
  return tinker_msgs::msg::to_yaml(msg);
}

template<>
inline const char * data_type<tinker_msgs::msg::LowState>()
{
  return "tinker_msgs::msg::LowState";
}

template<>
inline const char * name<tinker_msgs::msg::LowState>()
{
  return "tinker_msgs/msg/LowState";
}

template<>
struct has_fixed_size<tinker_msgs::msg::LowState>
  : std::integral_constant<bool, has_fixed_size<builtin_interfaces::msg::Time>::value && has_fixed_size<tinker_msgs::msg::IMUState>::value && has_fixed_size<tinker_msgs::msg::MotorState>::value> {};

template<>
struct has_bounded_size<tinker_msgs::msg::LowState>
  : std::integral_constant<bool, has_bounded_size<builtin_interfaces::msg::Time>::value && has_bounded_size<tinker_msgs::msg::IMUState>::value && has_bounded_size<tinker_msgs::msg::MotorState>::value> {};

template<>
struct is_message<tinker_msgs::msg::LowState>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // TINKER_MSGS__MSG__DETAIL__LOW_STATE__TRAITS_HPP_
