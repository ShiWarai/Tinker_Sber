markdown# Alpha-Human-Gym: Running without Docker using Conda

This README provides instructions for setting up and running the **Alpha-Human-Gym** environment **without Docker**, using `conda` for dependency management. It includes environment setup, training, playback, sim-to-sim, and sim-to-real deployment on the **Alpha-Human-H1** robot.

---

## Prerequisites

Before installation, you **must** install the required simulation environments:

### 1. Install `isaacgym` and `legged_gym`

```bash
git clone https://github.com/leggedrobotics/legged_gym.git
Follow the official installation instructions in the repository. This includes:

NVIDIA Isaac Gym
Required dependencies and assets


Note: Isaac Gym must be properly installed and accessible from Python.


Installation
1. Create and Activate Conda Environment
bashconda create -n Alpha_Human_gym python=3.8
conda activate Alpha_Human_gym
2. Install Python Dependencies
bashpip install numpy==1.23.5 mujoco==2.3.7 mujoco-py mujoco-python-viewer
pip install dm_control==1.0.14 opencv-python matplotlib einops tqdm packaging h5py ipython getkey wandb chardet h5py_cache tensorboard pyquaternion pyyaml rospkg pexpect
3. Install PyTorch with CUDA 12.1
bashconda install pytorch==2.3.1 torchvision==0.18.1 torchaudio==2.3.1 pytorch-cuda=12.1 -c pytorch -c nvidia
4. Install CMake
bashconda install -c conda-forge cmake=3.25
5. Install isaacgym in Editable Mode
bashcd /path/to/isaacgym/python/
pip install -e .

Replace /path/to/isaacgym with your actual Isaac Gym installation path.


RL Training
Train the Policy
bashconda activate Alpha_Human_gym
cd Alpha_Human_gym/scripts/
python train.py
Common Fixes
Error: ImportError: libpython3.8.so.1.0: cannot open shared object file
bashexport LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/home/user/anaconda3/envs/Alpha_Human_gym/lib/

Add this line to your ~/.bashrc for persistence:
bashecho 'export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/home/user/anaconda3/envs/Alpha_Human_gym/lib/' >> ~/.bashrc

Error: AttributeError: module 'numpy' has no attribute 'float'
bashpip uninstall numpy -y
pip install numpy==1.23.5

Play Trained Model
After training, models are saved in the logs/ folder.
1. Update Model Path in play.py
pythonPLAY_DIR = 'logs/h1_constraint_trot/May31_21-37-30_/model_30000.pt'

Replace with your actual .pt model path.

2. Run Playback
bashpython play.py

Sim-to-Sim
Test policy transfer in simulation:
bashpython sim2sim.py