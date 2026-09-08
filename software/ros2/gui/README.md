# ROS 2 GUI tools

Инструменты управления роботом с ПК. Используют unified `tinker_msgs` из `software/ros2/src/tinker_msgs`.

## Пакеты

| Пакет | Источник | Назначение |
|---|---|---|
| `tinker_gui` | `ros2-gui/unified_messages` | PyQt5-слайдеры: `/low_level_command`, `/control_command`, `/single_motor_command` |
| `tinker_joy` | `ros2-gui/unified_messages` | Launch для джойстика (`teleop_twist_joy`) |
| `button_control` | `dev-software-ros2-gui-buttons` | Qt-кнопки Start/Stop/Standing/Lying |

## Топики (unified stack)

- `/low_level_state` — `tinker_msgs/LowState`
- `/low_level_command` — `tinker_msgs/LowCmd`
- `/control_command` — `tinker_msgs/ControlCmd`
- `/single_motor_command` — `tinker_msgs/OneMotorCmd`
- `/imu_state` — `sensor_msgs/Imu` (отдельно от `LowState`)

## Сборка

Из корня workspace (рядом с `software/ros2/src/`):

```bash
colcon build --packages-select tinker_msgs button_control tinker_gui tinker_joy
source install/setup.bash
ros2 launch tinker_gui gui_system.launch.py
```

`button_control` — отдельный исполняемый файл после сборки пакета.
