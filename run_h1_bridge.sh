#!/bin/bash

# run_h1_bridge.sh - Скрипт для запуска H1 ROS2 bridge

cd ~/RL/docker_rl/Alpha_Human_gym-main

echo "Building h1_ros2_bridge package..."
colcon build --packages-select h1_ros2_bridge

if [ $? -eq 0 ]; then
    echo "Build successful! Starting H1 MuJoCo node..."
    source install/setup.bash
    ros2 run h1_ros2_bridge h1_mujoco_node
else
    echo "Build failed! Please check errors above."
    exit 1
fi