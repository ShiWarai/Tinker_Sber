from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
import os

def generate_launch_description():
    # Get project root directory
    project_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    
    return LaunchDescription([
        DeclareLaunchArgument(
            'model_path',
            default_value=os.path.join(project_root, 'scripts/model/trot.pt'),
            description='Path to the trained model'
        ),
        
        DeclareLaunchArgument(
            'mujoco_xml', 
            default_value=os.path.join(project_root, 'resources/h1/xml/world.xml'),
            description='Path to MuJoCo XML file'
        ),
        
        DeclareLaunchArgument(
            'sim_duration',
            default_value='60.0',
            description='Simulation duration in seconds'
        ),
        
        Node(
            package='h1_ros2_bridge',
            executable='h1_mujoco_node',
            name='h1_mujoco_node',
            output='screen',
            parameters=[{
                'model_path': LaunchConfiguration('model_path'),
                'mujoco_xml': LaunchConfiguration('mujoco_xml'),
                'sim_duration': LaunchConfiguration('sim_duration'),
            }]
        )
    ])