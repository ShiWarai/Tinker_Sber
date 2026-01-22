// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from tinker_msgs:msg/MotorState.idl
// generated code does not contain a copyright notice
#include "tinker_msgs/msg/detail/motor_state__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `timestamp_state`
#include "builtin_interfaces/msg/detail/time__functions.h"

bool
tinker_msgs__msg__MotorState__init(tinker_msgs__msg__MotorState * msg)
{
  if (!msg) {
    return false;
  }
  // timestamp_state
  if (!builtin_interfaces__msg__Time__init(&msg->timestamp_state)) {
    tinker_msgs__msg__MotorState__fini(msg);
    return false;
  }
  // position
  // velocity
  // torque
  // temperature_mosfet
  // temperature_rotor
  // error
  return true;
}

void
tinker_msgs__msg__MotorState__fini(tinker_msgs__msg__MotorState * msg)
{
  if (!msg) {
    return;
  }
  // timestamp_state
  builtin_interfaces__msg__Time__fini(&msg->timestamp_state);
  // position
  // velocity
  // torque
  // temperature_mosfet
  // temperature_rotor
  // error
}

bool
tinker_msgs__msg__MotorState__are_equal(const tinker_msgs__msg__MotorState * lhs, const tinker_msgs__msg__MotorState * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // timestamp_state
  if (!builtin_interfaces__msg__Time__are_equal(
      &(lhs->timestamp_state), &(rhs->timestamp_state)))
  {
    return false;
  }
  // position
  if (lhs->position != rhs->position) {
    return false;
  }
  // velocity
  if (lhs->velocity != rhs->velocity) {
    return false;
  }
  // torque
  if (lhs->torque != rhs->torque) {
    return false;
  }
  // temperature_mosfet
  if (lhs->temperature_mosfet != rhs->temperature_mosfet) {
    return false;
  }
  // temperature_rotor
  if (lhs->temperature_rotor != rhs->temperature_rotor) {
    return false;
  }
  // error
  if (lhs->error != rhs->error) {
    return false;
  }
  return true;
}

bool
tinker_msgs__msg__MotorState__copy(
  const tinker_msgs__msg__MotorState * input,
  tinker_msgs__msg__MotorState * output)
{
  if (!input || !output) {
    return false;
  }
  // timestamp_state
  if (!builtin_interfaces__msg__Time__copy(
      &(input->timestamp_state), &(output->timestamp_state)))
  {
    return false;
  }
  // position
  output->position = input->position;
  // velocity
  output->velocity = input->velocity;
  // torque
  output->torque = input->torque;
  // temperature_mosfet
  output->temperature_mosfet = input->temperature_mosfet;
  // temperature_rotor
  output->temperature_rotor = input->temperature_rotor;
  // error
  output->error = input->error;
  return true;
}

tinker_msgs__msg__MotorState *
tinker_msgs__msg__MotorState__create(void)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  tinker_msgs__msg__MotorState * msg = (tinker_msgs__msg__MotorState *)allocator.allocate(sizeof(tinker_msgs__msg__MotorState), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(tinker_msgs__msg__MotorState));
  bool success = tinker_msgs__msg__MotorState__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
tinker_msgs__msg__MotorState__destroy(tinker_msgs__msg__MotorState * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    tinker_msgs__msg__MotorState__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
tinker_msgs__msg__MotorState__Sequence__init(tinker_msgs__msg__MotorState__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  tinker_msgs__msg__MotorState * data = NULL;

  if (size) {
    data = (tinker_msgs__msg__MotorState *)allocator.zero_allocate(size, sizeof(tinker_msgs__msg__MotorState), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = tinker_msgs__msg__MotorState__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        tinker_msgs__msg__MotorState__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
tinker_msgs__msg__MotorState__Sequence__fini(tinker_msgs__msg__MotorState__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      tinker_msgs__msg__MotorState__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

tinker_msgs__msg__MotorState__Sequence *
tinker_msgs__msg__MotorState__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  tinker_msgs__msg__MotorState__Sequence * array = (tinker_msgs__msg__MotorState__Sequence *)allocator.allocate(sizeof(tinker_msgs__msg__MotorState__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = tinker_msgs__msg__MotorState__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
tinker_msgs__msg__MotorState__Sequence__destroy(tinker_msgs__msg__MotorState__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    tinker_msgs__msg__MotorState__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
tinker_msgs__msg__MotorState__Sequence__are_equal(const tinker_msgs__msg__MotorState__Sequence * lhs, const tinker_msgs__msg__MotorState__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!tinker_msgs__msg__MotorState__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
tinker_msgs__msg__MotorState__Sequence__copy(
  const tinker_msgs__msg__MotorState__Sequence * input,
  tinker_msgs__msg__MotorState__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(tinker_msgs__msg__MotorState);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    tinker_msgs__msg__MotorState * data =
      (tinker_msgs__msg__MotorState *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!tinker_msgs__msg__MotorState__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          tinker_msgs__msg__MotorState__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!tinker_msgs__msg__MotorState__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
