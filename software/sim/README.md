# Simulation (`software/sim`)

MuJoCo-based simulation and RL for Tinker.

## Contents

| Path | Purpose |
|---|---|
| `mujoco/export_inference/` | Sim2sim: MuJoCo twin + ONNX inference over ROS 2 `tinker_msgs` |
| `mujoco/mujoco_playground_learnimg/` | Training (Brax PPO, env `BDJoystickFlatTerrain`) |
| `legacy/dev-inf/` | Archive: docker sim2sim + inference from branch `dev-inf` (Feb 2026) |

ROS message package: `software/ros2/src/tinker_msgs/` (after merge `ros2-node/unified_messages`).

## Quick links

- [export_inference README](mujoco/export_inference/README.md) — run sim2sim inference
- [mujoco_playground_learnimg README](mujoco/mujoco_playground_learnimg/README.md) — train a policy
- [legacy/dev-inf README](legacy/dev-inf/README.md) — archived docker sim2sim stack
