#!/bin/bash
# Сборка GUI и отладочных инструментов на ПК (ноутбук).
# Использование: ./scripts/build_gui.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$WORKSPACE_DIR"
echo "Сборка GUI stack: tinker_msgs + tinker_gui, tinker_joy, button_control, test_talker"
colcon build --symlink-install --base-paths src \
  --packages-select tinker_msgs tinker_gui tinker_joy button_control test_talker

if [ -f "$WORKSPACE_DIR/install/setup.bash" ]; then
  echo "source $WORKSPACE_DIR/install/setup.bash"
fi
