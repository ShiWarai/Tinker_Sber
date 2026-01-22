# Alpha-Human-Gym: Running without Docker using Conda

This repository provides instructions for setting up and running the **Alpha-Human-Gym** environment **without Docker**, using `conda` for dependency management. It includes environment setup, training, playback, sim-to-sim, and sim-to-real deployment on the **Alpha-Human-H1** robot.

---

## Prerequisites

Before installation, you **must** install the required simulation environments:

### 1. Install `isaacgym` and `legged_gym`

```bash
git clone https://github.com/leggedrobotics/legged_gym.git
```
### Follow the official installation instructions in the repository. This includes:

NVIDIA Isaac Gym

Required dependencies and assets

Note: Isaac Gym must be properly installed and accessible from Python.

Installation
1. Create and Activate Conda Environment
```bash
conda create -n Alpha_Human_gym python=3.8
conda activate Alpha_Human_gym
```
2. Install Python Dependencies
```bash
pip install numpy==1.23.5 mujoco==2.3.7 mujoco-py mujoco-python-viewer
pip install dm_control==1.0.14 opencv-python matplotlib einops tqdm packaging h5py ipython getkey wandb chardet h5py_cache tensorboard pyquaternion pyyaml rospkg pexpect pynput
```
3. Install PyTorch with CUDA 12.1
```bash
conda install pytorch==2.3.1 torchvision==0.18.1 torchaudio==2.3.1 pytorch-cuda=12.1 -c pytorch -c nvidia
```
4. Install CMake
```bash
conda install -c conda-forge cmake=3.25
```
5. Install isaacgym in Editable Mode
```bash
cd /path/to/isaacgym/python/
pip install -e .
```
Replace /path/to/isaacgym with your actual Isaac Gym installation path.

RL Training
Train the Policy
```bash
conda activate Alpha_Human_gym
cd Alpha_Human_gym/scripts/
python train.py
```
Common Fixes
Error: AttributeError: module 'numpy' has no attribute 'float'
```bash
pip uninstall numpy -y
pip install numpy==1.23.5
Play Trained Model
```
After training, models are saved in the logs/ folder.

1. Update Model Path in play.py
python
PLAY_DIR = 'logs/h1_constraint_trot/May31_21-37-30_/model_30000.pt'
Replace with your actual .pt model path.

2. Run Playback
```bash
python play.py
```
Sim-to-Sim
Test policy transfer in simulation:

```bash
python sim2sim.py
```
Notes
 * Make sure all paths are correctly set according to your system configuration

 * Ensure proper CUDA version compatibility with your GPU drivers

 * For sim-to-real deployment, additional hardware-specific configurations may be required
