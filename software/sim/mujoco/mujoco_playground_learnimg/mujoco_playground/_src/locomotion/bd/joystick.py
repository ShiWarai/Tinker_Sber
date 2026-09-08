"""Joystick task for BD biped (Isaac squat60 parity in env params)."""

from typing import Any, Dict, Optional, Union

import jax
import jax.numpy as jp
from ml_collections import config_dict
from mujoco import mjx
from mujoco.mjx._src import math
import numpy as np

from mujoco_playground._src import gait
from mujoco_playground._src import mjx_env
from mujoco_playground._src.locomotion.bd import base as bd_base
from mujoco_playground._src.locomotion.bd import bd_constants as consts

NUM_JOINTS = 10


def default_config() -> config_dict.ConfigDict:
  return config_dict.create(
      # Isaac: 100 Hz control / 1 kHz physics.
      ctrl_dt=0.01,
      sim_dt=0.001,
      episode_length=1000,
      action_repeat=1,
      action_scale=0.5,
      history_len=1,
      soft_joint_pos_limit_factor=0.9,
      noise_config=config_dict.create(
          level=1.0,
          scales=config_dict.create(
              joint_pos=0.03,
              joint_vel=1.5,
              gravity=0.05,
              linvel=0.1,
              gyro=0.2,
          ),
          # Stochastic latency in ctrl steps (ctrl_dt=0.01 → 1 step = 10 ms).
          # Each step samples lag ~ U{0,...,delay}; 0 disables that channel.
          sensor_delay=3,  # max actor-obs lag; privileged_state stays current
          action_delay=2,  # max motor-command lag
      ),
      reward_config=config_dict.create(
          scales=config_dict.create(
              # Tracking — same style as T1/G1/Berkeley joystick.
              tracking_lin_vel=1.0,
              tracking_ang_vel=0.5,
              # Base. Squared-metre error is small at BD scale, hence -10.
              lin_vel_z=0.0,
              ang_vel_xy=-0.3,
              orientation=-3.0,
              base_height=-10.0,
              # Energy. action_rate damps the chattering swing leg.
              torques=0.0,
              action_rate=-0.15,
              energy=-0.0,
              # Feet — single-support credit, anti-hop, anti-tap, anti-parked-leg.
              feet_clearance=-1.0,
              feet_air_time=5.0,
              feet_slip=-0.25,
              feet_height=-0.5,
              feet_phase=1.0,
              feet_flight=-1.0,
              feet_rapid_step=-1.0,
              feet_stuck_swing=-2.0,
              feet_toe_only=-0.5,
              # Other — T1 alive; G1 stand_still at zero cmd.
              stand_still=-0.5,
              alive=0.25,
              termination=0.0,
              # Pose (G1-scale; T1 pose=-1.0 is too stiff for BD squat).
              joint_deviation_knee=-0.1,
              joint_deviation_hip=-0.3,
              dof_pos_limits=-1.0,
              pose=-0.1,
          ),
          tracking_sigma=0.25,
          # BD leg is ~0.29 m: 6 cm swing already clears the flat foot.
          max_foot_height=0.06,
          # Single-support time that saturates the credit. Below the 0.4-0.5 s
          # swing of gait_freq 1.0-1.25 Hz, so a parked leg cannot out-earn it.
          air_time_target=0.5,
          # Swing longer than this means a leg is parked in the air.
          max_swing_time= 0.75,
          # Settled height after PD hold on current home keyframe (flat ankles ±0.57).
          base_height_target=0.241,
      ),
      push_config=config_dict.create(
          enable=True,
          interval_range=[8.0, 15.0],
          magnitude_range=[0.02, 0.1],
      ),
      # Isaac command ranges.
      lin_vel_x=[-0.35, 0.35],
      lin_vel_y=[-0.25, 0.25],
      ang_vel_yaw=[-0.9, 0.9],
      impl="jax",
      naconmax=8 * 8192,
      njmax=40,
  )


