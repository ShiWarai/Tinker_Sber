// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from tinker_msgs:msg/LowCmd.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/low_cmd.h"


#ifndef TINKER_MSGS__MSG__DETAIL__LOW_CMD__STRUCT_H_
#define TINKER_MSGS__MSG__DETAIL__LOW_CMD__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Constants defined in the message

// Include directives for member types
// Member 'timestamp_state'
#include "builtin_interfaces/msg/detail/time__struct.h"
// Member 'motor_cmd'
#include "tinker_msgs/msg/detail/motor_cmd__struct.h"

/// Struct defined in msg/LowCmd in the package tinker_msgs.
typedef struct tinker_msgs__msg__LowCmd
{
  builtin_interfaces__msg__Time timestamp_state;
  tinker_msgs__msg__MotorCmd motor_cmd[10];
} tinker_msgs__msg__LowCmd;

// Struct for a sequence of tinker_msgs__msg__LowCmd.
typedef struct tinker_msgs__msg__LowCmd__Sequence
{
  tinker_msgs__msg__LowCmd * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} tinker_msgs__msg__LowCmd__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // TINKER_MSGS__MSG__DETAIL__LOW_CMD__STRUCT_H_
