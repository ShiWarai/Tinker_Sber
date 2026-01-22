// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from tinker_msgs:msg/MotorCmd.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "tinker_msgs/msg/motor_cmd.h"


#ifndef TINKER_MSGS__MSG__DETAIL__MOTOR_CMD__FUNCTIONS_H_
#define TINKER_MSGS__MSG__DETAIL__MOTOR_CMD__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/action_type_support_struct.h"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_c/service_type_support_struct.h"
#include "rosidl_runtime_c/type_description/type_description__struct.h"
#include "rosidl_runtime_c/type_description/type_source__struct.h"
#include "rosidl_runtime_c/type_hash.h"
#include "rosidl_runtime_c/visibility_control.h"
#include "tinker_msgs/msg/rosidl_generator_c__visibility_control.h"

#include "tinker_msgs/msg/detail/motor_cmd__struct.h"

/// Initialize msg/MotorCmd message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * tinker_msgs__msg__MotorCmd
 * )) before or use
 * tinker_msgs__msg__MotorCmd__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
bool
tinker_msgs__msg__MotorCmd__init(tinker_msgs__msg__MotorCmd * msg);

/// Finalize msg/MotorCmd message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
void
tinker_msgs__msg__MotorCmd__fini(tinker_msgs__msg__MotorCmd * msg);

/// Create msg/MotorCmd message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * tinker_msgs__msg__MotorCmd__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
tinker_msgs__msg__MotorCmd *
tinker_msgs__msg__MotorCmd__create(void);

/// Destroy msg/MotorCmd message.
/**
 * It calls
 * tinker_msgs__msg__MotorCmd__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
void
tinker_msgs__msg__MotorCmd__destroy(tinker_msgs__msg__MotorCmd * msg);

/// Check for msg/MotorCmd message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
bool
tinker_msgs__msg__MotorCmd__are_equal(const tinker_msgs__msg__MotorCmd * lhs, const tinker_msgs__msg__MotorCmd * rhs);

/// Copy a msg/MotorCmd message.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source message pointer.
 * \param[out] output The target message pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer is null
 *   or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
bool
tinker_msgs__msg__MotorCmd__copy(
  const tinker_msgs__msg__MotorCmd * input,
  tinker_msgs__msg__MotorCmd * output);

/// Retrieve pointer to the hash of the description of this type.
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
const rosidl_type_hash_t *
tinker_msgs__msg__MotorCmd__get_type_hash(
  const rosidl_message_type_support_t * type_support);

/// Retrieve pointer to the description of this type.
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
const rosidl_runtime_c__type_description__TypeDescription *
tinker_msgs__msg__MotorCmd__get_type_description(
  const rosidl_message_type_support_t * type_support);

/// Retrieve pointer to the single raw source text that defined this type.
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
const rosidl_runtime_c__type_description__TypeSource *
tinker_msgs__msg__MotorCmd__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support);

/// Retrieve pointer to the recursive raw sources that defined the description of this type.
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
const rosidl_runtime_c__type_description__TypeSource__Sequence *
tinker_msgs__msg__MotorCmd__get_type_description_sources(
  const rosidl_message_type_support_t * type_support);

/// Initialize array of msg/MotorCmd messages.
/**
 * It allocates the memory for the number of elements and calls
 * tinker_msgs__msg__MotorCmd__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
bool
tinker_msgs__msg__MotorCmd__Sequence__init(tinker_msgs__msg__MotorCmd__Sequence * array, size_t size);

/// Finalize array of msg/MotorCmd messages.
/**
 * It calls
 * tinker_msgs__msg__MotorCmd__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
void
tinker_msgs__msg__MotorCmd__Sequence__fini(tinker_msgs__msg__MotorCmd__Sequence * array);

/// Create array of msg/MotorCmd messages.
/**
 * It allocates the memory for the array and calls
 * tinker_msgs__msg__MotorCmd__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
tinker_msgs__msg__MotorCmd__Sequence *
tinker_msgs__msg__MotorCmd__Sequence__create(size_t size);

/// Destroy array of msg/MotorCmd messages.
/**
 * It calls
 * tinker_msgs__msg__MotorCmd__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
void
tinker_msgs__msg__MotorCmd__Sequence__destroy(tinker_msgs__msg__MotorCmd__Sequence * array);

/// Check for msg/MotorCmd message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
bool
tinker_msgs__msg__MotorCmd__Sequence__are_equal(const tinker_msgs__msg__MotorCmd__Sequence * lhs, const tinker_msgs__msg__MotorCmd__Sequence * rhs);

/// Copy an array of msg/MotorCmd messages.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source array pointer.
 * \param[out] output The target array pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer
 *   is null or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
bool
tinker_msgs__msg__MotorCmd__Sequence__copy(
  const tinker_msgs__msg__MotorCmd__Sequence * input,
  tinker_msgs__msg__MotorCmd__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // TINKER_MSGS__MSG__DETAIL__MOTOR_CMD__FUNCTIONS_H_
