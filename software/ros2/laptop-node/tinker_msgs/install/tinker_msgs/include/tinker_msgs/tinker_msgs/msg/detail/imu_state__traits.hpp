// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from tinker_msgs:msg/IMUState.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/imu_state.hpp"


#ifndef TINKER_MSGS__MSG__DETAIL__IMU_STATE__TRAITS_HPP_
#define TINKER_MSGS__MSG__DETAIL__IMU_STATE__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "tinker_msgs/msg/detail/imu_state__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'timestamp_state'
#include "builtin_interfaces/msg/detail/time__traits.hpp"

namespace tinker_msgs
{

namespace msg
{

inline void to_flow_style_yaml(
  const IMUState & msg,
  std::ostream & out)
{
  out << "{";
  // member: timestamp_state
  {
    out << "timestamp_state: ";
    to_flow_style_yaml(msg.timestamp_state, out);
    out << ", ";
  }

  // member: quaternion
  {
    if (msg.quaternion.size() == 0) {
      out << "quaternion: []";
    } else {
      out << "quaternion: [";
      size_t pending_items = msg.quaternion.size();
      for (auto item : msg.quaternion) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: gyroscope
  {
    if (msg.gyroscope.size() == 0) {
      out << "gyroscope: []";
    } else {
      out << "gyroscope: [";
      size_t pending_items = msg.gyroscope.size();
      for (auto item : msg.gyroscope) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: accelerometer
  {
    if (msg.accelerometer.size() == 0) {
      out << "accelerometer: []";
    } else {
      out << "accelerometer: [";
      size_t pending_items = msg.accelerometer.size();
      for (auto item : msg.accelerometer) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: rpy
  {
    if (msg.rpy.size() == 0) {
      out << "rpy: []";
    } else {
      out << "rpy: [";
      size_t pending_items = msg.rpy.size();
      for (auto item : msg.rpy) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: temperature
  {
    out << "temperature: ";
    rosidl_generator_traits::value_to_yaml(msg.temperature, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const IMUState & msg,
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

  // member: quaternion
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.quaternion.size() == 0) {
      out << "quaternion: []\n";
    } else {
      out << "quaternion:\n";
      for (auto item : msg.quaternion) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: gyroscope
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.gyroscope.size() == 0) {
      out << "gyroscope: []\n";
    } else {
      out << "gyroscope:\n";
      for (auto item : msg.gyroscope) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: accelerometer
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.accelerometer.size() == 0) {
      out << "accelerometer: []\n";
    } else {
      out << "accelerometer:\n";
      for (auto item : msg.accelerometer) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: rpy
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.rpy.size() == 0) {
      out << "rpy: []\n";
    } else {
      out << "rpy:\n";
      for (auto item : msg.rpy) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: temperature
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "temperature: ";
    rosidl_generator_traits::value_to_yaml(msg.temperature, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const IMUState & msg, bool use_flow_style = false)
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
  const tinker_msgs::msg::IMUState & msg,
  std::ostream & out, size_t indentation = 0)
{
  tinker_msgs::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use tinker_msgs::msg::to_yaml() instead")]]
inline std::string to_yaml(const tinker_msgs::msg::IMUState & msg)
{
  return tinker_msgs::msg::to_yaml(msg);
}

template<>
inline const char * data_type<tinker_msgs::msg::IMUState>()
{
  return "tinker_msgs::msg::IMUState";
}

template<>
inline const char * name<tinker_msgs::msg::IMUState>()
{
  return "tinker_msgs/msg/IMUState";
}

template<>
struct has_fixed_size<tinker_msgs::msg::IMUState>
  : std::integral_constant<bool, has_fixed_size<builtin_interfaces::msg::Time>::value> {};

template<>
struct has_bounded_size<tinker_msgs::msg::IMUState>
  : std::integral_constant<bool, has_bounded_size<builtin_interfaces::msg::Time>::value> {};

template<>
struct is_message<tinker_msgs::msg::IMUState>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // TINKER_MSGS__MSG__DETAIL__IMU_STATE__TRAITS_HPP_
