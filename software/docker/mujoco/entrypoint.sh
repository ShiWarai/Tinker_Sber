#!/bin/bash
set -e
source /opt/ros/${ROS_DISTRO}/setup.bash
source /workspace/tinker_msgs/install/setup.bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
exec "$@"