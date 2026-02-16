cmake_minimum_required(VERSION 3.8)
project(tinker_gui)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclpy REQUIRED)
find_package(tinker_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)

ament_python_install_package(${PROJECT_NAME})

install(
  PROGRAMS
    tinker_gui/gui_slider_node.py
    tinker_gui/test_data_publisher.py
    tinker_gui/commands_to_states_bridge.py
    tinker_gui/states_to_joint_states.py
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

install(
  DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmakecopyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
