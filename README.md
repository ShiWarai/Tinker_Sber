## Данный код еще в процессе разработки

Для запуска ноды нужно установить QT


```
sudo apt-get install build-essential libgl1-mesa-dev
```


```
sudo apt install qt6-base-dev cmake
```


Рабочие кнопки - Activate motors, Set zero position, Move zero position (но с ней проблемы дёргания двигателей возникают лютые).

В данный момент данные сразу публикуются в топики управления двигателями.

Для запуска ноды:

```
cd button_control/src
colcon build
ros2 run button_control button_control
```