#!/bin/bash
# Сборка пакетов для одноплатника (управление моторами на роботе).
# Использование: ./scripts/build_robot.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$WORKSPACE_DIR"
echo "Сборка robot stack: tinker_msgs, motor_control, tinker_description"
colcon build --symlink-install --base-paths src \
  --packages-select tinker_msgs motor_control tinker_description

if [ -f "$WORKSPACE_DIR/install/setup.bash" ]; then
  echo "source $WORKSPACE_DIR/install/setup.bash"
fi
