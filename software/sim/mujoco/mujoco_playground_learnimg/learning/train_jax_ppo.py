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
"""Train a PPO agent using JAX on the specified environment."""

import datetime
import functools
import json
import os
import time
import warnings

from absl import app
from absl import flags
from absl import logging
from etils import epath
import jax
import jax.numpy as jp
import mediapy as media
from ml_collections import config_dict
import mujoco

# Brax 0.14.x still calls jax.device_put_replicated; JAX>=0.11 removed it.
# Old API laid out replicas with a leading device axis so brax `_unpmap`
# can `squeeze(0)` on `addressable_shards[0].data`. Plain NamedSharding(P())
# does not add that axis and breaks training.
if not hasattr(jax, "device_put_replicated"):

  def device_put_replicated(x, devices):
    devices = tuple(devices)
    mesh = jax.sharding.Mesh(devices, axis_names=("i",))
    sharding = jax.sharding.NamedSharding(
        mesh, jax.sharding.PartitionSpec("i")
    )

    def _put(leaf):
      leaf = jax.numpy.asarray(leaf)
      # (n_devices, *leaf.shape) — one full copy per device along axis 0
      stacked = jax.numpy.stack([leaf] * len(devices), axis=0)
      return jax.device_put(stacked, sharding)

    return jax.tree_util.tree_map(_put, x)

  jax.device_put_replicated = device_put_replicated  # type: ignore[attr-defined]

from brax.training.agents.ppo import networks as ppo_networks
from brax.training.agents.ppo import networks_vision as ppo_networks_vision
from brax.training.agents.ppo import train as ppo

from mujoco_playground import registry
from mujoco_playground import wrapper
import mujoco_playground
from mujoco_playground.config import locomotion_params

try:
  import tensorboardX
except ImportError:
  tensorboardX = None

try:
  import wandb
except ImportError:
  wandb = None


xla_flags = os.environ.get("XLA_FLAGS", "")
xla_flags += " --xla_gpu_triton_gemm_any=True"
os.environ["XLA_FLAGS"] = xla_flags
os.environ["XLA_PYTHON_CLIENT_PREALLOCATE"] = "false"
os.environ["MUJOCO_GL"] = "egl"

# Ignore the info logs from brax
logging.set_verbosity(logging.WARNING)


def _format_duration(seconds: float) -> str:
  """Человекочитаемая длительность на русском."""
  if seconds < 0:
    seconds = 0.0
  total = int(round(seconds))
  hours, rem = divmod(total, 3600)
  minutes, secs = divmod(rem, 60)
  if hours:
    return f"{hours} ч {minutes} мин {secs} с"
  if minutes:
    return f"{minutes} мин {secs} с"
  return f"{secs} с"


# Suppress warnings

# Suppress RuntimeWarnings from JAX
warnings.filterwarnings("ignore", category=RuntimeWarning, module="jax")
# Suppress DeprecationWarnings from JAX
warnings.filterwarnings("ignore", category=DeprecationWarning, module="jax")
# Suppress UserWarnings from absl (used by JAX and TensorFlow)
warnings.filterwarnings("ignore", category=UserWarning, module="absl")


