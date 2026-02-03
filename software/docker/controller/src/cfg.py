from __future__ import annotations

from dataclasses import MISSING
from typing import Literal
import configclass


@configclass
class RslRlBaseRunnerCfg:
    """Base configuration of the runner."""

    seed: int = 42
    """The seed for the experiment. Default is 42."""

    device: str = "cuda:0"
    """The device for the rl-agent. Default is cuda:0."""

    num_steps_per_env: int = MISSING
    """The number of steps per environment per update."""

    max_iterations: int = MISSING
    """The maximum number of iterations."""

    empirical_normalization: bool | None = None
    """This parameter is deprecated and will be removed in the future.

    Use `actor_obs_normalization` and `critic_obs_normalization` instead.
    """

    obs_groups: dict[str, list[str]] = MISSING
    """A mapping from observation groups to observation sets.

    The keys of the dictionary are predefined observation sets used by the underlying algorithm
    and values are lists of observation groups provided by the environment.

    For instance, if the environment provides a dictionary of observations with groups "policy", "images",
    and "privileged", these can be mapped to algorithmic observation sets as follows:

    .. code-block:: python

        obs_groups = {
            "policy": ["policy", "images"],
            "critic": ["policy", "privileged"],
        }

    This way, the policy will receive the "policy" and "images" observations, and the critic will
    receive the "policy" and "privileged" observations.

    For more details, please check ``vec_env.py`` in the rsl_rl library.
    """

    clip_actions: float | None = None
    """The clipping value for actions. If None, then no clipping is done. Defaults to None.

    .. note::
        This clipping is performed inside the :class:`RslRlVecEnvWrapper` wrapper.
    """

    save_interval: int = MISSING
    """The number of iterations between saves."""

    experiment_name: str = MISSING
    """The experiment name."""

    run_name: str = ""
    """The run name. Default is empty string.

    The name of the run directory is typically the time-stamp at execution. If the run name is not empty,
    then it is appended to the run directory's name, i.e. the logging directory's name will become
    ``{time-stamp}_{run_name}``.
    """

    logger: Literal["tensorboard", "neptune", "wandb"] = "tensorboard"
    """The logger to use. Default is tensorboard."""

    neptune_project: str = "isaaclab"
    """The neptune project name. Default is "isaaclab"."""

    wandb_project: str = "isaaclab"
    """The wandb project name. Default is "isaaclab"."""

    resume: bool = False
    """Whether to resume a previous training. Default is False.

    This flag will be ignored for distillation.
    """

    load_run: str = ".*"
    """The run directory to load. Default is ".*" (all).

    If regex expression, the latest (alphabetical order) matching run will be loaded.
    """

    load_checkpoint: str = "model_.*.pt"
    """The checkpoint file to load. Default is ``"model_.*.pt"`` (all).

    If regex expression, the latest (alphabetical order) matching file will be loaded.
    """


@configclass
class RslRlOnPolicyRunnerCfg(RslRlBaseRunnerCfg):
    """Configuration of the runner for on-policy algorithms."""

    class_name: str = "OnPolicyRunner"
    """The runner class name. Default is OnPolicyRunner."""

    # policy: RslRlPpoActorCriticCfg = MISSING
    policy = MISSING
    """The policy configuration."""

    # algorithm: RslRlPpoAlgorithmCfg = MISSING
    algorithm = MISSING
    """The algorithm configuration."""


@configclass
class RslRlOnPolicyRunnerMlpCfg(RslRlOnPolicyRunnerCfg):
    """Configuration of the runner for on-policy algorithms."""

    runner_type: str = "OnPolicyRunner"
