// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from tinker_msgs:msg/LowState.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "tinker_msgs/msg/detail/low_state__rosidl_typesupport_introspection_c.h"
#include "tinker_msgs/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "tinker_msgs/msg/detail/low_state__functions.h"
#include "tinker_msgs/msg/detail/low_state__struct.h"


// Include directives for member types
// Member `timestamp_state`
#include "builtin_interfaces/msg/time.h"
// Member `timestamp_state`
#include "builtin_interfaces/msg/detail/time__rosidl_typesupport_introspection_c.h"
// Member `imu_state`
#include "tinker_msgs/msg/imu_state.h"
// Member `imu_state`
#include "tinker_msgs/msg/detail/imu_state__rosidl_typesupport_introspection_c.h"
// Member `motor_state`
#include "tinker_msgs/msg/motor_state.h"
// Member `motor_state`
#include "tinker_msgs/msg/detail/motor_state__rosidl_typesupport_introspection_c.h"

#ifdef __cplusplus
extern "C"
{
#endif

void tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  tinker_msgs__msg__LowState__init(message_memory);
}

void tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_fini_function(void * message_memory)
{
  tinker_msgs__msg__LowState__fini(message_memory);
}

size_t tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__size_function__LowState__motor_state(
  const void * untyped_member)
{
  (void)untyped_member;
  return 10;
}

const void * tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__get_const_function__LowState__motor_state(
  const void * untyped_member, size_t index)
{
  const tinker_msgs__msg__MotorState * member =
    (const tinker_msgs__msg__MotorState *)(untyped_member);
  return &member[index];
}

void * tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__get_function__LowState__motor_state(
  void * untyped_member, size_t index)
{
  tinker_msgs__msg__MotorState * member =
    (tinker_msgs__msg__MotorState *)(untyped_member);
  return &member[index];
}

void tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__fetch_function__LowState__motor_state(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const tinker_msgs__msg__MotorState * item =
    ((const tinker_msgs__msg__MotorState *)
    tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__get_const_function__LowState__motor_state(untyped_member, index));
  tinker_msgs__msg__MotorState * value =
    (tinker_msgs__msg__MotorState *)(untyped_value);
  *value = *item;
}

void tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__assign_function__LowState__motor_state(
  void * untyped_member, size_t index, const void * untyped_value)
{
  tinker_msgs__msg__MotorState * item =
    ((tinker_msgs__msg__MotorState *)
    tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__get_function__LowState__motor_state(untyped_member, index));
  const tinker_msgs__msg__MotorState * value =
    (const tinker_msgs__msg__MotorState *)(untyped_value);
  *item = *value;
}

static rosidl_typesupport_introspection_c__MessageMember tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_message_member_array[4] = {
  {
    "timestamp_state",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(tinker_msgs__msg__LowState, timestamp_state),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "tick",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(tinker_msgs__msg__LowState, tick),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "imu_state",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(tinker_msgs__msg__LowState, imu_state),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "motor_state",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is key
    true,  // is array
    10,  // array size
    false,  // is upper bound
    offsetof(tinker_msgs__msg__LowState, motor_state),  // bytes offset in struct
    NULL,  // default value
    tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__size_function__LowState__motor_state,  // size() function pointer
    tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__get_const_function__LowState__motor_state,  // get_const(index) function pointer
    tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__get_function__LowState__motor_state,  // get(index) function pointer
    tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__fetch_function__LowState__motor_state,  // fetch(index, &value) function pointer
    tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__assign_function__LowState__motor_state,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_message_members = {
  "tinker_msgs__msg",  // message namespace
  "LowState",  // message name
  4,  // number of fields
  sizeof(tinker_msgs__msg__LowState),
  false,  // has_any_key_member_
  tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_message_member_array,  // message members
  tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_init_function,  // function to initialize message memory (memory has to be allocated)
  tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_message_type_support_handle = {
  0,
  &tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_message_members,
  get_message_typesupport_handle_function,
  &tinker_msgs__msg__LowState__get_type_hash,
  &tinker_msgs__msg__LowState__get_type_description,
  &tinker_msgs__msg__LowState__get_type_description_sources,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_tinker_msgs
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, tinker_msgs, msg, LowState)() {
  tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_message_member_array[0].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, builtin_interfaces, msg, Time)();
  tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_message_member_array[2].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, tinker_msgs, msg, IMUState)();
  tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_message_member_array[3].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, tinker_msgs, msg, MotorState)();
  if (!tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_message_type_support_handle.typesupport_identifier) {
    tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &tinker_msgs__msg__LowState__rosidl_typesupport_introspection_c__LowState_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