_ENV_NAME = flags.DEFINE_string(
    "env_name",
    "LeapCubeReorient",
    f"Name of the environment. One of {', '.join(registry.ALL_ENVS)}",
)
_IMPL = flags.DEFINE_enum("impl", "jax", ["jax", "warp"], "MJX implementation")
_PLAYGROUND_CONFIG_OVERRIDES = flags.DEFINE_string(
    "playground_config_overrides",
    None,
    "Overrides for the playground env config.",
)
_VISION = flags.DEFINE_boolean("vision", False, "Use vision input")
_LOAD_CHECKPOINT_PATH = flags.DEFINE_string(
    "load_checkpoint_path", None, "Path to load checkpoint from"
)
_SUFFIX = flags.DEFINE_string("suffix", None, "Suffix for the experiment name")
_PLAY_ONLY = flags.DEFINE_boolean(
    "play_only", False, "If true, only play with the model and do not train"
)
_USE_WANDB = flags.DEFINE_boolean(
    "use_wandb",
    False,
    "Use Weights & Biases for logging (ignored in play-only mode)",
)
_USE_TB = flags.DEFINE_boolean(
    "use_tb", False, "Use TensorBoard for logging (ignored in play-only mode)"
)
_DOMAIN_RANDOMIZATION = flags.DEFINE_boolean(
    "domain_randomization", False, "Use domain randomization"
)
_SEED = flags.DEFINE_integer("seed", 1, "Random seed")
_NUM_TIMESTEPS = flags.DEFINE_integer(
    "num_timesteps", 1_000_000, "Number of timesteps"
)
_NUM_VIDEOS = flags.DEFINE_integer(
    "num_videos", 1, "Number of videos to record after training."
)
_NUM_EVALS = flags.DEFINE_integer("num_evals", 5, "Number of evaluations")
_REWARD_SCALING = flags.DEFINE_float("reward_scaling", 0.1, "Reward scaling")
_EPISODE_LENGTH = flags.DEFINE_integer("episode_length", 1000, "Episode length")
_NORMALIZE_OBSERVATIONS = flags.DEFINE_boolean(
    "normalize_observations", True, "Normalize observations"
)
_ACTION_REPEAT = flags.DEFINE_integer("action_repeat", 1, "Action repeat")
_UNROLL_LENGTH = flags.DEFINE_integer("unroll_length", 10, "Unroll length")
_NUM_MINIBATCHES = flags.DEFINE_integer(
    "num_minibatches", 8, "Number of minibatches"
)
_NUM_UPDATES_PER_BATCH = flags.DEFINE_integer(
    "num_updates_per_batch", 8, "Number of updates per batch"
)
_DISCOUNTING = flags.DEFINE_float("discounting", 0.97, "Discounting")
_LEARNING_RATE = flags.DEFINE_float("learning_rate", 5e-4, "Learning rate")
_ENTROPY_COST = flags.DEFINE_float("entropy_cost", 5e-3, "Entropy cost")
_NUM_ENVS = flags.DEFINE_integer("num_envs", 1024, "Number of environments")
_NUM_EVAL_ENVS = flags.DEFINE_integer(
    "num_eval_envs", 128, "Number of evaluation environments"
)
_BATCH_SIZE = flags.DEFINE_integer("batch_size", 256, "Batch size")
_MAX_GRAD_NORM = flags.DEFINE_float("max_grad_norm", 1.0, "Max grad norm")
_CLIPPING_EPSILON = flags.DEFINE_float(
    "clipping_epsilon", 0.3, "Clipping epsilon for PPO"
)
_POLICY_HIDDEN_LAYER_SIZES = flags.DEFINE_list(
    "policy_hidden_layer_sizes",
    [64, 64, 64],
    "Policy hidden layer sizes",
)
_VALUE_HIDDEN_LAYER_SIZES = flags.DEFINE_list(
    "value_hidden_layer_sizes",
    [64, 64, 64],
    "Value hidden layer sizes",
)
_POLICY_OBS_KEY = flags.DEFINE_string(
    "policy_obs_key", "state", "Policy obs key"
)
_VALUE_OBS_KEY = flags.DEFINE_string("value_obs_key", "state", "Value obs key")
_RSCOPE_ENVS = flags.DEFINE_integer(
    "rscope_envs",
    None,
    "Number of parallel environment rollouts to save for the rscope viewer",
)
_DETERMINISTIC_RSCOPE = flags.DEFINE_boolean(
    "deterministic_rscope",
    True,
    "Run deterministic rollouts for the rscope viewer",
)
_RUN_EVALS = flags.DEFINE_boolean(
    "run_evals",
    True,
    "Run evaluation rollouts between policy updates.",
)
_LOG_TRAINING_METRICS = flags.DEFINE_boolean(
    "log_training_metrics",
    False,
    "Whether to log training metrics and callback to progress_fn. Significantly"
    " slows down training if too frequent.",
)
_TRAINING_METRICS_STEPS = flags.DEFINE_integer(
    "training_metrics_steps",
    1_000_000,
    "Number of steps between logging training metrics. Increase if training"
    " experiences slowdown.",
)
_WARP_KERNEL_CACHE_DIR = flags.DEFINE_string(
    "warp_kernel_cache_dir",
    None,
    "Directory for caching compiled Warp kernels.",
)
_LOGDIR = flags.DEFINE_string("logdir", None, "Directory for logging.")


