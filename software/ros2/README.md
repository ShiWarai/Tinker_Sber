# ROS 2 workspace

Все пакеты лежат в **`src/`**. Общий API сообщений — **`tinker_msgs`**.

## Структура `src/`

| Пакет | Где запускается | Назначение |
|---|---|---|
| `tinker_msgs` | везде | Unified low-level сообщения |
| `motor_control` | одноплатник на роботе | SPI ↔ STM32, `/low_level_*` |
| `tinker_description` | робот / RViz | URDF, меши, launch для визуализации |
| `tinker_gui` | ПК | PyQt5-слайдеры моторов |
| `tinker_joy` | ПК | Launch для джойстика |
| `button_control` | ПК | Qt-кнопки Start/Stop/Standing/Lying |
| `test_talker` | ПК | Отладочный синусоидальный talker |

## Сборка

Из каталога `software/ros2/`:

```bash
# Только робот (одноплатник)
./scripts/build_robot.sh

# Только GUI и отладка (ноутбук)
./scripts/build_gui.sh

# Всё сразу
./scripts/build.sh

source install/setup.bash
```

## Топики (unified stack)

| Топик | Тип |
|---|---|
| `/low_level_state` | `tinker_msgs/LowState` |
| `/low_level_command` | `tinker_msgs/LowCmd` |
| `/control_command` | `tinker_msgs/ControlCmd` |
| `/single_motor_command` | `tinker_msgs/OneMotorCmd` |
| `/imu_state` | `sensor_msgs/Imu` |

Примеры команд — в [COMMANDS.md](COMMANDS.md).

## Запуск

```bash
# На роботе
ros2 launch motor_control motor_control.launch.py

# GUI на ПК
ros2 launch tinker_gui gui_system.launch.py
ros2 run button_control button_control
ros2 run test_talker test_talker
```
