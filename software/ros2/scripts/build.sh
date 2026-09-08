#!/bin/bash
# Сборка всех ROS 2 пакетов из src/.
# Для частичной сборки используйте build_robot.sh или build_gui.sh.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$WORKSPACE_DIR"
echo "Сборка всех пакетов из src/"
colcon build --symlink-install --base-paths src

if [ -f "$WORKSPACE_DIR/install/setup.bash" ]; then
  echo "source $WORKSPACE_DIR/install/setup.bash"
fi
