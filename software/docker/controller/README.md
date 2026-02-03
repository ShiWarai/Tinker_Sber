# Inference RL Controller

## About


## Installation and launch

- `xhost +local:docker`

Build container from **/software/docker/controller/** path:
- `docker build -t gait-controller:jazzy .`

then run it:
- `docker run -it --net=host -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix gait-controller:jazzy`

### For changes to the src/ executable files, without affecting the message packages:
- `-v $(pwd)/src:/workspace/src`, when *run* the container


## ROS2 node
