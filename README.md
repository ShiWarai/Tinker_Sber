## Данный код еще в процессе разработки

Для запуска ноды нужно установить Qt5:

```
sudo apt-get install build-essential libgl1-mesa-dev
sudo apt install qtbase5-dev cmake
```

Рабочие кнопки - Start motors, Set zero position, Standing / Lying down.

GUI проверяет состояние моторов по топику `/low_level_state`, а не по истории нажатий.

Для сборки и запуска:

```
cd button_control
source /opt/ros/humble/setup.bash
colcon build
source install/setup.bash
ros2 run button_control button_control
```
