// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from tinker_msgs:msg/MotorState.idl
// generated code does not contain a copyright notice

#include "tinker_msgs/msg/detail/motor_state__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_tinker_msgs
const rosidl_type_hash_t *
tinker_msgs__msg__MotorState__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0x67, 0x00, 0x32, 0x02, 0xb8, 0x19, 0x4b, 0xe8,
      0xfa, 0xa6, 0xdc, 0xed, 0xdf, 0x34, 0x0c, 0xe0,
      0x6b, 0xcc, 0x34, 0x90, 0xd2, 0x41, 0x08, 0x6d,
      0x61, 0x31, 0x29, 0x41, 0x0b, 0x61, 0x84, 0x9e,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types
#include "builtin_interfaces/msg/detail/time__functions.h"

// Hashes for external referenced types
#ifndef NDEBUG
static const rosidl_type_hash_t builtin_interfaces__msg__Time__EXPECTED_HASH = {1, {
    0xb1, 0x06, 0x23, 0x5e, 0x25, 0xa4, 0xc5, 0xed,
    0x35, 0x09, 0x8a, 0xa0, 0xa6, 0x1a, 0x3e, 0xe9,
    0xc9, 0xb1, 0x8d, 0x19, 0x7f, 0x39, 0x8b, 0x0e,
    0x42, 0x06, 0xce, 0xa9, 0xac, 0xf9, 0xc1, 0x97,
  }};
#endif

static char tinker_msgs__msg__MotorState__TYPE_NAME[] = "tinker_msgs/msg/MotorState";
static char builtin_interfaces__msg__Time__TYPE_NAME[] = "builtin_interfaces/msg/Time";

// Define type names, field names, and default values
static char tinker_msgs__msg__MotorState__FIELD_NAME__timestamp_state[] = "timestamp_state";
static char tinker_msgs__msg__MotorState__FIELD_NAME__position[] = "position";
static char tinker_msgs__msg__MotorState__FIELD_NAME__velocity[] = "velocity";
static char tinker_msgs__msg__MotorState__FIELD_NAME__torque[] = "torque";
static char tinker_msgs__msg__MotorState__FIELD_NAME__temperature_mosfet[] = "temperature_mosfet";
static char tinker_msgs__msg__MotorState__FIELD_NAME__temperature_rotor[] = "temperature_rotor";
static char tinker_msgs__msg__MotorState__FIELD_NAME__error[] = "error";

static rosidl_runtime_c__type_description__Field tinker_msgs__msg__MotorState__FIELDS[] = {
  {
    {tinker_msgs__msg__MotorState__FIELD_NAME__timestamp_state, 15, 15},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_NESTED_TYPE,
      0,
      0,
      {builtin_interfaces__msg__Time__TYPE_NAME, 27, 27},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__MotorState__FIELD_NAME__position, 8, 8},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__MotorState__FIELD_NAME__velocity, 8, 8},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__MotorState__FIELD_NAME__torque, 6, 6},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__MotorState__FIELD_NAME__temperature_mosfet, 18, 18},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_INT16,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__MotorState__FIELD_NAME__temperature_rotor, 17, 17},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_INT16,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {tinker_msgs__msg__MotorState__FIELD_NAME__error, 5, 5},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
};

static rosidl_runtime_c__type_description__IndividualTypeDescription tinker_msgs__msg__MotorState__REFERENCED_TYPE_DESCRIPTIONS[] = {
  {
    {builtin_interfaces__msg__Time__TYPE_NAME, 27, 27},
    {NULL, 0, 0},
  },
};

const rosidl_runtime_c__type_description__TypeDescription *
tinker_msgs__msg__MotorState__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {tinker_msgs__msg__MotorState__TYPE_NAME, 26, 26},
      {tinker_msgs__msg__MotorState__FIELDS, 7, 7},
    },
    {tinker_msgs__msg__MotorState__REFERENCED_TYPE_DESCRIPTIONS, 1, 1},
  };
  if (!constructed) {
    assert(0 == memcmp(&builtin_interfaces__msg__Time__EXPECTED_HASH, builtin_interfaces__msg__Time__get_type_hash(NULL), sizeof(rosidl_type_hash_t)));
    description.referenced_type_descriptions.data[0].fields = builtin_interfaces__msg__Time__get_type_description(NULL)->type_description.fields;
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "#uint8 DISABLING = 0\n"
  "uint8 DISABLING = 0\n"
  "#uint8 ENABLE = 0x01\n"
  "uint8 ENABLE = 1\n"
  "#uint8 OVERCURRENT = 0x0A\n"
  "uint8 OVERCURRENT = 10\n"
  "#uint8 OVERVOLTAGE = 0x08\n"
  "uint8 OVERVOLTAGE = 8\n"
  "#uint8 LOWVOLTAGE = 0x09\n"
  "uint8 LOWVOLTAGE = 9\n"
  "#uint8 OVERTEMPERATURE_MOSFET = 0x0B\n"
  "uint8 OVERTEMPERATURE_MOSFET = 11\n"
  "#uint8 OVERTEMPERATURE_ROTOR = 0x0C\n"
  "uint8 OVERTEMPERATURE_ROTOR = 12\n"
  "#uint8 LOSS_CONNECTION= 0x0D\n"
  "uint8 LOSS_CONNECTION= 13\n"
  "#uint8 OVERLOAD = 0x0E\n"
  "uint8 OVERLOAD = 14\n"
  "\n"
  "builtin_interfaces/Time timestamp_state\n"
  "float32 position\n"
  "float32 velocity\n"
  "float32 torque\n"
  "int16 temperature_mosfet\n"
  "int16 temperature_rotor\n"
  "uint8 error\n"
  "\n"
  "\n"
  "#     0x01: receive fail \n"
  "#     0x11: receive success\n"
  "#     0x02: send fail \n"
  "#     0x12: send success\n"
  "#     0x03: set baudrate fail\n"
  "#     0x13: set baudrate success\n"
  "#     0xEE: communication error";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
tinker_msgs__msg__MotorState__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {tinker_msgs__msg__MotorState__TYPE_NAME, 26, 26},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 810, 810},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
tinker_msgs__msg__MotorState__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[2];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 2, 2};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *tinker_msgs__msg__MotorState__get_individual_type_description_source(NULL),
    sources[1] = *builtin_interfaces__msg__Time__get_individual_type_description_source(NULL);
    constructed = true;
  }
  return &source_sequence;
}
