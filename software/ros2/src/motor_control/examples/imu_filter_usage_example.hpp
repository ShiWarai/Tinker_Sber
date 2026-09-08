/**
 * @file imu_filter_usage_example.hpp
 * @brief Пример публикации IMU в ROS2 (справочный черновик).
 *
 * Актуальная интеграция — в motor_control_node.cpp:
 *   /imu_state       → sensor_msgs/msg/Imu
 *   /imu_orientation → geometry_msgs/msg/Quaternion  (w,x,y,z фильтра)
 * RPY только в логе ноды (из quatToEuler).
 */

#ifndef IMU_FILTER_USAGE_EXAMPLE_HPP
#define IMU_FILTER_USAGE_EXAMPLE_HPP

#include "geometry_msgs/msg/quaternion.hpp"
#include "sensor_msgs/msg/imu.hpp"

#endif /* IMU_FILTER_USAGE_EXAMPLE_HPP */
