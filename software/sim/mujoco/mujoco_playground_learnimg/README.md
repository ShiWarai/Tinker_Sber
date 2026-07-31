# MuJoCo Playground (BD only)

Trimmed fork of [google-deepmind/mujoco_playground](https://github.com/google-deepmind/mujoco_playground)
with a single locomotion environment: **BDJoystickFlatTerrain**.

## Install

```bash
pip install -e ".[cuda]"   # or: pip install -e .
```

## Train / play

```bash
train-jax-ppo --env_name BDJoystickFlatTerrain --impl jax --use_tb
```

See also `learning/train_jax_ppo.py` and the `playground_bd/` helpers in the parent monorepo if present.

## Layout

- `mujoco_playground/_src/locomotion/bd/` — BD env, XML, meshes
- `mujoco_playground/config/locomotion_params.py` — Brax / RSL-RL PPO configs
- `learning/` — training entrypoints

Menagerie / manipulation / dm_control suite are not included.