def get_rl_config(env_name: str) -> config_dict.ConfigDict:
  if env_name in mujoco_playground.locomotion._envs:
    return locomotion_params.brax_ppo_config(env_name, _IMPL.value)

  raise ValueError(f"Окружение {env_name} не найдено в {registry.ALL_ENVS}.")


def rscope_fn(full_states, obs, rew, done):
  """
  All arrays are of shape (unroll_length, rscope_envs, ...)
  full_states: dict with keys 'qpos', 'qvel', 'time', 'metrics'
  obs: nd.array or dict obs based on env configuration
  rew: nd.array rewards
  done: nd.array done flags
  """
  # Calculate cumulative rewards per episode, stopping at first done flag
  done_mask = jp.cumsum(done, axis=0)
  valid_rewards = rew * (done_mask == 0)
  episode_rewards = jp.sum(valid_rewards, axis=0)
  print(
      "Собраны rscope-роллауты, награда"
      f" {episode_rewards.mean():.3f} +- {episode_rewards.std():.3f}"
  )


def main(argv):
  """Run training and evaluation for the specified environment."""

  del argv

  run_t0 = time.monotonic()

  if _WARP_KERNEL_CACHE_DIR.value is not None:
    import warp as wp  # pylint: disable=g-import-not-at-top

    wp.config.kernel_cache_dir = _WARP_KERNEL_CACHE_DIR.value

  # Load environment configuration
  env_cfg = registry.get_default_config(_ENV_NAME.value)

  ppo_params = get_rl_config(_ENV_NAME.value)

  if _NUM_TIMESTEPS.present:
    ppo_params.num_timesteps = _NUM_TIMESTEPS.value
  if _PLAY_ONLY.present:
    ppo_params.num_timesteps = 0
  if _NUM_EVALS.present:
    ppo_params.num_evals = _NUM_EVALS.value
  if _REWARD_SCALING.present:
    ppo_params.reward_scaling = _REWARD_SCALING.value
  if _EPISODE_LENGTH.present:
    ppo_params.episode_length = _EPISODE_LENGTH.value
  if _NORMALIZE_OBSERVATIONS.present:
    ppo_params.normalize_observations = _NORMALIZE_OBSERVATIONS.value
  if _ACTION_REPEAT.present:
    ppo_params.action_repeat = _ACTION_REPEAT.value
  if _UNROLL_LENGTH.present:
    ppo_params.unroll_length = _UNROLL_LENGTH.value
  if _NUM_MINIBATCHES.present:
    ppo_params.num_minibatches = _NUM_MINIBATCHES.value
  if _NUM_UPDATES_PER_BATCH.present:
    ppo_params.num_updates_per_batch = _NUM_UPDATES_PER_BATCH.value
  if _DISCOUNTING.present:
    ppo_params.discounting = _DISCOUNTING.value
  if _LEARNING_RATE.present:
    ppo_params.learning_rate = _LEARNING_RATE.value
  if _ENTROPY_COST.present:
    ppo_params.entropy_cost = _ENTROPY_COST.value
  if _NUM_ENVS.present:
    ppo_params.num_envs = _NUM_ENVS.value
  if _NUM_EVAL_ENVS.present:
    ppo_params.num_eval_envs = _NUM_EVAL_ENVS.value
  if _BATCH_SIZE.present:
    ppo_params.batch_size = _BATCH_SIZE.value
  if _MAX_GRAD_NORM.present:
    ppo_params.max_grad_norm = _MAX_GRAD_NORM.value
  if _CLIPPING_EPSILON.present:
    ppo_params.clipping_epsilon = _CLIPPING_EPSILON.value
  if _POLICY_HIDDEN_LAYER_SIZES.present:
    ppo_params.network_factory.policy_hidden_layer_sizes = list(
        map(int, _POLICY_HIDDEN_LAYER_SIZES.value)
    )
  if _VALUE_HIDDEN_LAYER_SIZES.present:
    ppo_params.network_factory.value_hidden_layer_sizes = list(
        map(int, _VALUE_HIDDEN_LAYER_SIZES.value)
    )
  if _POLICY_OBS_KEY.present:
    ppo_params.network_factory.policy_obs_key = _POLICY_OBS_KEY.value
  if _VALUE_OBS_KEY.present:
    ppo_params.network_factory.value_obs_key = _VALUE_OBS_KEY.value

  env_cfg_overrides = {"impl": _IMPL.value}
  if _VISION.value:
    env_cfg_overrides["vision"] = True
    env_cfg_overrides["vision_config.nworld"] = ppo_params.num_envs
  if _PLAYGROUND_CONFIG_OVERRIDES.value is not None:
    env_cfg_overrides.update(json.loads(_PLAYGROUND_CONFIG_OVERRIDES.value))

  env = registry.load(
      _ENV_NAME.value, config=env_cfg, config_overrides=env_cfg_overrides
  )
  if _RUN_EVALS.present:
    ppo_params.run_evals = _RUN_EVALS.value
  if _LOG_TRAINING_METRICS.present:
    ppo_params.log_training_metrics = _LOG_TRAINING_METRICS.value
  if _TRAINING_METRICS_STEPS.present:
    ppo_params.training_metrics_steps = _TRAINING_METRICS_STEPS.value

  print(f"Конфиг окружения:\n{env_cfg}")
  if env_cfg_overrides:
    print(f"Переопределения конфига окружения:\n{env_cfg_overrides}\n")
  print(f"Параметры обучения PPO:\n{ppo_params}")

  # Generate unique experiment name
  now = datetime.datetime.now()
  timestamp = now.strftime("%Y%m%d-%H%M%S")
  exp_name = f"{_ENV_NAME.value}-{timestamp}"
  if _SUFFIX.value is not None:
    exp_name += f"-{_SUFFIX.value}"
  print(f"Имя эксперимента: {exp_name}")

  # Set up logging directory
  logdir = epath.Path(_LOGDIR.value or "logs").resolve() / exp_name
  logdir.mkdir(parents=True, exist_ok=True)
  print(f"Логи сохраняются в: {logdir}")

  # Initialize Weights & Biases if required
  if _USE_WANDB.value and not _PLAY_ONLY.value:
    if wandb is None:
      raise ImportError(
          "Для --use_wandb нужен wandb. Установите: pip install wandb"
      )
    wandb.init(project="mjxrl", name=exp_name)
    wandb.config.update(env_cfg.to_dict())
    wandb.config.update({"env_name": _ENV_NAME.value})

  # Initialize TensorBoard if required
  writer = None
  if _USE_TB.value and not _PLAY_ONLY.value and tensorboardX is not None:
    writer = tensorboardX.SummaryWriter(logdir)

  # Handle checkpoint loading
  if _LOAD_CHECKPOINT_PATH.value is not None:
    # Convert to absolute path
    ckpt_path = epath.Path(_LOAD_CHECKPOINT_PATH.value).resolve()
    if ckpt_path.is_dir():
      latest_ckpts = list(ckpt_path.glob("*"))
      latest_ckpts = [ckpt for ckpt in latest_ckpts if ckpt.is_dir()]
      latest_ckpts.sort(key=lambda x: int(x.name))
      latest_ckpt = latest_ckpts[-1]
      restore_checkpoint_path = latest_ckpt
      print(f"Восстановление из: {restore_checkpoint_path}")
    else:
      restore_checkpoint_path = ckpt_path
      print(f"Восстановление из чекпоинта: {restore_checkpoint_path}")
  else:
    print("Путь к чекпоинту не задан, восстановление пропущено")
    restore_checkpoint_path = None

  # Set up checkpoint directory
  ckpt_path = logdir / "checkpoints"
  ckpt_path.mkdir(parents=True, exist_ok=True)
  print(f"Папка чекпоинтов: {ckpt_path}")

  # Save environment configuration
  with open(ckpt_path / "config.json", "w", encoding="utf-8") as fp:
    json.dump(env_cfg.to_dict(), fp, indent=4)

  reward_csv_path = logdir / "reward_history.csv"
  reward_png_path = logdir / "reward_curve.png"
  reward_components_csv_path = logdir / "reward_components.csv"
  reward_bonuses_png_path = logdir / "reward_bonuses.png"
  reward_penalties_png_path = logdir / "reward_penalties.png"

  # Split reward terms by config scale sign (skip zeros).
  reward_scales = {}
  if hasattr(env_cfg, "reward_config") and hasattr(env_cfg.reward_config, "scales"):
    reward_scales = dict(env_cfg.reward_config.scales)
  bonus_keys = sorted(k for k, s in reward_scales.items() if float(s) > 0)
  penalty_keys = sorted(k for k, s in reward_scales.items() if float(s) < 0)
  component_keys = bonus_keys + penalty_keys

  if not _PLAY_ONLY.value:
    with open(reward_csv_path, "w", encoding="utf-8") as fp:
      fp.write("steps,eval_reward\n")
    if component_keys:
      with open(reward_components_csv_path, "w", encoding="utf-8") as fp:
        fp.write("steps," + ",".join(component_keys) + "\n")

  def _update_reward_curve():
    """Rewrite a single PNG covering all eval rewards so far."""
    try:
      import matplotlib

      matplotlib.use("Agg")
      import matplotlib.pyplot as plt
    except ImportError:
      print("matplotlib не установлен; пропуск reward_curve.png")
      return
    steps, rewards = [], []
    with open(reward_csv_path, encoding="utf-8") as fp:
      next(fp, None)  # header
      for line in fp:
        line = line.strip()
        if not line:
          continue
        s, r = line.split(",")
        steps.append(int(float(s)))
        rewards.append(float(r))
    if not steps:
      return
    fig, ax = plt.subplots(figsize=(8, 4))
    ax.plot(steps, rewards, marker="o", linewidth=2)
    ax.set_xlabel("env steps")
    ax.set_ylabel("eval episode reward")
    ax.set_title(f"{exp_name} reward")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(reward_png_path, dpi=120)
    plt.close(fig)

  def _update_reward_component_curves():
    """Two plots: bonus terms (scale>0) and penalty terms (scale<0)."""
    if not component_keys or not reward_components_csv_path.exists():
      return
    try:
      import matplotlib

      matplotlib.use("Agg")
      import matplotlib.pyplot as plt
    except ImportError:
      print("matplotlib не установлен; пропуск графиков компонентов награды")
      return

    rows = []
    with open(reward_components_csv_path, encoding="utf-8") as fp:
      header = next(fp).strip().split(",")
      for line in fp:
        line = line.strip()
        if not line:
          continue
        parts = line.split(",")
        rows.append({header[i]: float(parts[i]) for i in range(len(header))})
    if not rows:
      return
    steps = [int(r["steps"]) for r in rows]

    def _plot(keys, path, title, ylabel):
      if not keys:
        return
      fig, ax = plt.subplots(figsize=(10, 5))
      for key in keys:
        if key not in rows[0]:
          continue
        ax.plot(
            steps,
            [r[key] for r in rows],
            marker="o",
            linewidth=2,
            label=key,
        )
      ax.set_xlabel("env steps")
      ax.set_ylabel(ylabel)
      ax.set_title(title)
      ax.grid(True, alpha=0.3)
      ax.legend(loc="best", fontsize=8, ncol=2)
      fig.tight_layout()
      fig.savefig(path, dpi=120)
      plt.close(fig)

    _plot(
        bonus_keys,
        reward_bonuses_png_path,
        f"{exp_name} rewards (bonuses)",
        "episode sum of scaled reward terms",
    )
    _plot(
        penalty_keys,
        reward_penalties_png_path,
        f"{exp_name} penalties",
        "episode sum of scaled penalty terms",
    )

  training_params = dict(ppo_params)
  if "network_factory" in training_params:
    del training_params["network_factory"]

  network_fn = (
      ppo_networks_vision.make_ppo_networks_vision
      if _VISION.value
      else ppo_networks.make_ppo_networks
  )
  if hasattr(ppo_params, "network_factory"):
    network_factory = functools.partial(
        network_fn, **ppo_params.network_factory
    )
  else:
    network_factory = network_fn

  if _DOMAIN_RANDOMIZATION.value:
    training_params["randomization_fn"] = registry.get_domain_randomizer(
        _ENV_NAME.value
    )

  num_eval_envs = ppo_params.get("num_eval_envs", 128)

  if "num_eval_envs" in training_params:
    del training_params["num_eval_envs"]

  train_fn = functools.partial(
      ppo.train,
      **training_params,
      network_factory=network_factory,
      seed=_SEED.value,
      restore_checkpoint_path=restore_checkpoint_path,
      save_checkpoint_path=ckpt_path,
      wrap_env_fn=wrapper.wrap_for_brax_training,
      num_eval_envs=num_eval_envs,
      vision=_VISION.value,
  )

  times = [time.monotonic()]

  # Progress function for logging
  def progress(num_steps, metrics):
    times.append(time.monotonic())

    # Log to Weights & Biases
    if _USE_WANDB.value and not _PLAY_ONLY.value:
      wandb.log(metrics, step=num_steps)

    # Log to TensorBoard
    if _USE_TB.value and not _PLAY_ONLY.value and writer is not None:
      for key, value in metrics.items():
        writer.add_scalar(key, value, num_steps)
      writer.flush()
    if _RUN_EVALS.value and "eval/episode_reward" in metrics:
      reward = float(metrics["eval/episode_reward"])
      print(f"{num_steps}: награда={reward:.3f}")
      if not _PLAY_ONLY.value:
        with open(reward_csv_path, "a", encoding="utf-8") as fp:
          fp.write(f"{int(num_steps)},{reward}\n")
        _update_reward_curve()
        if component_keys:
          vals = []
          for key in component_keys:
            metric_key = f"eval/episode_reward/{key}"
            vals.append(str(float(metrics.get(metric_key, 0.0))))
          with open(reward_components_csv_path, "a", encoding="utf-8") as fp:
            fp.write(f"{int(num_steps)}," + ",".join(vals) + "\n")
          _update_reward_component_curves()
    if _LOG_TRAINING_METRICS.value:
      if "episode/sum_reward" in metrics:
        print(
            f"{num_steps}: средняя награда за эпизод"
            f"={metrics['episode/sum_reward']:.3f}"
        )

  eval_env_overrides = dict(env_cfg_overrides)
  if _VISION.value:
    eval_env_overrides["vision_config.nworld"] = num_eval_envs
  eval_env = registry.load(
      _ENV_NAME.value,
      config=registry.get_default_config(_ENV_NAME.value),
      config_overrides=eval_env_overrides,
  )

  policy_params_fn = lambda *args: None
  if _RSCOPE_ENVS.value:
    # Interactive visualisation of policy checkpoints
    from rscope import brax as rscope_utils

    if not _VISION.value:
      rscope_env = registry.load(
          _ENV_NAME.value, config=env_cfg, config_overrides=env_cfg_overrides
      )
      rscope_env = wrapper.wrap_for_brax_training(
          rscope_env,
          episode_length=ppo_params.episode_length,
          action_repeat=ppo_params.action_repeat,
          randomization_fn=training_params.get("randomization_fn"),
      )
    else:
      rscope_env = env

    rscope_handle = rscope_utils.BraxRolloutSaver(
        rscope_env,
        ppo_params,
        _VISION.value,
        _RSCOPE_ENVS.value,
        _DETERMINISTIC_RSCOPE.value,
        jax.random.PRNGKey(_SEED.value),
        rscope_fn,
    )

    def policy_params_fn(current_step, make_policy, params):  # pylint: disable=unused-argument
      rscope_handle.set_make_policy(make_policy)
      # rscope_handle.dump_rollout(params) # Disabled to prevent rendering slice crash

  # Train or load the model
  make_inference_fn, params, _ = train_fn(  # pylint: disable=no-value-for-parameter
      environment=env,
      progress_fn=progress,
      policy_params_fn=policy_params_fn,
      eval_env=eval_env,
  )

  jit_compile_s = 0.0
  train_s = 0.0
  print("Обучение завершено.")
  if len(times) > 1:
    jit_compile_s = times[1] - times[0]
    train_s = times[-1] - times[1]
    print(f"Время JIT-компиляции: {_format_duration(jit_compile_s)} ({jit_compile_s:.1f} с)")
    print(f"Время обучения: {_format_duration(train_s)} ({train_s:.1f} с)")

  print("Запуск инференса и записи видео...")
  inference_t0 = time.monotonic()

  # Create inference function.
  inference_fn = make_inference_fn(params, deterministic=True)
  jit_inference_fn = jax.jit(inference_fn)

  infer_env_overrides = dict(env_cfg_overrides)
  if _VISION.value:
    infer_env_overrides["vision_config.nworld"] = _NUM_VIDEOS.value
  # Demo video: no random pushes so the gait is easier to read.
  if _ENV_NAME.value == "BDJoystickFlatTerrain":
    infer_env_overrides["push_config.enable"] = False
  infer_env = registry.load(
      _ENV_NAME.value,
      config=registry.get_default_config(_ENV_NAME.value),
      config_overrides=infer_env_overrides,
  )

  # BD demo: 5 s each of forward / backward / side / yaw (ctrl_dt=0.01 → 500).
  # Other envs keep training episode_length and natural random cmds.
  bd_demo_seg_steps = 500
  bd_demo_cmds = None
  video_length = int(ppo_params.episode_length)
  if _ENV_NAME.value == "BDJoystickFlatTerrain":
    bd_demo_cmds = jp.array(
        [
            [0.30, 0.00, 0.00],   # forward
            [-0.30, 0.00, 0.00],  # backward
            [0.00, 0.20, 0.00],   # side left
            [0.00, 0.00, 0.60],   # yaw left
        ],
        dtype=jp.float32,
    )
    video_length = bd_demo_seg_steps * int(bd_demo_cmds.shape[0])
    print(
        "Режим видео BD: 5 с вперёд / 5 с назад / 5 с вбок / 5 с поворот "
        f"(всего {video_length * float(infer_env.dt):.0f} с)"
    )

  # Run evaluation rollouts matching how training handles batched environments.
  # For BD, episode_length covers the full cmd sequence so auto-reset does not cut it.
  wrapped_infer_env = wrapper.wrap_for_brax_training(
      infer_env,
      episode_length=video_length,
      action_repeat=ppo_params.get("action_repeat", 1),
  )

  rng = jax.random.split(jax.random.PRNGKey(_SEED.value), _NUM_VIDEOS.value)
  reset_states = jax.jit(wrapped_infer_env.reset)(rng)

  def _force_cmd(state, cmd):
    info = {
        **state.info,
        "command": cmd,
        "step": jp.zeros(_NUM_VIDEOS.value, dtype=jp.int32),
    }
    obs = state.obs
    if isinstance(obs, dict) and "state" in obs:
      st = obs["state"].at[..., 9:12].set(cmd)
      new_obs = dict(obs)
      new_obs["state"] = st
      if "privileged_state" in obs:
        new_obs["privileged_state"] = obs["privileged_state"].at[..., 9:12].set(
            cmd
        )
      obs = new_obs
    return state.replace(info=info, obs=obs)

  def _cmd_at_step(t):
    seg = jp.minimum(t // bd_demo_seg_steps, bd_demo_cmds.shape[0] - 1)
    return jp.broadcast_to(bd_demo_cmds[seg], (_NUM_VIDEOS.value, 3))

  if bd_demo_cmds is not None:
    reset_states = _force_cmd(reset_states, _cmd_at_step(0))

  empty_data = reset_states.data.__class__(
      **{k: None for k in reset_states.data.__annotations__}
  )  # pytype: disable=attribute-error
  empty_traj = reset_states.__class__(
      **{k: None for k in reset_states.__annotations__}
  )  # pytype: disable=attribute-error
  empty_traj = empty_traj.replace(data=empty_data)

  def step(carry, t):
    state, rng = carry
    if bd_demo_cmds is not None:
      state = _force_cmd(state, _cmd_at_step(t))
    rng, act_key = jax.random.split(rng)
    act_keys = jax.random.split(act_key, _NUM_VIDEOS.value)
    act = jax.vmap(jit_inference_fn)(state.obs, act_keys)[0]
    state = wrapped_infer_env.step(state, act)
    traj_data = empty_traj.tree_replace({
        "data.qpos": state.data.qpos,
        "data.qvel": state.data.qvel,
        "data.time": state.data.time,
        "data.ctrl": state.data.ctrl,
        "data.mocap_pos": state.data.mocap_pos,
        "data.mocap_quat": state.data.mocap_quat,
        "data.xfrc_applied": state.data.xfrc_applied,
    })
    return (state, rng), traj_data

  @jax.jit
  def do_rollout(state, rng):
    _, traj = jax.lax.scan(
        step, (state, rng), jp.arange(video_length), length=video_length
    )
    return traj

  traj_stacked = do_rollout(reset_states, jax.random.PRNGKey(_SEED.value + 1))
  # traj_stacked has shape (time, nworld, ...), swap to (nworld, time, ...).
  traj_stacked = jax.tree.map(lambda x: jp.moveaxis(x, 0, 1), traj_stacked)
  trajectories = [None] * _NUM_VIDEOS.value
  for i in range(_NUM_VIDEOS.value):
    t = jax.tree.map(lambda x, i=i: x[i], traj_stacked)
    trajectories[i] = [
        jax.tree.map(lambda x, j=j: x[j], t)
        for j in range(video_length)
    ]

  # Render and save the rollout.
  render_every = 2
  fps = 1.0 / infer_env.dt / render_every
  print(f"FPS рендера: {fps:.1f}")
  scene_option = mujoco.MjvOption()
  scene_option.flags[mujoco.mjtVisFlag.mjVIS_TRANSPARENT] = False
  scene_option.flags[mujoco.mjtVisFlag.mjVIS_PERTFORCE] = False
  scene_option.flags[mujoco.mjtVisFlag.mjVIS_CONTACTFORCE] = False
  # Prefer named tracking camera (BD/G1 put camera "track" on the torso).
  render_camera = None
  if (
      mujoco.mj_name2id(
          infer_env.mj_model, mujoco.mjtObj.mjOBJ_CAMERA, "track"
      )
      >= 0
  ):
    render_camera = "track"

  for i, rollout in enumerate(trajectories):
    traj = rollout[::render_every]
    frames = infer_env.render(
        traj,
        height=480,
        width=640,
        camera=render_camera,
        scene_option=scene_option,
    )
    media.write_video(logdir / f"rollout{i}.mp4", frames, fps=fps)
    print(f"Видео роллаута сохранено: '{logdir}/rollout{i}.mp4'")

  inference_s = time.monotonic() - inference_t0
  total_s = time.monotonic() - run_t0
  timing_path = logdir / "training_timing.txt"
  timing_lines = [
      f"Эксперимент: {exp_name}",
      f"JIT-компиляция: {_format_duration(jit_compile_s)} ({jit_compile_s:.1f} с)",
      f"Обучение: {_format_duration(train_s)} ({train_s:.1f} с)",
      f"Инференс и видео: {_format_duration(inference_s)} ({inference_s:.1f} с)",
      f"Итого: {_format_duration(total_s)} ({total_s:.1f} с)",
  ]
  with open(timing_path, "w", encoding="utf-8") as fp:
    fp.write("\n".join(timing_lines) + "\n")

  print("--- Итоговое время ---")
  for line in timing_lines[1:]:
    print(line)
  print(f"Сводка сохранена в: {timing_path}")


def run():
  """Entry point for uv/pip script."""
  app.run(main)


if __name__ == "__main__":
  run()
