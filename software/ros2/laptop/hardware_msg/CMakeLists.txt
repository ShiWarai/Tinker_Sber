cmake_minimum_required(VERSION 3.8)
project(hardware_msg)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/BoardParameters.msg"
  "msg/ImuParameters.msg" 
  "msg/MotorParameters.msg"  
  "msg/MotorsStates.msg"
  "msg/Imu.msg"              
  "msg/MotorData.msg"      
  "msg/MotorsCommands.msg"

  # DEPENDENCIES geometry_msgs 
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
