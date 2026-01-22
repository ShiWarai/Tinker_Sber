// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from tinker_msgs:msg/LowState.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/low_state.h"


#ifndef TINKER_MSGS__MSG__DETAIL__LOW_STATE__STRUCT_H_
#define TINKER_MSGS__MSG__DETAIL__LOW_STATE__STRUCT_H_

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
// Member 'imu_state'
#include "tinker_msgs/msg/detail/imu_state__struct.h"
// Member 'motor_state'
#include "tinker_msgs/msg/detail/motor_state__struct.h"

/// Struct defined in msg/LowState in the package tinker_msgs.
typedef struct tinker_msgs__msg__LowState
{
  builtin_interfaces__msg__Time timestamp_state;
  uint32_t tick;
  tinker_msgs__msg__IMUState imu_state;
  tinker_msgs__msg__MotorState motor_state[10];
} tinker_msgs__msg__LowState;

// Struct for a sequence of tinker_msgs__msg__LowState.
typedef struct tinker_msgs__msg__LowState__Sequence
{
  tinker_msgs__msg__LowState * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} tinker_msgs__msg__LowState__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // TINKER_MSGS__MSG__DETAIL__LOW_STATE__STRUCT_H_
