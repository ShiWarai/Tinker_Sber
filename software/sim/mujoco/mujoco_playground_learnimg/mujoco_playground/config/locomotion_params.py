# Copyright 2025 DeepMind Technologies Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""RL config for BD locomotion envs."""

from typing import Optional

from ml_collections import config_dict

from mujoco_playground._src import locomotion


def brax_ppo_config(
    env_name: str, impl: Optional[str] = None
) -> config_dict.ConfigDict:
  """Returns tuned Brax PPO config for the given environment."""
  del impl  # unused
  env_config = locomotion.get_default_config(env_name)

  rl_config = config_dict.create(
      num_timesteps=100_000_000,
      num_evals=10,
      reward_scaling=1.0,
      episode_length=env_config.episode_length,
      normalize_observations=True,
      action_repeat=1,
      unroll_length=20,
      num_minibatches=32,
      num_updates_per_batch=4,
      discounting=0.97,
      learning_rate=3e-4,
      entropy_cost=1e-2,
      num_envs=8192 * 2,
      batch_size=512,
      max_grad_norm=1.0,
      network_factory=config_dict.create(
          policy_hidden_layer_sizes=(128, 128, 128, 128),
          value_hidden_layer_sizes=(256, 256, 256, 256, 256),
          policy_obs_key="state",
          value_obs_key="state",
      ),
      num_resets_per_eval=10,
  )

  if env_name == "BDJoystickFlatTerrain":
    rl_config.num_timesteps = 80_000_000
    rl_config.num_evals = 16  # eval + checkpoint every ~5M
    rl_config.clipping_epsilon = 0.2
    rl_config.num_resets_per_eval = 1
    rl_config.entropy_cost = 0.005
    rl_config.network_factory = config_dict.create(
        policy_hidden_layer_sizes=(512, 256, 128),
        value_hidden_layer_sizes=(512, 256, 128),
        policy_obs_key="state",
        value_obs_key="privileged_state",
    )
  else:
    raise ValueError(f"Unsupported env: {env_name}")

  return rl_config


def rsl_rl_config(
    env_name: str, unused_impl: Optional[str] = None
) -> config_dict.ConfigDict:
  """Returns tuned RSL-RL PPO config for the given environment."""
  del unused_impl  # unused

  rl_config = config_dict.create(
      seed=1,
      runner_class_name="OnPolicyRunner",
      policy=config_dict.create(
          init_noise_std=1.0,
          actor_hidden_dims=[512, 256, 128],
          critic_hidden_dims=[512, 256, 128],
          activation="elu",
          class_name="ActorCritic",
      ),
      algorithm=config_dict.create(
          class_name="PPO",
          value_loss_coef=1.0,
          use_clipped_value_loss=True,
          clip_param=0.2,
          entropy_coef=0.001,
          num_learning_epochs=5,
          num_mini_batches=4,
          learning_rate=3.0e-4,
          schedule="fixed",
          gamma=0.99,
          lam=0.95,
          desired_kl=0.01,
          max_grad_norm=1.0,
      ),
      num_steps_per_env=24,
      max_iterations=100000,
      empirical_normalization=True,
      save_interval=50,
      experiment_name="test",
      run_name="",
      resume=False,
      load_run="-1",
      checkpoint=-1,
      resume_path=None,
  )

  if env_name != "BDJoystickFlatTerrain":
    raise ValueError(f"Unsupported env: {env_name}")

  return rl_config
