// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from tinker_msgs:msg/MotorState.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/motor_state.h"


#ifndef TINKER_MSGS__MSG__DETAIL__MOTOR_STATE__STRUCT_H_
#define TINKER_MSGS__MSG__DETAIL__MOTOR_STATE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Constants defined in the message

/// Constant 'DISABLING'.
enum
{
  tinker_msgs__msg__MotorState__DISABLING = 0
};

/// Constant 'ENABLE'.
/**
  * uint8 ENABLE = 0x01
 */
enum
{
  tinker_msgs__msg__MotorState__ENABLE = 1
};

/// Constant 'OVERCURRENT'.
/**
  * uint8 OVERCURRENT = 0x0A
 */
enum
{
  tinker_msgs__msg__MotorState__OVERCURRENT = 10
};

/// Constant 'OVERVOLTAGE'.
/**
  * uint8 OVERVOLTAGE = 0x08
 */
enum
{
  tinker_msgs__msg__MotorState__OVERVOLTAGE = 8
};

/// Constant 'LOWVOLTAGE'.
/**
  * uint8 LOWVOLTAGE = 0x09
 */
enum
{
  tinker_msgs__msg__MotorState__LOWVOLTAGE = 9
};

/// Constant 'OVERTEMPERATURE_MOSFET'.
/**
  * uint8 OVERTEMPERATURE_MOSFET = 0x0B
 */
enum
{
  tinker_msgs__msg__MotorState__OVERTEMPERATURE_MOSFET = 11
};

/// Constant 'OVERTEMPERATURE_ROTOR'.
/**
  * uint8 OVERTEMPERATURE_ROTOR = 0x0C
 */
enum
{
  tinker_msgs__msg__MotorState__OVERTEMPERATURE_ROTOR = 12
};

/// Constant 'LOSS_CONNECTION'.
/**
  * uint8 LOSS_CONNECTION= 0x0D
 */
enum
{
  tinker_msgs__msg__MotorState__LOSS_CONNECTION = 13
};

/// Constant 'OVERLOAD'.
/**
  * uint8 OVERLOAD = 0x0E
 */
enum
{
  tinker_msgs__msg__MotorState__OVERLOAD = 14
};

// Include directives for member types
// Member 'timestamp_state'
#include "builtin_interfaces/msg/detail/time__struct.h"

/// Struct defined in msg/MotorState in the package tinker_msgs.
/**
  * uint8 DISABLING = 0
 */
typedef struct tinker_msgs__msg__MotorState
{
  builtin_interfaces__msg__Time timestamp_state;
  float position;
  float velocity;
  float torque;
  int16_t temperature_mosfet;
  int16_t temperature_rotor;
  uint8_t error;
} tinker_msgs__msg__MotorState;

// Struct for a sequence of tinker_msgs__msg__MotorState.
typedef struct tinker_msgs__msg__MotorState__Sequence
{
  tinker_msgs__msg__MotorState * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} tinker_msgs__msg__MotorState__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // TINKER_MSGS__MSG__DETAIL__MOTOR_STATE__STRUCT_H_
