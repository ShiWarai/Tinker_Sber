# Команды управления моторами и IMU

Пакеты и сборка: см. [README.md](README.md). Все пакеты — в `src/`, сообщения — `tinker_msgs`.

## Топики

| Топик | Тип сообщения | Назначение |
|-------|---------------|------------|
| `/control_command` | `tinker_msgs/msg/ControlCmd` | Системные команды (вкл/выкл, калибровка) |
| `/single_motor_command` | `tinker_msgs/msg/OneMotorCmd` | Команда для одного мотора |
| `/low_level_command` | `tinker_msgs/msg/LowCmd` | Команды для всех 10 моторов |
| `/low_level_state` | `tinker_msgs/msg/LowState` | Состояние моторов (чтение) |
| `/imu_state` | `sensor_msgs/msg/Imu` | IMU: ориентация, гироскоп, акселерометр |
| `/imu_orientation` | `geometry_msgs/msg/Quaternion` | Ориентация фильтра `[w,x,y,z]` |
| `/robot_joints` | `sensor_msgs/msg/JointState` | Позиции суставов для RViz |

## Коды команд ControlCmd

| Код | Константа | Описание |
|-----|-----------|----------|
| 252 | ENABLE | Включить моторы |
| 253 | DISABLE | Выключить моторы |
| 254 | SET_ZERO_POSITION | Обнулить позицию (только при выключенных моторах) |
| 251 | CLEAR_ERROR | Сбросить ошибки |
| 250 | IMU_CALIBRATE | Калибровка IMU |

## Примеры команд

### Включение моторов

```bash
ros2 topic pub --once /control_command tinker_msgs/msg/ControlCmd "{motor_id: 0, cmd: 252}"
```

### Выключение моторов

```bash
ros2 topic pub --once /control_command tinker_msgs/msg/ControlCmd "{motor_id: 0, cmd: 253}"
```

### Обнуление позиции моторов

Перед обнулением моторы должны быть выключены (DISABLE).

```bash
ros2 topic pub --once /control_command tinker_msgs/msg/ControlCmd "{motor_id: 0, cmd: 254}"
```

### Сброс ошибок

```bash
ros2 topic pub --once /control_command tinker_msgs/msg/ControlCmd "{motor_id: 0, cmd: 251}"
```

### Калибровка IMU

```bash
ros2 topic pub --once /control_command tinker_msgs/msg/ControlCmd "{motor_id: 0, cmd: 250}"
```

### Задать ПИД одному мотору (kp=2, kd=0.05)

По одному мотору (motor_id 0–9):

```bash
# Мотор 0 (joint_l_yaw)
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 0, position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05}"

# Мотор 1 (joint_l_roll)
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 1, position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05}"

# Мотор 2 (joint_l_pitch)
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 2, position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05}"

# Мотор 3 (joint_l_knee)
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 3, position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05}"

# Мотор 4 (joint_l_ankle)
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 4, position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05}"

# Мотор 5 (joint_r_yaw)
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 5, position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05}"

# Мотор 6 (joint_r_roll)
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 6, position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05}"

# Мотор 7 (joint_r_pitch)
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 7, position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05}"

# Мотор 8 (joint_r_knee)
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 8, position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05}"

# Мотор 9 (joint_r_ankle)
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 9, position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05}"
```

Все 10 моторов одной командой (kp=2, kd=0.05):

```bash
ros2 topic pub --once /low_level_command tinker_msgs/msg/LowCmd "{motor_cmd: [
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 2.0, kd: 0.05}
]}"
```

### Задать угол одному мотору

Параметры:
- `motor_id` — ID мотора (0–9)
- `position` — угол в радианах
- `velocity` — скорость в рад/с
- `torque` — момент в Н·м
- `kp`, `kd` — коэффициенты ПД-регулятора

```bash
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 0, position: 1.57, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.05}"
```

Примеры углов:
- 90° = 1.57 рад
- 45° = 0.785 рад
- 180° = 3.14 рад

### Задать углы всем 10 моторам

```bash
ros2 topic pub --once /low_level_command tinker_msgs/msg/LowCmd "{motor_cmd: [
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.05},
  {position: 0.0, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.05}
]}"
```

## Чтение данных

### Чтение состояния IMU

```bash
ros2 topic echo /imu_state
ros2 topic echo /imu_orientation
```

Одно сообщение:

```bash
ros2 topic echo /imu_state --once
```

### Чтение состояния всех моторов

```bash
ros2 topic echo /low_level_state
```

### Чтение позиций суставов (для RViz)

```bash
ros2 topic echo /robot_joints
```

## Типичная последовательность

```bash
# 1. Включить моторы
ros2 topic pub --once /control_command tinker_msgs/msg/ControlCmd "{motor_id: 0, cmd: 252}"

# 2. Задать угол мотору 0 (90 градусов)
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 0, position: 1.57, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.05}"

# 3. Выключить моторы
ros2 topic pub --once /control_command tinker_msgs/msg/ControlCmd "{motor_id: 0, cmd: 253}"
```

## Лимиты параметров (по умолчанию)

| Параметр | Мин | Макс |
|----------|-----|------|
| position | -π (-3.14) рад | +π (+3.14) рад |
| velocity | -20.0 рад/с | +20.0 рад/с |
| torque | -12.0 Н·м | +12.0 Н·м |
| kp | 0.0 | 1000.0 |
| kd | 0.0 | 100.0 |

## Названия суставов (motor_id)

| ID | Название |
|----|----------|
| 0 | joint_l_yaw |
| 1 | joint_l_roll |
| 2 | joint_l_pitch |
| 3 | joint_l_knee |
| 4 | joint_l_ankle |
| 5 | joint_r_yaw |
| 6 | joint_r_roll |
| 7 | joint_r_pitch |
| 8 | joint_r_knee |
| 9 | joint_r_ankle |
