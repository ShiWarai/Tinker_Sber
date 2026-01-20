// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from tinker_msgs:msg/LowCmd.idl
// generated code does not contain a copyright notice

#include "tinker_msgs/msg/detail/low_cmd__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
const rosidl_type_hash_t *
tinker_msgs__msg__LowCmd__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0x3d, 0x32, 0xf6, 0x09, 0x31, 0x06, 0xbe, 0x37,
      0x8b, 0x77, 0xf3, 0x16, 0xed, 0xfb, 0x70, 0x01,
      0x03, 0x7a, 0x1b, 0x21, 0x7f, 0xf8, 0xcc, 0xc0,
      0xd2, 0xec, 0x71, 0x3c, 0xb0, 0x6a, 0x6c, 0x5e,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types
#include "tinker_msgs/msg/detail/motor_cmd__functions.h"
#include "builtin_interfaces/msg/detail/time__functions.h"

// Hashes for external referenced types
#ifndef NDEBUG
static const rosidl_type_hash_t builtin_interfaces__msg__Time__EXPECTED_HASH = {1, {
    0xb1, 0x06, 0x23, 0x5e, 0x25, 0xa4, 0xc5, 0xed,
    0x35, 0x09, 0x8a, 0xa0, 0xa6, 0x1a, 0x3e, 0xe9,
    0xc9, 0xb1, 0x8d, 0x19, 0x7f, 0x39, 0x8b, 0x0e,
    0x42, 0x06, 0xce, 0xa9, 0xac, 0xf9, 0xc1, 0x97,
  }};
static const rosidl_type_hash_t tinker_msgs__msg__MotorCmd__EXPECTED_HASH = {1, {
    0x12, 0xeb, 0xc6, 0x1c, 0xa6, 0x90, 0xe2, 0x21,
    0xd6, 0x5d, 0x92, 0x99, 0xf7, 0x14, 0xfd, 0xc5,
    0x69, 0x11, 0xf6, 0xe1, 0x8e, 0xb1, 0x44, 0x95,
    0xf4, 0x29, 0x50, 0x4f, 0x14, 0x7c, 0x38, 0x36,
  }};
#endif

static char tinker_msgs__msg__LowCmd__TYPE_NAME[] = "tinker_msgs/msg/LowCmd";
static char builtin_interfaces__msg__Time__TYPE_NAME[] = "builtin_interfaces/msg/Time";
static char tinker_msgs__msg__MotorCmd__TYPE_NAME[] = "tinker_msgs/msg/MotorCmd";

// Define type names, field names, and default values
static char tinker_msgs__msg__LowCmd__FIELD_NAME__timestamp_state[] = "timestamp_state";
static char tinker_msgs__msg__LowCmd__FIELD_NAME__motor_cmd[] = "motor_cmd";

static rosidl_runtime_c__type_description__Field tinker_msgs__msg__LowCmd__FIELDS[] = {
  {
    {tinker_msgs__msg__LowCmd__FIELD_NAME__timestamp_state, 15, 15},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_NESTED_TYPE,
      0,
      0,
      {builtin_interfaces__msg__Time__TYPE_NAME, 27, 27},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__LowCmd__FIELD_NAME__motor_cmd, 9, 9},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_NESTED_TYPE_ARRAY,
      10,
      0,
      {tinker_msgs__msg__MotorCmd__TYPE_NAME, 24, 24},
    },
    {NULL, 0, 0},
  },
};

static rosidl_runtime_c__type_description__IndividualTypeDescription tinker_msgs__msg__LowCmd__REFERENCED_TYPE_DESCRIPTIONS[] = {
  {
    {builtin_interfaces__msg__Time__TYPE_NAME, 27, 27},
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__MotorCmd__TYPE_NAME, 24, 24},
    {NULL, 0, 0},
  },
};

const rosidl_runtime_c__type_description__TypeDescription *
tinker_msgs__msg__LowCmd__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {tinker_msgs__msg__LowCmd__TYPE_NAME, 22, 22},
      {tinker_msgs__msg__LowCmd__FIELDS, 2, 2},
    },
    {tinker_msgs__msg__LowCmd__REFERENCED_TYPE_DESCRIPTIONS, 2, 2},
  };
  if (!constructed) {
    assert(0 == memcmp(&builtin_interfaces__msg__Time__EXPECTED_HASH, builtin_interfaces__msg__Time__get_type_hash(NULL), sizeof(rosidl_type_hash_t)));
    description.referenced_type_descriptions.data[0].fields = builtin_interfaces__msg__Time__get_type_description(NULL)->type_description.fields;
    assert(0 == memcmp(&tinker_msgs__msg__MotorCmd__EXPECTED_HASH, tinker_msgs__msg__MotorCmd__get_type_hash(NULL), sizeof(rosidl_type_hash_t)));
    description.referenced_type_descriptions.data[1].fields = tinker_msgs__msg__MotorCmd__get_type_description(NULL)->type_description.fields;
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "builtin_interfaces/Time timestamp_state\n"
  "MotorCmd[10] motor_cmd";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
tinker_msgs__msg__LowCmd__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {tinker_msgs__msg__LowCmd__TYPE_NAME, 22, 22},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 62, 62},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
tinker_msgs__msg__LowCmd__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[3];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 3, 3};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *tinker_msgs__msg__LowCmd__get_individual_type_description_source(NULL),
    sources[1] = *builtin_interfaces__msg__Time__get_individual_type_description_source(NULL);
    sources[2] = *tinker_msgs__msg__MotorCmd__get_individual_type_description_source(NULL);
    constructed = true;
  }
  return &source_sequence;
}
