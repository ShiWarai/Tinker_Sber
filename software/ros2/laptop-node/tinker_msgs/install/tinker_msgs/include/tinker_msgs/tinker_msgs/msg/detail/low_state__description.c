// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from tinker_msgs:msg/LowState.idl
// generated code does not contain a copyright notice

#include "tinker_msgs/msg/detail/low_state__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
const rosidl_type_hash_t *
tinker_msgs__msg__LowState__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0x46, 0x64, 0x36, 0xbd, 0xd9, 0xfe, 0x31, 0x23,
      0x28, 0x00, 0x72, 0x47, 0x78, 0x85, 0xcf, 0xbf,
      0xf1, 0xf5, 0x75, 0x4e, 0x12, 0x00, 0xb4, 0xe4,
      0xd0, 0x9c, 0x23, 0xf1, 0x5e, 0xa0, 0x19, 0x6d,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types
#include "tinker_msgs/msg/detail/imu_state__functions.h"
#include "tinker_msgs/msg/detail/motor_state__functions.h"
#include "builtin_interfaces/msg/detail/time__functions.h"

// Hashes for external referenced types
#ifndef NDEBUG
static const rosidl_type_hash_t builtin_interfaces__msg__Time__EXPECTED_HASH = {1, {
    0xb1, 0x06, 0x23, 0x5e, 0x25, 0xa4, 0xc5, 0xed,
    0x35, 0x09, 0x8a, 0xa0, 0xa6, 0x1a, 0x3e, 0xe9,
    0xc9, 0xb1, 0x8d, 0x19, 0x7f, 0x39, 0x8b, 0x0e,
    0x42, 0x06, 0xce, 0xa9, 0xac, 0xf9, 0xc1, 0x97,
  }};
static const rosidl_type_hash_t tinker_msgs__msg__IMUState__EXPECTED_HASH = {1, {
    0xc8, 0x96, 0x12, 0xec, 0x4c, 0x34, 0xd3, 0x84,
    0x09, 0xca, 0xf0, 0x53, 0xbd, 0x1f, 0xff, 0x28,
    0xfc, 0xc4, 0xdd, 0x01, 0xd2, 0x44, 0x07, 0x2d,
    0xa7, 0xee, 0x4b, 0x64, 0x85, 0x4d, 0x3b, 0xc1,
  }};
static const rosidl_type_hash_t tinker_msgs__msg__MotorState__EXPECTED_HASH = {1, {
    0x67, 0x00, 0x32, 0x02, 0xb8, 0x19, 0x4b, 0xe8,
    0xfa, 0xa6, 0xdc, 0xed, 0xdf, 0x34, 0x0c, 0xe0,
    0x6b, 0xcc, 0x34, 0x90, 0xd2, 0x41, 0x08, 0x6d,
    0x61, 0x31, 0x29, 0x41, 0x0b, 0x61, 0x84, 0x9e,
  }};
#endif

static char tinker_msgs__msg__LowState__TYPE_NAME[] = "tinker_msgs/msg/LowState";
static char builtin_interfaces__msg__Time__TYPE_NAME[] = "builtin_interfaces/msg/Time";
static char tinker_msgs__msg__IMUState__TYPE_NAME[] = "tinker_msgs/msg/IMUState";
static char tinker_msgs__msg__MotorState__TYPE_NAME[] = "tinker_msgs/msg/MotorState";

// Define type names, field names, and default values
static char tinker_msgs__msg__LowState__FIELD_NAME__timestamp_state[] = "timestamp_state";
static char tinker_msgs__msg__LowState__FIELD_NAME__tick[] = "tick";
static char tinker_msgs__msg__LowState__FIELD_NAME__imu_state[] = "imu_state";
static char tinker_msgs__msg__LowState__FIELD_NAME__motor_state[] = "motor_state";

static rosidl_runtime_c__type_description__Field tinker_msgs__msg__LowState__FIELDS[] = {
  {
    {tinker_msgs__msg__LowState__FIELD_NAME__timestamp_state, 15, 15},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_NESTED_TYPE,
      0,
      0,
      {builtin_interfaces__msg__Time__TYPE_NAME, 27, 27},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__LowState__FIELD_NAME__tick, 4, 4},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT32,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__LowState__FIELD_NAME__imu_state, 9, 9},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_NESTED_TYPE,
      0,
      0,
      {tinker_msgs__msg__IMUState__TYPE_NAME, 24, 24},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__LowState__FIELD_NAME__motor_state, 11, 11},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_NESTED_TYPE_ARRAY,
      10,
      0,
      {tinker_msgs__msg__MotorState__TYPE_NAME, 26, 26},
    },
    {NULL, 0, 0},
  },
};

static rosidl_runtime_c__type_description__IndividualTypeDescription tinker_msgs__msg__LowState__REFERENCED_TYPE_DESCRIPTIONS[] = {
  {
    {builtin_interfaces__msg__Time__TYPE_NAME, 27, 27},
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__IMUState__TYPE_NAME, 24, 24},
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__MotorState__TYPE_NAME, 26, 26},
    {NULL, 0, 0},
  },
};

const rosidl_runtime_c__type_description__TypeDescription *
tinker_msgs__msg__LowState__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {tinker_msgs__msg__LowState__TYPE_NAME, 24, 24},
      {tinker_msgs__msg__LowState__FIELDS, 4, 4},
    },
    {tinker_msgs__msg__LowState__REFERENCED_TYPE_DESCRIPTIONS, 3, 3},
  };
  if (!constructed) {
    assert(0 == memcmp(&builtin_interfaces__msg__Time__EXPECTED_HASH, builtin_interfaces__msg__Time__get_type_hash(NULL), sizeof(rosidl_type_hash_t)));
    description.referenced_type_descriptions.data[0].fields = builtin_interfaces__msg__Time__get_type_description(NULL)->type_description.fields;
    assert(0 == memcmp(&tinker_msgs__msg__IMUState__EXPECTED_HASH, tinker_msgs__msg__IMUState__get_type_hash(NULL), sizeof(rosidl_type_hash_t)));
    description.referenced_type_descriptions.data[1].fields = tinker_msgs__msg__IMUState__get_type_description(NULL)->type_description.fields;
    assert(0 == memcmp(&tinker_msgs__msg__MotorState__EXPECTED_HASH, tinker_msgs__msg__MotorState__get_type_hash(NULL), sizeof(rosidl_type_hash_t)));
    description.referenced_type_descriptions.data[2].fields = tinker_msgs__msg__MotorState__get_type_description(NULL)->type_description.fields;
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "builtin_interfaces/Time timestamp_state\n"
  "uint32 tick\n"
  "IMUState imu_state\n"
  "MotorState[10] motor_state";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
tinker_msgs__msg__LowState__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {tinker_msgs__msg__LowState__TYPE_NAME, 24, 24},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 97, 97},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
tinker_msgs__msg__LowState__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[4];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 4, 4};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *tinker_msgs__msg__LowState__get_individual_type_description_source(NULL),
    sources[1] = *builtin_interfaces__msg__Time__get_individual_type_description_source(NULL);
    sources[2] = *tinker_msgs__msg__IMUState__get_individual_type_description_source(NULL);
    sources[3] = *tinker_msgs__msg__MotorState__get_individual_type_description_source(NULL);
    constructed = true;
  }
  return &source_sequence;
}
