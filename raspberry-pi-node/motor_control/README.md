# Motor Control Package

ROS2 пакет для управления моторами через SPI интерфейс.

## Описание

Пакет обеспечивает взаимодействие между ROS2 и аппаратным контроллером моторов через SPI. Поддерживает управление 10 моторами, получение данных с IMU и энкодеров, автоматическое преобразование единиц измерения (радианы ↔ градусы) и ограничение параметров для безопасности.

## Запуск

```bash
ros2 launch motor_control motor_control.launch.py
```

## Топики

### Подписки
- `/low_level_command` (`tinker_msgs/msg/LowCmd`) - команды для всех 10 моторов
- `/single_motor_command` (`tinker_msgs/msg/OneMotorCmd`) - команда для одного мотора
- `/control_command` (`tinker_msgs/msg/ControlCmd`) - системные команды (ENABLE/DISABLE, калибровка IMU)

### Публикации
- `/low_level_state` (`tinker_msgs/msg/LowState`) - состояние всех моторов и IMU
- `/imu_state` (`tinker_msgs/msg/IMUState`) - данные IMU
- `/robot_joints` (`sensor_msgs/msg/JointState`) - состояния суставов для визуализации

## Параметры

Все параметры настраиваются через launch файл:

- `limits.position.min` / `limits.position.max` - лимиты позиции в радианах (по умолчанию: ±π)
- `limits.velocity.min` / `limits.velocity.max` - лимиты скорости в рад/с (по умолчанию: ±20.0)
- `limits.torque.min` / `limits.torque.max` - лимиты момента в Н·м (по умолчанию: ±12.0)
- `limits.kp.min` / `limits.kp.max` - лимиты коэффициента kp (по умолчанию: 0.0-1000.0)
- `limits.kd.min` / `limits.kd.max` - лимиты коэффициента kd (по умолчанию: 0.0-100.0)

## Особенности

- Автоматическое преобразование единиц: ROS2 использует радианы, SPI плата работает в градусах
- Ограничение параметров с предупреждениями при превышении лимитов
- Частота обновления: 1000 Гц
- Поддержка 10 моторов

## Примеры использования

### Включить двигатели
```bash
ros2 topic pub --once /control_command tinker_msgs/msg/ControlCmd "{motor_id: 0, cmd: 252}"
```

### Отключить двигатели
```bash
ros2 topic pub --once /control_command tinker_msgs/msg/ControlCmd "{motor_id: 0, cmd: 253}"
```

### Повернуть мотор 0 на 0.5 радиан
```bash
ros2 topic pub --once /single_motor_command tinker_msgs/msg/OneMotorCmd "{motor_id: 0, motor_group: 0, position: 0.5, velocity: 0.0, torque: 0.0, kp: 1.0, kd: 0.1}"
```

### Просмотр состояния
```bash
ros2 topic echo /low_level_state
```

