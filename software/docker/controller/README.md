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
docker run -it --net host --ipc host -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix gait-controller:jazzy
```

To add changes from the src/ executable files, without affecting the message packages:
- `-v $(pwd)/src:/workspace/src`, when *run* the container

**Start the application:**
```bash
python3 inference_controller_setup.py
```

If needs to see arguments, run:
```bash
python3 inference_controller_setup.py --help
```


## ROS2 node
