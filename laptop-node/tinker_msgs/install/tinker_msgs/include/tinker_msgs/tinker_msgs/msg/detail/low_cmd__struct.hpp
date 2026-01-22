// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from tinker_msgs:msg/LowCmd.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/low_cmd.hpp"


#ifndef TINKER_MSGS__MSG__DETAIL__LOW_CMD__STRUCT_HPP_
#define TINKER_MSGS__MSG__DETAIL__LOW_CMD__STRUCT_HPP_

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
// Member 'motor_cmd'
#include "tinker_msgs/msg/detail/motor_cmd__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__tinker_msgs__msg__LowCmd __attribute__((deprecated))
#else
# define DEPRECATED__tinker_msgs__msg__LowCmd __declspec(deprecated)
#endif

namespace tinker_msgs
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct LowCmd_
{
  using Type = LowCmd_<ContainerAllocator>;

  explicit LowCmd_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : timestamp_state(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->motor_cmd.fill(tinker_msgs::msg::MotorCmd_<ContainerAllocator>{_init});
    }
  }

  explicit LowCmd_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : timestamp_state(_alloc, _init),
    motor_cmd(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->motor_cmd.fill(tinker_msgs::msg::MotorCmd_<ContainerAllocator>{_alloc, _init});
    }
  }

  // field types and members
  using _timestamp_state_type =
    builtin_interfaces::msg::Time_<ContainerAllocator>;
  _timestamp_state_type timestamp_state;
  using _motor_cmd_type =
    std::array<tinker_msgs::msg::MotorCmd_<ContainerAllocator>, 10>;
  _motor_cmd_type motor_cmd;

  // setters for named parameter idiom
  Type & set__timestamp_state(
    const builtin_interfaces::msg::Time_<ContainerAllocator> & _arg)
  {
    this->timestamp_state = _arg;
    return *this;
  }
  Type & set__motor_cmd(
    const std::array<tinker_msgs::msg::MotorCmd_<ContainerAllocator>, 10> & _arg)
  {
    this->motor_cmd = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    tinker_msgs::msg::LowCmd_<ContainerAllocator> *;
  using ConstRawPtr =
    const tinker_msgs::msg::LowCmd_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<tinker_msgs::msg::LowCmd_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<tinker_msgs::msg::LowCmd_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      tinker_msgs::msg::LowCmd_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<tinker_msgs::msg::LowCmd_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      tinker_msgs::msg::LowCmd_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<tinker_msgs::msg::LowCmd_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<tinker_msgs::msg::LowCmd_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<tinker_msgs::msg::LowCmd_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__tinker_msgs__msg__LowCmd
    std::shared_ptr<tinker_msgs::msg::LowCmd_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__tinker_msgs__msg__LowCmd
    std::shared_ptr<tinker_msgs::msg::LowCmd_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const LowCmd_ & other) const
  {
    if (this->timestamp_state != other.timestamp_state) {
      return false;
    }
    if (this->motor_cmd != other.motor_cmd) {
      return false;
    }
    return true;
  }
  bool operator!=(const LowCmd_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct LowCmd_

// alias to use template instance with default allocator
using LowCmd =
  tinker_msgs::msg::LowCmd_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace tinker_msgs

#endif  // TINKER_MSGS__MSG__DETAIL__LOW_CMD__STRUCT_HPP_
