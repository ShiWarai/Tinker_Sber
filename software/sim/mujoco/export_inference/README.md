# MuJoCo inference (sim2sim)

ROS 2 sim2sim stack: MuJoCo digital twin + ONNX policy controller over `tinker_msgs`.

Previously this path was a broken git submodule (gitlink without `.gitmodules`). The code is vendored here as regular files so `git clone` works out of the box.

## Layout

```
export_inference/
├── mujoco/          # MuJoCo twin (publishes LowState, subscribes LowCmd)
│   ├── sim_mujoco.py
│   ├── xml/
│   └── meshes/
└── controller/      # RL inference node (keyboard/gamepad → LowCmd)
    ├── run_inference.py
    └── src/
        └── model/tinker/   # params.yaml + policy/policy.onnx
```

Message definitions live in `software/ros2/laptop/tinker_msgs/` (do not duplicate them here).

Training environment: `../mujoco_playground_learnimg/`.

## Prerequisites

- ROS 2 Jazzy
- Python 3.10+
- MuJoCo, ONNX Runtime, SciPy, PyYAML, pynput

Build `tinker_msgs` in your workspace:

```bash
cd ~/ws
colcon build --packages-select tinker_msgs --symlink-install
source install/setup.bash
```

Install Python deps (in a venv or system Python):

```bash
pip install -r requirements.txt
```

## Run (two terminals)

**Terminal 1 — MuJoCo twin:**

```bash
source /opt/ros/jazzy/setup.bash
source ~/ws/install/setup.bash
cd software/sim/mujoco/export_inference/mujoco
python3 sim_mujoco.py
```

**Terminal 2 — inference controller:**

```bash
source /opt/ros/jazzy/setup.bash
source ~/ws/install/setup.bash
cd software/sim/mujoco/export_inference/controller
python3 run_inference.py
```

Keyboard: `W/S/A/D` — linear velocity, `Q/E` — yaw. See `src/devices.py`.

## Topics

| Topic | Type | Direction (twin) | Direction (controller) |
|---|---|---|---|
| `/tinker_msgs/lowstate` | `tinker_msgs/LowState` | publish | subscribe |
| `/tinker_msgs/lowcmd` | `tinker_msgs/LowCmd` | subscribe | publish |

Replace `policy/policy.onnx` with a model exported from `mujoco_playground_learnimg` training when ready.
