# Inference RL Controller

## About

## Installation and launch

- `xhost +local:docker`

In **.../Tinker_Sber/software/docker/controller** path:
- `docker build -t gait-controller:jazzy .`
- `docker run -it --net=host -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix gait-controller:jazzy`

### Для связи с контейнером gym:foxy **на одном устройстве**:

Сборка контейнера на ros2 foxy (необходимо только для связи с IsaacGym):
- `docker build -t gait_controller:foxy --build-arg ROS_DISTRO=foxy .`

Для сборки на ros2 jazzy оставить `docker build -t gait-controller:jazzy .`

В основном терминале:
- `docker network create ros-net`
- `docker network connect ros-net <jazzy_container>`
- `docker network connect ros-net <foxy_container>`

где <> - имена контейнеров (без скобок <>)

Или указать при запуске:
- `docker run -it --network=ros-net --env ROS_DOMAIN_ID=1 ...`

ROS_DOMAIN_ID= - любой **одинаковый** для обоих контейнеров *и всех последующих связанных нод*

### При изменениях в исполняемых файлах src/, не затрагивая пакеты сообщений:
- `-v $(pwd)/src:/workspace/src`

### Полная команда запуска контейнера для работы с **IsaacGym** на одном устройстве
- `docker run -it --network=ros-net --env ROS_DOMAIN_ID=1 -v $(pwd)/src:/workspace/src -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix gait-controller:foxy`

## ROS2 node
