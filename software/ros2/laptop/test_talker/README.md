# test_talker

Отладочный talker с ноутбука: синусоидальная траектория на `/low_level_command`.

Источник: `dev-software-ros2-laptop-node` (без vendored `tinker_msgs` — использует `software/ros2/src/tinker_msgs`).

```bash
colcon build --packages-select tinker_msgs test_talker
source install/setup.bash
ros2 run test_talker test_talker
```
