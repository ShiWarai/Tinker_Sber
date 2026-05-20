# Inference RL Controller

## About


## Installation and launch

- `xhost +local:docker`

**Build container** from **/software/docker/controller/** path:
```bash
docker build -t gait-controller:jazzy .
```

then **run it**:
```bash
docker run -it --net host --ipc host --gpus all \
  -e DISPLAY=$DISPLAY \
  -e CYCLONEDDS_URI=file:///tmp/cyclone.xml \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v /tmp/cyclone.xml:/tmp/cyclone.xml \
  gait_controller:jazzy

```

