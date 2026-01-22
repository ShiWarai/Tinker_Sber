// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from tinker_msgs:msg/MotorCmd.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/motor_cmd.h"


#ifndef TINKER_MSGS__MSG__DETAIL__MOTOR_CMD__STRUCT_H_
#define TINKER_MSGS__MSG__DETAIL__MOTOR_CMD__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Constants defined in the message

/// Struct defined in msg/MotorCmd in the package tinker_msgs.
typedef struct tinker_msgs__msg__MotorCmd
{
  float position;
  float velocity;
  float torque;
  float kp;
  float kd;
} tinker_msgs__msg__MotorCmd;

// Struct for a sequence of tinker_msgs__msg__MotorCmd.
typedef struct tinker_msgs__msg__MotorCmd__Sequence
{
  tinker_msgs__msg__MotorCmd * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} tinker_msgs__msg__MotorCmd__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // TINKER_MSGS__MSG__DETAIL__MOTOR_CMD__STRUCT_H_
