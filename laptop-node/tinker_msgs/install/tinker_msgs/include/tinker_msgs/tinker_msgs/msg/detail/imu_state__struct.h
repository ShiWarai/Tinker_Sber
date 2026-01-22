// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from tinker_msgs:msg/IMUState.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/imu_state.h"


#ifndef TINKER_MSGS__MSG__DETAIL__IMU_STATE__STRUCT_H_
#define TINKER_MSGS__MSG__DETAIL__IMU_STATE__STRUCT_H_

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

/// Struct defined in msg/IMUState in the package tinker_msgs.
typedef struct tinker_msgs__msg__IMUState
{
  builtin_interfaces__msg__Time timestamp_state;
  float quaternion[4];
  float gyroscope[3];
  float accelerometer[3];
  float rpy[3];
  int16_t temperature;
} tinker_msgs__msg__IMUState;

// Struct for a sequence of tinker_msgs__msg__IMUState.
typedef struct tinker_msgs__msg__IMUState__Sequence
{
  tinker_msgs__msg__IMUState * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} tinker_msgs__msg__IMUState__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // TINKER_MSGS__MSG__DETAIL__IMU_STATE__STRUCT_H_
