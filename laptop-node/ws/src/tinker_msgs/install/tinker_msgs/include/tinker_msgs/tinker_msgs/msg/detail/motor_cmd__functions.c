// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from tinker_msgs:msg/MotorCmd.idl
// generated code does not contain a copyright notice
#include "tinker_msgs/msg/detail/motor_cmd__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


bool
tinker_msgs__msg__MotorCmd__init(tinker_msgs__msg__MotorCmd * msg)
{
  if (!msg) {
    return false;
  }
  // position
  // velocity
  // torque
  // kp
  // kd
  return true;
}

void
tinker_msgs__msg__MotorCmd__fini(tinker_msgs__msg__MotorCmd * msg)
{
  if (!msg) {
    return;
  }
  // position
  // velocity
  // torque
  // kp
  // kd
}

bool
tinker_msgs__msg__MotorCmd__are_equal(const tinker_msgs__msg__MotorCmd * lhs, const tinker_msgs__msg__MotorCmd * rhs)
{
  if (!lhs || !rhs) {
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
  // kp
  if (lhs->kp != rhs->kp) {
    return false;
  }
  // kd
  if (lhs->kd != rhs->kd) {
    return false;
  }
  return true;
}

bool
tinker_msgs__msg__MotorCmd__copy(
  const tinker_msgs__msg__MotorCmd * input,
  tinker_msgs__msg__MotorCmd * output)
{
  if (!input || !output) {
    return false;
  }
  // position
  output->position = input->position;
  // velocity
  output->velocity = input->velocity;
  // torque
  output->torque = input->torque;
  // kp
  output->kp = input->kp;
  // kd
  output->kd = input->kd;
  return true;
}

tinker_msgs__msg__MotorCmd *
tinker_msgs__msg__MotorCmd__create(void)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  tinker_msgs__msg__MotorCmd * msg = (tinker_msgs__msg__MotorCmd *)allocator.allocate(sizeof(tinker_msgs__msg__MotorCmd), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(tinker_msgs__msg__MotorCmd));
  bool success = tinker_msgs__msg__MotorCmd__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
tinker_msgs__msg__MotorCmd__destroy(tinker_msgs__msg__MotorCmd * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    tinker_msgs__msg__MotorCmd__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
tinker_msgs__msg__MotorCmd__Sequence__init(tinker_msgs__msg__MotorCmd__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  tinker_msgs__msg__MotorCmd * data = NULL;

  if (size) {
    data = (tinker_msgs__msg__MotorCmd *)allocator.zero_allocate(size, sizeof(tinker_msgs__msg__MotorCmd), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = tinker_msgs__msg__MotorCmd__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        tinker_msgs__msg__MotorCmd__fini(&data[i - 1]);
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
tinker_msgs__msg__MotorCmd__Sequence__fini(tinker_msgs__msg__MotorCmd__Sequence * array)
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
      tinker_msgs__msg__MotorCmd__fini(&array->data[i]);
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

tinker_msgs__msg__MotorCmd__Sequence *
tinker_msgs__msg__MotorCmd__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  tinker_msgs__msg__MotorCmd__Sequence * array = (tinker_msgs__msg__MotorCmd__Sequence *)allocator.allocate(sizeof(tinker_msgs__msg__MotorCmd__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = tinker_msgs__msg__MotorCmd__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
tinker_msgs__msg__MotorCmd__Sequence__destroy(tinker_msgs__msg__MotorCmd__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    tinker_msgs__msg__MotorCmd__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
tinker_msgs__msg__MotorCmd__Sequence__are_equal(const tinker_msgs__msg__MotorCmd__Sequence * lhs, const tinker_msgs__msg__MotorCmd__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!tinker_msgs__msg__MotorCmd__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
tinker_msgs__msg__MotorCmd__Sequence__copy(
  const tinker_msgs__msg__MotorCmd__Sequence * input,
  tinker_msgs__msg__MotorCmd__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(tinker_msgs__msg__MotorCmd);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    tinker_msgs__msg__MotorCmd * data =
      (tinker_msgs__msg__MotorCmd *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!tinker_msgs__msg__MotorCmd__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          tinker_msgs__msg__MotorCmd__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!tinker_msgs__msg__MotorCmd__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
