from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description() -> LaunchDescription:
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
            'limits.kp.max': 15.0,
            'limits.kd.min': 0.0,
            'limits.kd.max': 5.0,
        }]
    )

    return LaunchDescription([
        motor_control_node,
    ])

