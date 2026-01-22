################################################################################
# Set minimum required version of cmake, project name and compile options
################################################################################
cmake_minimum_required(VERSION 3.5)
project(tinker_msgs)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

################################################################################
# Find ament packages and libraries for ament and system dependencies
################################################################################
find_package(action_msgs REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)

################################################################################
# Setup for python modules and scripts
################################################################################

################################################################################
# Declare ROS messages, services and actions
################################################################################
set(msg_files
  "msg/IMUState.msg"
  "msg/LowCmd.msg"
  "msg/LowState.msg"
  "msg/MotorCmd.msg"
  "msg/MotorState.msg"
  "msg/ControlCmd.msg"
  "msg/OneMotorCmd.msg"
)

#set(srv_files
# "srv/MotorList.srv"
#)


rosidl_generate_interfaces(${PROJECT_NAME}
  ${msg_files}
  #${srv_files}  
  DEPENDENCIES action_msgs std_msgs geometry_msgs builtin_interfaces
  ADD_LINTER_TESTS
)
ament_export_dependencies(rosidl_default_runtime)
ament_package()
