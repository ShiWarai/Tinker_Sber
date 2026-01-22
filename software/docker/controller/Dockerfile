ARG ROS_DISTRO=jazzy
FROM osrf/ros:${ROS_DISTRO}-desktop

RUN apt-get update && apt-get install -y \
    python3-pip \
    python3-venv \
    python3-catkin-pkg-modules \
    python3-colcon-common-extensions \
    python3-empy \
    python3-lark \
    iputils-ping \
    libgl1 \
    libglx-mesa0 \
    libglib2.0-0 \
    ros-${ROS_DISTRO}-ament-cmake \
    ros-${ROS_DISTRO}-ros-workspace \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir \
    empy==3.3.4 \
    torch==2.3.0 \
    torchvision==0.18.0 \
    numpy \
    pynput

WORKDIR /workspace

COPY . /workspace/

RUN . /opt/ros/$ROS_DISTRO/setup.sh && \
    cd /workspace/tinker_msgs && \
    colcon build --symlink-install

RUN echo "source /workspace/tinker_msgs/install/setup.bash" >> ~/.bashrc && \
    echo "source /opt/ros/$ROS_DISTRO/setup.bash" >> ~/.bashrc

RUN chmod +x /workspace/inference_controller_setup.py