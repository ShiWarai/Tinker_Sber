// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from tinker_msgs:msg/MotorCmd.idl
// generated code does not contain a copyright notice

#include "tinker_msgs/msg/detail/motor_cmd__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
const rosidl_type_hash_t *
tinker_msgs__msg__MotorCmd__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0x12, 0xeb, 0xc6, 0x1c, 0xa6, 0x90, 0xe2, 0x21,
      0xd6, 0x5d, 0x92, 0x99, 0xf7, 0x14, 0xfd, 0xc5,
      0x69, 0x11, 0xf6, 0xe1, 0x8e, 0xb1, 0x44, 0x95,
      0xf4, 0x29, 0x50, 0x4f, 0x14, 0x7c, 0x38, 0x36,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types

// Hashes for external referenced types
#ifndef NDEBUG
#endif

static char tinker_msgs__msg__MotorCmd__TYPE_NAME[] = "tinker_msgs/msg/MotorCmd";

// Define type names, field names, and default values
static char tinker_msgs__msg__MotorCmd__FIELD_NAME__position[] = "position";
static char tinker_msgs__msg__MotorCmd__FIELD_NAME__velocity[] = "velocity";
static char tinker_msgs__msg__MotorCmd__FIELD_NAME__torque[] = "torque";
static char tinker_msgs__msg__MotorCmd__FIELD_NAME__kp[] = "kp";
static char tinker_msgs__msg__MotorCmd__FIELD_NAME__kd[] = "kd";

static rosidl_runtime_c__type_description__Field tinker_msgs__msg__MotorCmd__FIELDS[] = {
  {
    {tinker_msgs__msg__MotorCmd__FIELD_NAME__position, 8, 8},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__MotorCmd__FIELD_NAME__velocity, 8, 8},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__MotorCmd__FIELD_NAME__torque, 6, 6},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__MotorCmd__FIELD_NAME__kp, 2, 2},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__MotorCmd__FIELD_NAME__kd, 2, 2},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
};

const rosidl_runtime_c__type_description__TypeDescription *
tinker_msgs__msg__MotorCmd__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {tinker_msgs__msg__MotorCmd__TYPE_NAME, 24, 24},
      {tinker_msgs__msg__MotorCmd__FIELDS, 5, 5},
    },
    {NULL, 0, 0},
  };
  if (!constructed) {
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "float32 position\n"
  "float32 velocity\n"
  "float32 torque\n"
  "float32 kp\n"
  "float32 kd";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
tinker_msgs__msg__MotorCmd__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {tinker_msgs__msg__MotorCmd__TYPE_NAME, 24, 24},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 70, 70},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
tinker_msgs__msg__MotorCmd__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[1];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 1, 1};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *tinker_msgs__msg__MotorCmd__get_individual_type_description_source(NULL),
    constructed = true;
  }
  return &source_sequence;
}
