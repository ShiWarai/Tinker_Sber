## Laptop node

Данная нода предназначена для отладки кода и проверки работоспособности элементов робота. Она отправляет управляющие сигналы для моторов.

Сборка проекта

```bash

cd laptop_node // Переход в нужную директорию
colcon build // Сборка проекта
```

## Запуск

Чтобы запустить ноду, впишите команду

```bash
source install/setup.bash
source /opt/ros/jazzy/setup.bash

ros2 run test_talker test_talker

```