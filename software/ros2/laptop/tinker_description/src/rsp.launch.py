import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration
from launch.actions import DeclareLaunchArgument
from launch_ros.actions import Node

import xacro


def generate_launch_description():

    # Check if we're told to use sim time
    use_sim_time = LaunchConfiguration('use_sim_time')
    package_name='tinker_description'
    # Process the URDF file
    pkg_path = os.path.join(get_package_share_directory('tinker_description'))
    xacro_file = os.path.join(pkg_path,'urdf','robot.urdf.xacro')
    robot_description_config = xacro.process_file(xacro_file)
    
    params = {'robot_description': robot_description_config.toxml(),  'use_sim_time': use_sim_time}


    rviz2 = Node(
            package='rviz2',
            namespace='',
            executable='rviz2',
            name='rviz2',
            arguments=['-d', [os.path.join(get_package_share_directory(package_name), 'config', 'urdf_core.rviz')]],
            parameters = [{'use_sim_time': use_sim_time}]
        )

    return LaunchDescription([
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='true',
            description='Use sim time if true'),
        rviz2,
    ])
