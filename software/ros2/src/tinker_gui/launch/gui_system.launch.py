from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import SetEnvironmentVariable


def generate_launch_description():
    return LaunchDescription(
        [
            SetEnvironmentVariable("QT_QPA_PLATFORM", "xcb"),
            Node(
                package="tinker_gui",
                executable="gui_slider_node.py",
                name="gui_slider_node",
                output="screen",
            ),
        ]
    )
