import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description() -> LaunchDescription:
  pkg_share = get_package_share_directory('motor_control')
  urdf_path = os.path.join(pkg_share, 'urdf', 'robot.urdf')

  with open(urdf_path, 'r') as urdf_file:
    robot_description_content = urdf_file.read()

  robot_state_publisher = Node(
    package='robot_state_publisher',
    executable='robot_state_publisher',
    name='robot_state_publisher',
    output='screen',
    parameters=[{'robot_description': robot_description_content}]
  )

  rviz2 = Node(
    package='rviz2',
    executable='rviz2',
    name='rviz2',
    output='screen'
  )

  motor_control_node = Node(
    package='motor_control',
    executable='Motor_control',
    name='motor_control',
    output='screen',
    parameters=[{
        'limits.position.min': -3.14159,
        'limits.position.max': 3.14159,
        'limits.velocity.min': -20.0,
        'limits.velocity.max': 20.0,
        'limits.torque.min': -12.0,
        'limits.torque.max': 12.0,
        'limits.kp.min': 0.0,
        'limits.kp.max': 1000.0,
        'limits.kd.min': 0.0,
        'limits.kd.max': 100.0,
    }]
  )

  return LaunchDescription([
    robot_state_publisher,
    rviz2,
    motor_control_node,
  ])


