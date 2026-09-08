# MuJoCo inference (sim2sim)

ROS 2 sim2sim: MuJoCo digital twin + ONNX policy controller.

**Requires** `tinker_msgs` from `software/ros2/src/tinker_msgs/` (merge `ros2-node/unified_messages` first).

## Layout

```
export_inference/
├── mujoco/          # twin: /low_level_state + /imu_state
│   ├── sim_mujoco.py
│   ├── xml/
│   └── meshes/
└── controller/      # inference → /low_level_command
    ├── run_inference.py
    └── src/model/tinker/
```

## Prerequisites

- ROS 2 Jazzy, `tinker_msgs` built from `software/ros2/src/`
- Python: `pip install -r requirements.txt`

```bash
cd ~/ws/src/Tinker_Sber/software/ros2
colcon build --packages-select tinker_msgs --symlink-install
source ~/ws/install/setup.bash
```

## Run (two terminals)

**Terminal 1 — MuJoCo twin:**

```bash
source /opt/ros/jazzy/setup.bash
source ~/ws/install/setup.bash
cd software/sim/mujoco/export_inference/mujoco
python3 sim_mujoco.py
```

**Terminal 2 — inference:**

```bash
source /opt/ros/jazzy/setup.bash
source ~/ws/install/setup.bash
cd software/sim/mujoco/export_inference/controller
python3 run_inference.py
```

## Topics (same as motor_control)

| Topic | Type | Twin | Controller |
|---|---|---|---|
| `/low_level_state` | `tinker_msgs/LowState` | publish | subscribe |
| `/imu_state` | `sensor_msgs/Imu` | publish | subscribe |
| `/low_level_command` | `tinker_msgs/LowCmd` | subscribe | publish |

Training: `../mujoco_playground_learnimg/`.
