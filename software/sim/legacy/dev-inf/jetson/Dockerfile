FROM nvcr.io/nvidia/l4t-pytorch:r32.7.1-pth1.9-py3

ENV DEBIAN_FRONTEND=noninteractive
ENV CUDA_HOME=/usr/local/cuda
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib64:/usr/local/cuda/extras/CUPTI/lib64
ENV PATH=$PATH:$CUDA_HOME/bin

# 1. Install system dependencies
RUN apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 16FAAD7AF99A65E2 && \
        apt-get update -qq && \
        apt-get install -y --no-install-recommends \
            libosmesa6-dev \
            patchelf \
            libglew-dev \
            libglfw3-dev \
            libglu1-mesa-dev \
            libopenmpi-dev \
            libomp-dev \
            libgtk2.0-dev \
            zlib1g-dev \
            libjpeg-dev \
            libsqlite3-dev \
            libffi-dev \
            libdb++-dev \
            libgdbm-dev \
            libbz2-dev \
            libdb-dev \
            libnss3-dev \
            libgdm-dev \
            gettext-base \
            htop \
        && rm -rf /var/lib/apt/lists/*

# 2. Use existing Python 3.6 from base image
RUN ln -sf /usr/bin/python3.6 /usr/bin/python3 && \
    python3 -m pip install --upgrade pip setuptools wheel

# 3. Install MuJoCo 2.3.7
RUN wget https://github.com/deepmind/mujoco/releases/download/2.3.7/mujoco-2.3.7-linux-aarch64.tar.gz \
    && mkdir -p /opt/mujoco \
    && tar -xzf mujoco-2.3.7-linux-aarch64.tar.gz -C /opt/mujoco --strip-components=1 \
    && rm mujoco-2.3.7-linux-aarch64.tar.gz
ENV MUJOCO_VERSION=2.3.7
ENV MUJOCO_PATH=/opt/mujoco
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MUJOCO_PATH/bin


# 4. Install Python dependencies
RUN python3 -m pip install --no-cache-dir \
    numpy==1.19.5 \
    scipy==1.5.4 \
    pandas==1.1.5 \
    matplotlib==3.3.4 \
    opencv-python-headless==4.5.5.64 \
    scikit-learn==0.24.2 \
    pillow==8.4.0 \
    tensorboard==2.9.1 \
    pyyaml==5.4.1 \
    tqdm==4.62.3 \
    einops==0.4.1 \
    wandb==0.13.5 \
    ipython==7.16.3 \
    pexpect==4.8.0 \
    pyquaternion==0.9.9 \
    mujoco-py==2.1.2.14 \
    rospkg==1.4.0 \
    lcm==1.4.3 \
    chardet==5.0.0 \
    packaging==21.3 \
    -i https://pypi.tuna.tsinghua.edu.cn/simple

# 5. Configure environment variables
ENV MUJOCO_PY_MJKEY_PATH=/opt/mujoco/mjkey.txt \
    MJLIB_PATH=/opt/mujoco/bin/libmujoco.so \
    MJKEY_PATH=/opt/mujoco/mjkey.txtoco.so \
    MJKEY_PATH=/opt/mujoco/mjkey.txt

# 6. Fix library paths for ARM64
RUN ln -s /usr/lib/aarch64-linux-gnu/libjpeg.so /usr/lib/ \
    && ln -s /usr/lib/aarch64-linux-gnu/libopenjp2.so /usr/lib/ \
    && ldconfig

# 7. Cleanup
RUN apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# 8. Install PyTorch with C++ API
RUN wget https://nvidia.box.com/shared/static/h1z9sw4bb1ybi0rm3tu8qdj8hs05ljbm.whl -O torch-1.9.0-cp36-cp36m-linux_aarch64.whl \
    && pip3 install torch-1.9.0-cp36-cp36m-linux_aarch64.whl

# 9. Coping project from github
RUN git clone https://github.com/Romlose/Tinker_sber_jetson.git

WORKDIR /Tinker_sber_jetson
