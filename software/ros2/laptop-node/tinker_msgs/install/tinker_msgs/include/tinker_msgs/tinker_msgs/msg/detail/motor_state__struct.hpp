// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from tinker_msgs:msg/MotorState.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/motor_state.hpp"


#ifndef TINKER_MSGS__MSG__DETAIL__MOTOR_STATE__STRUCT_HPP_
#define TINKER_MSGS__MSG__DETAIL__MOTOR_STATE__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'timestamp_state'
#include "builtin_interfaces/msg/detail/time__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__tinker_msgs__msg__MotorState __attribute__((deprecated))
#else
# define DEPRECATED__tinker_msgs__msg__MotorState __declspec(deprecated)
#endif

namespace tinker_msgs
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct MotorState_
{
  using Type = MotorState_<ContainerAllocator>;

  explicit MotorState_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : timestamp_state(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->position = 0.0f;
      this->velocity = 0.0f;
      this->torque = 0.0f;
      this->temperature_mosfet = 0;
      this->temperature_rotor = 0;
      this->error = 0;
    }
  }

  explicit MotorState_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : timestamp_state(_alloc, _init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->position = 0.0f;
      this->velocity = 0.0f;
      this->torque = 0.0f;
      this->temperature_mosfet = 0;
      this->temperature_rotor = 0;
      this->error = 0;
    }
  }

  // field types and members
  using _timestamp_state_type =
    builtin_interfaces::msg::Time_<ContainerAllocator>;
  _timestamp_state_type timestamp_state;
  using _position_type =
    float;
  _position_type position;
  using _velocity_type =
    float;
  _velocity_type velocity;
  using _torque_type =
    float;
  _torque_type torque;
  using _temperature_mosfet_type =
    int16_t;
  _temperature_mosfet_type temperature_mosfet;
  using _temperature_rotor_type =
    int16_t;
  _temperature_rotor_type temperature_rotor;
  using _error_type =
    uint8_t;
  _error_type error;

  // setters for named parameter idiom
  Type & set__timestamp_state(
    const builtin_interfaces::msg::Time_<ContainerAllocator> & _arg)
  {
    this->timestamp_state = _arg;
    return *this;
  }
  Type & set__position(
    const float & _arg)
  {
    this->position = _arg;
    return *this;
  }
  Type & set__velocity(
    const float & _arg)
  {
    this->velocity = _arg;
    return *this;
  }
  Type & set__torque(
    const float & _arg)
  {
    this->torque = _arg;
    return *this;
  }
  Type & set__temperature_mosfet(
    const int16_t & _arg)
  {
    this->temperature_mosfet = _arg;
    return *this;
  }
  Type & set__temperature_rotor(
    const int16_t & _arg)
  {
    this->temperature_rotor = _arg;
    return *this;
  }
  Type & set__error(
    const uint8_t & _arg)
  {
    this->error = _arg;
    return *this;
  }

  // constant declarations
  static constexpr uint8_t DISABLING =
    0u;
  static constexpr uint8_t ENABLE =
    1u;
  static constexpr uint8_t OVERCURRENT =
    10u;
  static constexpr uint8_t OVERVOLTAGE =
    8u;
  static constexpr uint8_t LOWVOLTAGE =
    9u;
  static constexpr uint8_t OVERTEMPERATURE_MOSFET =
    11u;
  static constexpr uint8_t OVERTEMPERATURE_ROTOR =
    12u;
  static constexpr uint8_t LOSS_CONNECTION =
    13u;
  static constexpr uint8_t OVERLOAD =
    14u;

  // pointer types
  using RawPtr =
    tinker_msgs::msg::MotorState_<ContainerAllocator> *;
  using ConstRawPtr =
    const tinker_msgs::msg::MotorState_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<tinker_msgs::msg::MotorState_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<tinker_msgs::msg::MotorState_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      tinker_msgs::msg::MotorState_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<tinker_msgs::msg::MotorState_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      tinker_msgs::msg::MotorState_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<tinker_msgs::msg::MotorState_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<tinker_msgs::msg::MotorState_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<tinker_msgs::msg::MotorState_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__tinker_msgs__msg__MotorState
    std::shared_ptr<tinker_msgs::msg::MotorState_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__tinker_msgs__msg__MotorState
    std::shared_ptr<tinker_msgs::msg::MotorState_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const MotorState_ & other) const
  {
    if (this->timestamp_state != other.timestamp_state) {
      return false;
    }
    if (this->position != other.position) {
      return false;
    }
    if (this->velocity != other.velocity) {
      return false;
    }
    if (this->torque != other.torque) {
      return false;
    }
    if (this->temperature_mosfet != other.temperature_mosfet) {
      return false;
    }
    if (this->temperature_rotor != other.temperature_rotor) {
      return false;
    }
    if (this->error != other.error) {
      return false;
    }
    return true;
  }
  bool operator!=(const MotorState_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct MotorState_

// alias to use template instance with default allocator
using MotorState =
  tinker_msgs::msg::MotorState_<std::allocator<void>>;

// constant definitions
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr uint8_t MotorState_<ContainerAllocator>::DISABLING;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr uint8_t MotorState_<ContainerAllocator>::ENABLE;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr uint8_t MotorState_<ContainerAllocator>::OVERCURRENT;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr uint8_t MotorState_<ContainerAllocator>::OVERVOLTAGE;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr uint8_t MotorState_<ContainerAllocator>::LOWVOLTAGE;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr uint8_t MotorState_<ContainerAllocator>::OVERTEMPERATURE_MOSFET;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr uint8_t MotorState_<ContainerAllocator>::OVERTEMPERATURE_ROTOR;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr uint8_t MotorState_<ContainerAllocator>::LOSS_CONNECTION;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr uint8_t MotorState_<ContainerAllocator>::OVERLOAD;
#endif  // __cplusplus < 201703L

}  // namespace msg

}  // namespace tinker_msgs

#endif  // TINKER_MSGS__MSG__DETAIL__MOTOR_STATE__STRUCT_HPP_
