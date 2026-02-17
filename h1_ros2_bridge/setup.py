from setuptools import setup, find_packages
import os
from glob import glob

package_name = 'h1_ros2_bridge'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), 
         glob('h1_ros2_bridge/launch/*.launch.py')),  # ИСПРАВЛЕНО путь
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='dzirt',
    maintainer_email='dzirt@example.com',
    description='ROS 2 bridge for H1 robot MuJoCo simulation',
    license='BSD-3-Clause',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'h1_mujoco_node = h1_ros2_bridge.h1_mujoco_node:main',
        ],
    },
)