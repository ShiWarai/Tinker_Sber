cmake_minimum_required(VERSION 3.8)
project(motor_control)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tinker_msgs REQUIRED)

# Исполняемый файл: ROS-узел (motor_control_node.cpp) + протокол SPI (motor_control.cpp) + драйвер SPI (spi.cpp)
add_executable(motor_control
  src/motor_control_node.cpp
  src/motor_control.cpp
  src/spi.cpp
)
ament_target_dependencies(motor_control rclcpp std_msgs sensor_msgs tinker_msgs)
include_directories(./include)

install(TARGETS motor_control
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
