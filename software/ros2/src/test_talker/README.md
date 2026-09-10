# test_talker

Отладочный talker с ноутбука: синусоидальная траектория на `/low_level_command`.

Пакет в `software/ros2/src/test_talker`. Использует `tinker_msgs` из того же workspace.

```bash
colcon build --packages-select tinker_msgs test_talker
source install/setup.bash
ros2 run test_talker test_talker
```
