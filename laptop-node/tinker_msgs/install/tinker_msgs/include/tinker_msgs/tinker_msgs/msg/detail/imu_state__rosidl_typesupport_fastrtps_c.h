// generated from rosidl_typesupport_fastrtps_c/resource/idl__rosidl_typesupport_fastrtps_c.h.em
// with input from tinker_msgs:msg/IMUState.idl
// generated code does not contain a copyright notice
#ifndef TINKER_MSGS__MSG__DETAIL__IMU_STATE__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_
#define TINKER_MSGS__MSG__DETAIL__IMU_STATE__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_


#include <stddef.h>
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"
#include "tinker_msgs/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "tinker_msgs/msg/detail/imu_state__struct.h"
#include "fastcdr/Cdr.h"

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_tinker_msgs
bool cdr_serialize_tinker_msgs__msg__IMUState(
  const tinker_msgs__msg__IMUState * ros_message,
  eprosima::fastcdr::Cdr & cdr);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_tinker_msgs
bool cdr_deserialize_tinker_msgs__msg__IMUState(
  eprosima::fastcdr::Cdr &,
  tinker_msgs__msg__IMUState * ros_message);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_tinker_msgs
size_t get_serialized_size_tinker_msgs__msg__IMUState(
  const void * untyped_ros_message,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_tinker_msgs
size_t max_serialized_size_tinker_msgs__msg__IMUState(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_tinker_msgs
bool cdr_serialize_key_tinker_msgs__msg__IMUState(
  const tinker_msgs__msg__IMUState * ros_message,
  eprosima::fastcdr::Cdr & cdr);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_tinker_msgs
size_t get_serialized_size_key_tinker_msgs__msg__IMUState(
  const void * untyped_ros_message,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_tinker_msgs
size_t max_serialized_size_key_tinker_msgs__msg__IMUState(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_tinker_msgs
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, tinker_msgs, msg, IMUState)();

#ifdef __cplusplus
}
#endif

#endif  // TINKER_MSGS__MSG__DETAIL__IMU_STATE__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_
