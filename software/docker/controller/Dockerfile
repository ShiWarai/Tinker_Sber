ARG ROS_DISTRO=jazzy
FROM osrf/ros:${ROS_DISTRO}-desktop

ARG ROS_DISTRO=jazzy
ENV ROS_DISTRO=${ROS_DISTRO}
ENV ROS_DOMAIN_ID=5

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
    ros-${ROS_DISTRO}-pinocchio \
    ros-${ROS_DISTRO}-rmw-cyclonedds-cpp \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir --break-system-packages \
    empy==3.3.4 \
    torch==2.3.0 \
    torchvision==0.18.0 \
    numpy \
    pynput \
    onnxruntime \
    scipy \
    pyyaml

WORKDIR /workspace

COPY . /workspace/

RUN . /opt/ros/${ROS_DISTRO}/setup.sh && \
    cd /workspace/tinker_msgs && \
    colcon build --symlink-install

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
RUN chmod +x /workspace/inference_controller_setup.py

ENTRYPOINT ["/entrypoint.sh"]
CMD ["python3", "/workspace/inference_controller_setup.py"]