class Joystick(bd_base.BDEnv):
  """Track a joystick command."""

  def __init__(
      self,
      task: str = "flat_terrain",
      config: config_dict.ConfigDict = default_config(),
      config_overrides: Optional[Dict[str, Union[str, int, list[Any]]]] = None,
  ):
    super().__init__(
        xml_path=consts.task_to_xml(task).as_posix(),
        config=config,
        config_overrides=config_overrides,
    )
    self._post_init()

  def _post_init(self) -> None:
    self._init_q = jp.array(self._mj_model.keyframe("home").qpos)
    self._default_pose = jp.array(self._mj_model.keyframe("home").qpos[7:])

    self._lowers, self._uppers = self.mj_model.jnt_range[1:].T
    c = (self._lowers + self._uppers) / 2
    r = self._uppers - self._lowers
    self._soft_lowers = c - 0.5 * r * self._config.soft_joint_pos_limit_factor
    self._soft_uppers = c + 0.5 * r * self._config.soft_joint_pos_limit_factor

    # Hip yaw/roll indices: L0,L1,R0,R1
    self._hip_indices = jp.array([0, 1, 5, 6])
    # Knees: L3, R3
    self._knee_indices = jp.array([3, 8])

    # Pose weights: downweight hip yaw.
    self._weights = jp.array([
        0.01, 1.0, 1.0, 1.0, 1.0,  # left
        0.01, 1.0, 1.0, 1.0, 1.0,  # right
    ])

    self._torso_body_id = self._mj_model.body(consts.ROOT_BODY).id
    self._torso_mass = self._mj_model.body_subtreemass[self._torso_body_id]
    self._site_id = self._mj_model.site("imu").id

    self._feet_site_id = np.array(
        [self._mj_model.site(name).id for name in consts.FEET_SITES]
    )
    self._floor_geom_id = self._mj_model.geom("floor").id
    self._feet_geom_id = np.array(
        [self._mj_model.geom(name).id for name in consts.FEET_GEOMS]
    )

    foot_linvel_sensor_adr = []
    for site in consts.FEET_SITES:
      sensor_id = self._mj_model.sensor(f"{site}_global_linvel").id
      sensor_adr = self._mj_model.sensor_adr[sensor_id]
      sensor_dim = self._mj_model.sensor_dim[sensor_id]
      foot_linvel_sensor_adr.append(
          list(range(sensor_adr, sensor_adr + sensor_dim))
      )
    self._foot_linvel_sensor_adr = jp.array(foot_linvel_sensor_adr)

    self._qpos_noise_scale = jp.full(
        NUM_JOINTS, self._config.noise_config.scales.joint_pos
    )

    # Latency buffers (Playground Franka-style). Length = max_delay + 1.
    self._sensor_delay = int(self._config.noise_config.sensor_delay)
    self._action_delay = int(self._config.noise_config.action_delay)
    self._obs_history_len = self._sensor_delay + 1
    self._action_history_len = self._action_delay + 1
    # Actor "state" layout in _get_obs (must stay in sync).
    self._actor_obs_size = (
        3 + 3 + 3 + 3 + NUM_JOINTS + NUM_JOINTS + NUM_JOINTS + 4
    )

    self._feet_floor_toe_sensor = [
        self._mj_model.sensor(f"{geom}_floor_found").id
        for geom in consts.FEET_TOE_GEOMS
    ]
    self._feet_floor_heel_sensor = [
        self._mj_model.sensor(f"{geom}_floor_found").id
        for geom in consts.FEET_HEEL_GEOMS
    ]

  def _toe_heel_contact(self, data: mjx.Data) -> tuple[jax.Array, jax.Array]:
    """Return (partial, full) contact masks per foot from toe/heel spheres."""
    toe = jp.array([
        data.sensordata[self._mj_model.sensor_adr[sensor_id]] > 0
        for sensor_id in self._feet_floor_toe_sensor
    ])
    heel = jp.array([
        data.sensordata[self._mj_model.sensor_adr[sensor_id]] > 0
        for sensor_id in self._feet_floor_heel_sensor
    ])
    partial = toe | heel
    full = toe & heel
    return partial, full

  def reset(self, rng: jax.Array) -> mjx_env.State:
    qpos = self._init_q
    qvel = jp.zeros(self.mjx_model.nv)

    rng, key = jax.random.split(rng)
    dxy = jax.random.uniform(key, (2,), minval=-0.1, maxval=0.1)
    qpos = qpos.at[0:2].set(qpos[0:2] + dxy)
    rng, key = jax.random.split(rng)
    yaw = jax.random.uniform(key, (1,), minval=-0.2, maxval=0.2)
    quat = math.axis_angle_to_quat(jp.array([0, 0, 1]), yaw)
    new_quat = math.quat_mul(qpos[3:7], quat)
    qpos = qpos.at[3:7].set(new_quat)

    # Mild joint jitter around squat (avoid *U(0.5,1.5) which breaks BD signs).
    rng, key = jax.random.split(rng)
    qpos = qpos.at[7:].set(
        qpos[7:]
        + jax.random.uniform(key, (NUM_JOINTS,), minval=-0.02, maxval=0.02)
    )

    rng, key = jax.random.split(rng)
    qvel = qvel.at[0:6].set(
        jax.random.uniform(key, (6,), minval=-0.1, maxval=0.1)
    )

    data = mjx_env.make_data(
        self.mj_model,
        qpos=qpos,
        qvel=qvel,
        ctrl=qpos[7:],
        impl=self.mjx_model.impl.value,
        naconmax=self._config.naconmax,
        njmax=self._config.njmax,
    )
    data = mjx.forward(self.mjx_model, data)

    rng, key = jax.random.split(rng)
    gait_freq = jax.random.uniform(key, (1,), minval=1.0, maxval=1.25)
    phase_dt = 2 * jp.pi * self.dt * gait_freq
    phase = jp.array([0, jp.pi])

    rng, cmd_rng = jax.random.split(rng)
    cmd = self.sample_command(cmd_rng)

    rng, push_rng = jax.random.split(rng)
    push_interval = jax.random.uniform(
        push_rng,
        minval=self._config.push_config.interval_range[0],
        maxval=self._config.push_config.interval_range[1],
    )
    push_interval_steps = jp.round(push_interval / self.dt).astype(jp.int32)

    info = {
        "rng": rng,
        "step": 0,
        "command": cmd,
        "last_act": jp.zeros(self.mjx_model.nu),
        "last_last_act": jp.zeros(self.mjx_model.nu),
        "motor_targets": jp.zeros(self.mjx_model.nu),
        "feet_air_time": jp.zeros(2),
        "feet_contact_time": jp.zeros(2),
        # T1 parity: gait timing keyed on any foot point touching the floor.
        "last_contact": jp.zeros(2, dtype=bool),
        "swing_peak": jp.zeros(2),
        "phase_dt": phase_dt,
        "phase": phase,
        "push": jp.array([0.0, 0.0]),
        "push_step": 0,
        "push_interval_steps": push_interval_steps,
        "action_history": jp.zeros(
            self._action_history_len * self.mjx_model.nu
        ),
        "obs_history": jp.zeros(
            self._obs_history_len * self._actor_obs_size
        ),
    }

    metrics = {}
    for k in self._config.reward_config.scales.keys():
      metrics[f"reward/{k}"] = jp.zeros(())
    metrics["swing_peak"] = jp.zeros(())

    partial, full = self._toe_heel_contact(data)

    obs = self._get_obs(data, info, partial)
    obs = self._apply_obs_delay(obs, info)
    reward, done = jp.zeros(2)
    return mjx_env.State(data, obs, reward, done, metrics, info)

  def step(self, state: mjx_env.State, action: jax.Array) -> mjx_env.State:
    state.info["rng"], push1_rng, push2_rng = jax.random.split(
        state.info["rng"], 3
    )
    push_theta = jax.random.uniform(push1_rng, maxval=2 * jp.pi)
    push_magnitude = jax.random.uniform(
        push2_rng,
        minval=self._config.push_config.magnitude_range[0],
        maxval=self._config.push_config.magnitude_range[1],
    )
    push = jp.array([jp.cos(push_theta), jp.sin(push_theta)])
    push *= (
        jp.mod(state.info["push_step"] + 1, state.info["push_interval_steps"])
        == 0
    )
    push *= self._config.push_config.enable
    qvel = state.data.qvel
    qvel = qvel.at[:2].set(push * push_magnitude + qvel[:2])
    data = state.data.replace(qvel=qvel)
    state = state.replace(data=data)

    # Action (command) delay: ring-buffer of recent policy actions.
    nu = self.mjx_model.nu
    action_history = (
        jp.roll(state.info["action_history"], nu).at[:nu].set(action)
    )
    state.info["action_history"] = action_history
    state.info["rng"], act_delay_rng = jax.random.split(state.info["rng"])
    act_lag = jax.random.randint(
        act_delay_rng,
        (),
        minval=0,
        maxval=self._action_delay + 1,
    )
    action_applied = action_history.reshape(
        (self._action_history_len, nu)
    )[act_lag]

    motor_targets = self._default_pose + action_applied * self._config.action_scale
    data = mjx_env.step(
        self.mjx_model, state.data, motor_targets, self.n_substeps
    )
    state.info["motor_targets"] = motor_targets

    partial, full = self._toe_heel_contact(data)
    # T1 parity: any foot point ends the swing. Flat landing is shaped by
    # feet_toe_only instead of gating the whole gait clock on full plant.
    contact_filt = partial | state.info["last_contact"]
    first_contact = (state.info["feet_air_time"] > 0.0) * contact_filt
    state.info["feet_air_time"] += self.dt
    state.info["feet_contact_time"] += self.dt
    p_f = data.site_xpos[self._feet_site_id]
    p_fz = p_f[..., -1]
    state.info["swing_peak"] = jp.maximum(state.info["swing_peak"], p_fz)

    obs = self._get_obs(data, state.info, partial)
    obs = self._apply_obs_delay(obs, state.info)
    done = self._get_termination(data)

    rewards = self._get_reward(
        data,
        action,
        state.info,
        state.metrics,
        done,
        first_contact,
        partial,
        full,
    )
    rewards = {
        k: v * self._config.reward_config.scales[k] for k, v in rewards.items()
    }
    reward = jp.clip(sum(rewards.values()) * self.dt, 0.0, 10000.0)

    state.info["push"] = push
    state.info["step"] += 1
    state.info["push_step"] += 1
    # T1-style: advance phase only when commanded; freeze stance phase at π.
    phase_tp1 = state.info["phase"] + state.info["phase_dt"]
    state.info["phase"] = jp.fmod(phase_tp1 + jp.pi, 2 * jp.pi) - jp.pi
    state.info["phase"] = jp.where(
        jp.linalg.norm(state.info["command"]) > 0.01,
        state.info["phase"],
        jp.ones(2) * jp.pi,
    )
    state.info["last_last_act"] = state.info["last_act"]
    state.info["last_act"] = action
    # Isaac / Playground resampling_time = 5 s → 500 steps at ctrl_dt=0.01.
    state.info["rng"], cmd_rng = jax.random.split(state.info["rng"])
    state.info["command"] = jp.where(
        state.info["step"] > 500,
        self.sample_command(cmd_rng),
        state.info["command"],
    )
    state.info["step"] = jp.where(
        done | (state.info["step"] > 500),
        0,
        state.info["step"],
    )
    state.info["feet_air_time"] *= ~partial
    state.info["feet_contact_time"] *= partial
    state.info["last_contact"] = partial
    state.info["swing_peak"] *= ~partial
    for k, v in rewards.items():
      state.metrics[f"reward/{k}"] = v
    state.metrics["swing_peak"] = jp.mean(state.info["swing_peak"])

    done = done.astype(reward.dtype)
    state = state.replace(data=data, obs=obs, reward=reward, done=done)
    return state

  def _apply_obs_delay(
      self, obs: Dict[str, jax.Array], info: dict[str, Any]
  ) -> Dict[str, jax.Array]:
    """Push current actor state into a ring buffer and return a lagged copy.

    Critic `privileged_state` stays undelayed (sim-only). Latency is sampled
    each step in {0, ..., sensor_delay}.
    """
    state = obs["state"]
    obs_size = self._actor_obs_size
    obs_history = (
        jp.roll(info["obs_history"], obs_size).at[:obs_size].set(state)
    )
    info["obs_history"] = obs_history
    info["rng"], delay_rng = jax.random.split(info["rng"])
    obs_lag = jax.random.randint(
        delay_rng,
        (),
        minval=0,
        maxval=self._sensor_delay + 1,
    )
    delayed_state = obs_history.reshape(
        (self._obs_history_len, obs_size)
    )[obs_lag]
    return {
        "state": delayed_state,
        "privileged_state": obs["privileged_state"],
    }

  def _get_termination(self, data: mjx.Data) -> jax.Array:
    fall_termination = self.get_gravity(data)[-1] < 0.0
    return (
        fall_termination | jp.isnan(data.qpos).any() | jp.isnan(data.qvel).any()
    )

  def _get_obs(
      self, data: mjx.Data, info: dict[str, Any], contact: jax.Array
  ) -> mjx_env.Observation:
    gyro = self.get_gyro(data)
    info["rng"], noise_rng = jax.random.split(info["rng"])
    noisy_gyro = (
        gyro
        + (2 * jax.random.uniform(noise_rng, shape=gyro.shape) - 1)
        * self._config.noise_config.level
        * self._config.noise_config.scales.gyro
    )

    gravity = data.site_xmat[self._site_id].T @ jp.array([0, 0, -1])
    info["rng"], noise_rng = jax.random.split(info["rng"])
    noisy_gravity = (
        gravity
        + (2 * jax.random.uniform(noise_rng, shape=gravity.shape) - 1)
        * self._config.noise_config.level
        * self._config.noise_config.scales.gravity
    )

    joint_angles = data.qpos[7:]
    info["rng"], noise_rng = jax.random.split(info["rng"])
    noisy_joint_angles = (
        joint_angles
        + (2 * jax.random.uniform(noise_rng, shape=joint_angles.shape) - 1)
        * self._config.noise_config.level
        * self._qpos_noise_scale
    )

    joint_vel = data.qvel[6:]
    info["rng"], noise_rng = jax.random.split(info["rng"])
    noisy_joint_vel = (
        joint_vel
        + (2 * jax.random.uniform(noise_rng, shape=joint_vel.shape) - 1)
        * self._config.noise_config.level
        * self._config.noise_config.scales.joint_vel
    )

    cos = jp.cos(info["phase"])
    sin = jp.sin(info["phase"])
    phase = jp.concatenate([cos, sin])

    linvel = self.get_local_linvel(data)
    info["rng"], noise_rng = jax.random.split(info["rng"])
    noisy_linvel = (
        linvel
        + (2 * jax.random.uniform(noise_rng, shape=linvel.shape) - 1)
        * self._config.noise_config.level
        * self._config.noise_config.scales.linvel
    )

    state = jp.hstack([
        noisy_linvel,
        noisy_gyro,
        noisy_gravity,
        info["command"],
        noisy_joint_angles - self._default_pose,
        noisy_joint_vel,
        info["last_act"],
        phase,
    ])

    accelerometer = self.get_accelerometer(data)
    global_angvel = self.get_global_angvel(data)
    feet_vel = data.sensordata[self._foot_linvel_sensor_adr].ravel()
    root_height = data.qpos[2]

    privileged_state = jp.hstack([
        state,
        gyro,
        accelerometer,
        gravity,
        linvel,
        global_angvel,
        joint_angles - self._default_pose,
        joint_vel,
        root_height,
        data.actuator_force,
        contact,
        feet_vel,
        info["feet_air_time"],
        info["feet_contact_time"],
    ])

    return {
        "state": state,
        "privileged_state": privileged_state,
    }

  def _get_reward(
      self,
      data: mjx.Data,
      action: jax.Array,
      info: dict[str, Any],
      metrics: dict[str, Any],
      done: jax.Array,
      first_contact: jax.Array,
      partial: jax.Array,
      full: jax.Array,
  ) -> dict[str, jax.Array]:
    del metrics
    return {
        "tracking_lin_vel": self._reward_tracking_lin_vel(
            info["command"], self.get_local_linvel(data)
        ),
        "tracking_ang_vel": self._reward_tracking_ang_vel(
            info["command"], self.get_gyro(data)
        ),
        "lin_vel_z": self._cost_lin_vel_z(self.get_global_linvel(data)),
        "ang_vel_xy": self._cost_ang_vel_xy(self.get_global_angvel(data)),
        "orientation": self._cost_orientation(self.get_gravity(data)),
        "base_height": self._cost_base_height(data.qpos[2]),
        "torques": self._cost_torques(data.actuator_force),
        "action_rate": self._cost_action_rate(
            action, info["last_act"], info["last_last_act"]
        ),
        "energy": self._cost_energy(data.qvel[6:], data.actuator_force),
        "feet_slip": self._cost_feet_slip(data, partial, info),
        "feet_clearance": self._cost_feet_clearance(data, info),
        "feet_height": self._cost_feet_height(
            info["swing_peak"], first_contact, info
        ),
        "feet_air_time": self._reward_feet_air_time(
            info["feet_air_time"],
            info["feet_contact_time"],
            partial,
            info["command"],
        ),
        "feet_flight": self._cost_feet_flight(partial),
        "feet_rapid_step": self._cost_feet_rapid_step(
            info["feet_air_time"], first_contact, info["command"]
        ),
        "feet_stuck_swing": self._cost_feet_stuck_swing(info["feet_air_time"]),
        "feet_toe_only": self._cost_feet_toe_only(partial, full),
        "feet_phase": self._reward_feet_phase(
            data,
            info["phase"],
            self._config.reward_config.max_foot_height,
            info["command"],
        ),
        "alive": self._reward_alive(),
        "termination": self._cost_termination(done),
        "stand_still": self._cost_stand_still(info["command"], data.qpos[7:]),
        "joint_deviation_hip": self._cost_joint_deviation_hip(
            data.qpos[7:], info["command"]
        ),
        "joint_deviation_knee": self._cost_joint_deviation_knee(data.qpos[7:]),
        "dof_pos_limits": self._cost_joint_pos_limits(data.qpos[7:]),
        "pose": self._cost_pose(data.qpos[7:]),
    }

  def _reward_tracking_lin_vel(
      self, commands: jax.Array, local_vel: jax.Array
  ) -> jax.Array:
    lin_vel_error = jp.sum(jp.square(commands[:2] - local_vel[:2]))
    return jp.exp(-lin_vel_error / self._config.reward_config.tracking_sigma)

  def _reward_tracking_ang_vel(
      self, commands: jax.Array, ang_vel: jax.Array
  ) -> jax.Array:
    ang_vel_error = jp.square(commands[2] - ang_vel[2])
    return jp.exp(-ang_vel_error / self._config.reward_config.tracking_sigma)

  def _cost_lin_vel_z(self, global_linvel) -> jax.Array:
    return jp.square(global_linvel[2])

  def _cost_ang_vel_xy(self, global_angvel) -> jax.Array:
    return jp.sum(jp.square(global_angvel[:2]))

  def _cost_orientation(self, torso_zaxis: jax.Array) -> jax.Array:
    return jp.sum(jp.square(torso_zaxis[:2]))

  def _cost_base_height(self, base_height: jax.Array) -> jax.Array:
    return jp.square(
        base_height - self._config.reward_config.base_height_target
    )

  def _cost_torques(self, torques: jax.Array) -> jax.Array:
    return jp.sum(jp.abs(torques))

  def _cost_energy(
      self, qvel: jax.Array, qfrc_actuator: jax.Array
  ) -> jax.Array:
    return jp.sum(jp.abs(qvel) * jp.abs(qfrc_actuator))

  def _cost_action_rate(
      self, act: jax.Array, last_act: jax.Array, last_last_act: jax.Array
  ) -> jax.Array:
    del last_last_act
    return jp.sum(jp.square(act - last_act))

  def _cost_joint_pos_limits(self, qpos: jax.Array) -> jax.Array:
    out_of_limits = -jp.clip(qpos - self._soft_lowers, None, 0.0)
    out_of_limits += jp.clip(qpos - self._soft_uppers, 0.0, None)
    return jp.sum(out_of_limits)

  def _cost_stand_still(
      self, commands: jax.Array, qpos: jax.Array
  ) -> jax.Array:
    cmd_norm = jp.linalg.norm(commands)
    return jp.sum(jp.abs(qpos - self._default_pose)) * (cmd_norm < 0.1)

  def _cost_termination(self, done: jax.Array) -> jax.Array:
    return done

  def _reward_alive(self) -> jax.Array:
    return jp.array(1.0)

  def _cost_joint_deviation_hip(
      self, qpos: jax.Array, cmd: jax.Array
  ) -> jax.Array:
    cost = jp.sum(
        jp.abs(qpos[self._hip_indices] - self._default_pose[self._hip_indices])
    )
    cost *= jp.abs(cmd[1]) > 0.1
    return cost

  def _cost_joint_deviation_knee(self, qpos: jax.Array) -> jax.Array:
    return jp.sum(
        jp.abs(
            qpos[self._knee_indices] - self._default_pose[self._knee_indices]
        )
    )

  def _cost_pose(self, qpos: jax.Array) -> jax.Array:
    return jp.sum(jp.square(qpos - self._default_pose) * self._weights)

  def _cost_feet_slip(
      self, data: mjx.Data, contact: jax.Array, info: dict[str, Any]
  ) -> jax.Array:
    del info
    body_vel = self.get_global_linvel(data)[:2]
    reward = jp.sum(jp.linalg.norm(body_vel, axis=-1) * contact)
    return reward

  def _cost_feet_clearance(
      self, data: mjx.Data, info: dict[str, Any]
  ) -> jax.Array:
    del info
    feet_vel = data.sensordata[self._foot_linvel_sensor_adr]
    vel_xy = feet_vel[..., :2]
    vel_norm = jp.sqrt(jp.linalg.norm(vel_xy, axis=-1))
    foot_pos = data.site_xpos[self._feet_site_id]
    foot_z = foot_pos[..., -1]
    delta = jp.abs(foot_z - self._config.reward_config.max_foot_height)
    return jp.sum(delta * vel_norm)

  def _cost_feet_height(
      self,
      swing_peak: jax.Array,
      first_contact: jax.Array,
      info: dict[str, Any],
  ) -> jax.Array:
    del info
    error = swing_peak / self._config.reward_config.max_foot_height - 1.0
    return jp.sum(jp.square(error) * first_contact)

  def _reward_feet_air_time(
      self,
      air_time: jax.Array,
      contact_time: jax.Array,
      partial: jax.Array,
      commands: jax.Array,
  ) -> jax.Array:
    """Isaac-Lab biped variant: pay the shorter of the two single-support timers.

    Paying only in single support is what forces alternation: a hop (flight or
    both feet planted together) earns nothing. The air_time_target cap plus
    feet_stuck_swing keep a leg parked in the air from out-earning a real step.
    """
    in_mode_time = jp.where(partial, contact_time, air_time)
    single_stance = jp.sum(partial) == 1
    reward = jp.min(jp.where(single_stance, in_mode_time, 0.0))
    reward = jp.clip(reward, max=self._config.reward_config.air_time_target)
    return reward * (jp.linalg.norm(commands) > 0.1)

  def _cost_feet_flight(self, partial: jax.Array) -> jax.Array:
    """Penalize both feet off the ground: BD should walk, not hop."""
    return (jp.sum(partial) == 0).astype(jp.float32)

  def _cost_feet_toe_only(
      self, partial: jax.Array, full: jax.Array
  ) -> jax.Array:
    """Penalize a foot resting on one end (toe or heel) instead of flat."""
    return jp.sum((partial & ~full).astype(jp.float32))

  def _cost_feet_stuck_swing(self, air_time: jax.Array) -> jax.Array:
    """Penalize a foot that stays airborne past a full swing (hop / one-leg)."""
    over = air_time - self._config.reward_config.max_swing_time
    return jp.sum(jp.clip(over, 0.0, None))

  def _cost_feet_rapid_step(
      self,
      air_time: jax.Array,
      first_contact: jax.Array,
      commands: jax.Array,
      threshold_min: float = 0.2,
  ) -> jax.Array:
    """Penalize steps with air-time shorter than threshold_min (rapid tap)."""
    cmd_norm = jp.linalg.norm(commands)
    short = jp.clip(threshold_min - air_time, 0.0, threshold_min)
    return jp.sum(short * first_contact) * (cmd_norm > 0.1)

  def _reward_feet_phase(
      self,
      data: mjx.Data,
      phase: jax.Array,
      foot_height: jax.Array,
      commands: jax.Array,
  ) -> jax.Array:
    # G1-style: track swing height from phase, only when moving / commanded.
    foot_pos = data.site_xpos[self._feet_site_id]
    foot_z = foot_pos[..., -1]
    rz = gait.get_rz(phase, swing_height=foot_height)
    error = jp.sum(jp.square(foot_z - rz))
    reward = jp.exp(-error / 0.01)
    body_linvel = self.get_global_linvel(data)[:2]
    body_angvel = self.get_global_angvel(data)[2]
    linvel_mask = jp.logical_or(
        jp.linalg.norm(body_linvel) > 0.1,
        jp.abs(body_angvel) > 0.1,
    )
    mask = jp.logical_or(linvel_mask, jp.linalg.norm(commands) > 0.01)
    return reward * mask

  def sample_command(self, rng: jax.Array) -> jax.Array:
    rng1, rng2, rng3, rng4 = jax.random.split(rng, 4)

    lin_vel_x = jax.random.uniform(
        rng1, minval=self._config.lin_vel_x[0], maxval=self._config.lin_vel_x[1]
    )
    lin_vel_y = jax.random.uniform(
        rng2, minval=self._config.lin_vel_y[0], maxval=self._config.lin_vel_y[1]
    )
    ang_vel_yaw = jax.random.uniform(
        rng3,
        minval=self._config.ang_vel_yaw[0],
        maxval=self._config.ang_vel_yaw[1],
    )

    cmd = jp.hstack([lin_vel_x, lin_vel_y, ang_vel_yaw])
    # Clip planar speed like Isaac max_lin_speed.
    speed = jp.linalg.norm(cmd[:2])
    cmd = cmd.at[:2].set(
        jp.where(speed > 0.35, cmd[:2] * (0.35 / (speed + 1e-6)), cmd[:2])
    )

    return jp.where(
        jax.random.bernoulli(rng4, p=0.1),
        jp.zeros(3),
        cmd,
    )
