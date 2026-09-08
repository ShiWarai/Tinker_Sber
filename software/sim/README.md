# Simulation (`software/sim`)

MuJoCo-based simulation and RL for Tinker.

## Contents

| Path | Purpose |
|---|---|
| `mujoco/export_inference/` | Sim2sim: MuJoCo twin + ONNX inference over ROS 2 `tinker_msgs` |
| `mujoco/mujoco_playground_learnimg/` | Training (Brax PPO, env `BDJoystickFlatTerrain`) |

ROS message package: `software/ros2/laptop/tinker_msgs/`.

## Quick links

- [export_inference README](mujoco/export_inference/README.md) — run sim2sim inference
- [mujoco_playground_learnimg README](mujoco/mujoco_playground_learnimg/README.md) — train a policy
