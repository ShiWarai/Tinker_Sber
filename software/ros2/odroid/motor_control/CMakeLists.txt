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
find_package(hardware_msg REQUIRED)

AUX_SOURCE_DIRECTORY(src DIR_SRCS)
add_executable(Motor_control src/Motor_control.cpp src/comm.c src/serial.cpp src/spi.cpp src/sys_timer.cpp)
ament_target_dependencies(Motor_control rclcpp std_msgs sensor_msgs hardware_msg)
include_directories(./include)
# Reduce compile/link memory usage and output size on constrained devices
# target_compile_options(Motor_control PRIVATE -Os -g0 -ffunction-sections -fdata-sections)
# target_link_options(Motor_control PRIVATE -Wl,--gc-sections -Wl,--no-keep-memory -Wl,--reduce-memory-overheads)


install(TARGETS Motor_control
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch urdf
  DESTINATION share/${PROJECT_NAME}
)


ament_package()
