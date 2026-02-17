# SPDX-FileCopyrightText: Copyright (c) 2025
# SPDX-License-Identifier: BSD-3-Clause

from typing import Optional, Tuple, Dict, List
import torch
import torch.nn as nn
import numpy as np

import omni.isaac.lab.utils.math as math_utils
from omni.isaac.lab.assets import Articulation
from omni.isaac.lab.envs import DirectRLEnv
from omni.isaac.lab.managers import SceneEntityCfg
from omni.isaac.lab.sensors import ContactSensor, RayCaster
from omni.isaac.lab.terrains import TerrainImporter
from omni.isaac.lab.utils.math import quat_rotate_inverse, wrap_to_pi, quat_apply_yaw
from omni.isaac.lab.utils.noise import NoiseModel

from .h1_rough_env_cfg import H1RoughEnvCfg


class H1RoughEnv(DirectRLEnv):
    """Среда для H1 с RMA архитектурой и constraint-based обучением"""
    
    cfg: H1RoughEnvCfg
    
    def __init__(self, cfg: H1RoughEnvCfg, render_mode: Optional[str] = None, **kwargs):
        super().__init__(cfg, render_mode, **kwargs)
        
        # Размерности из конфига
        self._num_actions = cfg.num_actions
        self._num_proprio = cfg.n_proprio
        self._num_scan = cfg.n_scan
        self._history_len = cfg.history_len
        self._num_priv_latent = cfg.n_priv_latent
        
        # Буферы для истории (как в LeggedRobot)
        self.obs_history_buf = torch.zeros(
            self.num_envs, self._history_len, self._num_proprio,
            device=self.device, dtype=torch.float32
        )
        self.action_history_buf = torch.zeros(
            self.num_envs, self._history_len, self._num_actions,
            device=self.device, dtype=torch.float32
        )
        self.contact_buf = torch.zeros(
            self.num_envs, cfg.reward_params.cycle_time, 2,
            device=self.device, dtype=torch.float32
        )
        
        # Lag буферы (из domain_rand)
        if cfg.domain_rand.randomize_lag_timesteps:
            self.lag_buffer = torch.zeros(
                self.num_envs, cfg.domain_rand.lag_timesteps, self._num_actions,
                device=self.device
            )
        
        if cfg.domain_rand.add_imu_lag:
            self.imu_lag_buffer = torch.zeros(
                self.num_envs, 6, cfg.domain_rand.imu_lag_timesteps_range[1] + 1,
                device=self.device
            )
        
        # Рандомизированные параметры
        self._init_randomize_buffers()
        
        # Команды
        self.commands = torch.zeros(self.num_envs, cfg.commands.num_commands, device=self.device)
        self.commands_scale = torch.tensor(
            [cfg.obs_scales.lin_vel, cfg.obs_scales.lin_vel, cfg.obs_scales.ang_vel],
            device=self.device
        )
        
        # Состояния стоп
        self.feet_air_time = torch.zeros(self.num_envs, 2, device=self.device)
        self.last_contacts = torch.zeros(self.num_envs, 2, dtype=torch.bool, device=self.device)
        
        # Default joint positions
        self.default_dof_pos = self._get_default_joint_pos()
        
        # PD gains
        self.p_gains = self._get_pd_gains()
        self.d_gains = self._get_pd_gains(damping=True)
        
        # Рандомизация gains
        if cfg.domain_rand.randomize_kpkd:
            self.kp_factor = torch.ones(self.num_envs, self._num_actions, device=self.device)
            self.kd_factor = torch.ones(self.num_envs, self._num_actions, device=self.device)
        
        # Motor strength
        if cfg.domain_rand.randomize_motor:
            self.motor_strength = torch.ones(self.num_envs, self._num_actions, device=self.device)
        
        # Motor offsets
        if cfg.domain_rand.randomize_motor_offset:
            self.motor_offsets = torch.zeros(self.num_envs, self._num_actions, device=self.device)
        
        # Noise model
        self._observation_noise_model = NoiseModel(
            num_envs=self.num_envs,
            num_obs=self.cfg.num_observations,
            device=self.device,
            noise_cfg=self.cfg.observation_noise_model,
        )
        
        # Для отслеживания
        self.actions = torch.zeros(self.num_envs, self._num_actions, device=self.device)
        self.last_actions = torch.zeros(self.num_envs, self._num_actions, device=self.device)
        self.last_last_actions = torch.zeros(self.num_envs, self._num_actions, device=self.device)
        self.torques = torch.zeros(self.num_envs, self._num_actions, device=self.device)
        self.last_torques = torch.zeros_like(self.torques)
        
        # Счетчики
        self.common_step_counter = 0
        self._step_count = 0
        self._resample_step = int(cfg.commands.resampling_time / self.step_dt)
        
        # RMA политика будет установлена извне
        self.policy: Optional[nn.Module] = None
        self.teacher_act = False  # Будет установлено из конфига PPO
    
    def _init_randomize_buffers(self):
        """Инициализация буферов для рандомизации (как init_randomize_props)"""
        cfg = self.cfg.domain_rand
        
        if cfg.randomize_base_mass:
            self.payload_masses = torch.zeros(self.num_envs, 1, device=self.device)
        
        if cfg.randomize_base_com:
            self.com_displacements = torch.zeros(self.num_envs, 3, device=self.device)
        
        if cfg.randomize_all_mass:
            self.mass_multipliers = torch.ones(self.num_envs, device=self.device)
        
        if cfg.randomize_com:
            self.com_multipliers = torch.zeros(self.num_envs, 3, device=self.device)
        
        if cfg.random_inertia:
            self.inertia_multipliers = torch.ones(self.num_envs, 9, device=self.device)
    
    def _setup_scene(self):
        """Настройка сцены"""
        # Террейн
        self.cfg.terrain.num_envs = self.scene.cfg.num_envs
        self.cfg.terrain.env_spacing = self.scene.cfg.env_spacing
        self.terrain = TerrainImporter(self.cfg.terrain)
        
        # Клонируем среды
        self.scene.clone_environments(copy_from_source=False)
        self.scene.filter_collisions(global_prim_paths=[])
        
        # Добавляем ассеты
        self.scene.articulations["robot"] = Articulation(self.cfg.robot)
        self.scene.sensors["contact_sensor"] = ContactSensor(self.cfg.contact_sensor)
        self.scene.sensors["ray_caster"] = RayCaster(self.cfg.ray_caster)
        
        self.scene.terrain = self.terrain
    
    def _pre_physics_step(self, actions: torch.Tensor):
        """Применение действий перед шагом физики"""
        # Сохраняем действия
        self.last_last_actions = self.last_actions.clone()
        self.last_actions = self.actions.clone()
        self.actions = actions.clone()
        
        # Добавляем шум к действиям если нужно
        if self.cfg.domain_rand.action_noise > 0:
            noise = self.cfg.domain_rand.action_noise * torch.randn_like(actions)
            self.actions = self.actions + noise
        
        # Обновляем историю действий
        self.action_history_buf = torch.cat([
            self.action_history_buf[:, 1:],
            self.actions.unsqueeze(1)
        ], dim=1)
        
        # Применяем lag если нужно
        if self.cfg.domain_rand.randomize_lag_timesteps:
            self.lag_buffer = torch.cat([
                self.lag_buffer[:, 1:],
                self.actions.unsqueeze(1)
            ], dim=1)
            
            # Берем действие с задержкой
            lag_indices = torch.randint(
                0, self.cfg.domain_rand.lag_timesteps,
                (self.num_envs,), device=self.device
            )
            actions_to_apply = self.lag_buffer[torch.arange(self.num_envs), lag_indices]
        else:
            actions_to_apply = self.actions
        
        # Вычисляем и применяем torques
        self.torques = self._compute_torques(actions_to_apply)
        self.robot.set_joint_effort_target(self.torques)
    
    def _compute_torques(self, actions: torch.Tensor) -> torch.Tensor:
        """Вычисление моментов через PD контроллер"""
        # Масштабируем действия
        actions_scaled = actions * self.cfg.control.action_scale
        
        # Получаем текущие состояния
        dof_pos = self.robot.data.joint_pos[:, :self._num_actions]
        dof_vel = self.robot.data.joint_vel[:, :self._num_actions]
        
        # Целевая позиция
        if self.cfg.domain_rand.randomize_motor_offset:
            joint_pos_target = actions_scaled + self.default_dof_pos + self.motor_offsets
        else:
            joint_pos_target = actions_scaled + self.default_dof_pos
        
        # Применяем PD контроллер
        if self.cfg.control.control_type == "P":
            if self.cfg.domain_rand.randomize_kpkd:
                torques = (self.kp_factor * self.p_gains * (joint_pos_target - dof_pos) -
                          self.kd_factor * self.d_gains * dof_vel)
            else:
                torques = self.p_gains * (joint_pos_target - dof_pos) - self.d_gains * dof_vel
        else:
            raise ValueError(f"Unknown control type: {self.cfg.control.control_type}")
        
        # Применяем motor strength
        if self.cfg.domain_rand.randomize_motor:
            torques = torques * self.motor_strength
        
        return torques
    
    def _get_observations(self) -> Dict[str, torch.Tensor]:
        """Получение наблюдений для политики"""
        # Проприоцептивные наблюдения
        proprio_obs = self._get_proprio_observations()
        
        # Обновляем историю
        self.obs_history_buf = torch.cat([
            self.obs_history_buf[:, 1:],
            proprio_obs.unsqueeze(1)
        ], dim=1)
        
        # Сканирование
        scan_obs = self._get_scan_observations()
        
        # Приватные латентные переменные
        priv_latent = self._get_priv_latent()
        
        # Формируем полное наблюдение
        obs = torch.cat([
            proprio_obs,
            scan_obs,
            priv_latent,
            self.obs_history_buf.reshape(self.num_envs, -1)
        ], dim=-1)
        
        # Добавляем шум если нужно
        if self.cfg.observation_noise_model is not None:
            obs = self._observation_noise_model.apply(obs)
        
        return {"policy": obs}
    
    def _get_proprio_observations(self) -> torch.Tensor:
        """Проприоцептивные наблюдения (39 размерность)"""
        # Применяем lag если нужно
        if self.cfg.domain_rand.add_imu_lag:
            self._update_imu_lag_buffer()
            base_ang_vel = self.lagged_base_ang_vel
            base_euler = self.lagged_base_euler
        else:
            base_ang_vel = self.robot.data.root_ang_vel_b
            base_euler = math_utils.get_euler_xyz(self.robot.data.root_quat_w)
            base_euler = torch.stack(base_euler, dim=-1)
        
        # Проецированная гравитация
        projected_gravity = quat_rotate_inverse(
            self.robot.data.root_quat_w, 
            self.terrain.envs_gravity
        )
        
        # DOF позиции и скорости (с lag если нужно)
        if hasattr(self, 'lagged_dof_pos'):
            dof_pos = self.lagged_dof_pos
            dof_vel = self.lagged_dof_vel
        else:
            dof_pos = self.robot.data.joint_pos[:, :self._num_actions]
            dof_vel = self.robot.data.joint_vel[:, :self._num_actions]
        
        # Собираем наблюдения (как в compute_observations)
        obs_list = [
            base_ang_vel * self.cfg.obs_scales.ang_vel,
            base_euler * self.cfg.obs_scales.quat,
            self.commands[:, :3] * self.commands_scale,
            (dof_pos - self.default_dof_pos) * self.cfg.obs_scales.dof_pos,
            dof_vel * self.cfg.obs_scales.dof_vel,
            self.action_history_buf[:, -1],  # Последнее действие
        ]
        
        return torch.cat(obs_list, dim=-1)
    
    def _get_scan_observations(self) -> torch.Tensor:
        """Наблюдения сканирования (187 размерность)"""
        ray_hits = self.scene.sensors["ray_caster"].data.ray_hits
        return ray_hits.reshape(self.num_envs, -1)
    
    def _get_priv_latent(self) -> torch.Tensor:
        """Приватные латентные переменные (из compute_observations)"""
        priv_list = [
            self.robot.data.root_lin_vel_b * self.cfg.obs_scales.lin_vel,
            self.contact_filt.float() - 0.5,
            self.randomized_lag_tensor if hasattr(self, 'randomized_lag_tensor') else torch.zeros(self.num_envs, 1, device=self.device),
            self.mass_params_tensor if hasattr(self, 'mass_params_tensor') else torch.zeros(self.num_envs, 4, device=self.device),
            self.friction_coeffs_tensor if hasattr(self, 'friction_coeffs_tensor') else torch.ones(self.num_envs, 1, device=self.device),
            self.restitution_coeffs_tensor if hasattr(self, 'restitution_coeffs_tensor') else torch.ones(self.num_envs, 1, device=self.device),
            self.motor_strength if hasattr(self, 'motor_strength') else torch.ones(self.num_envs, self._num_actions, device=self.device),
            self.kp_factor if hasattr(self, 'kp_factor') else torch.ones(self.num_envs, self._num_actions, device=self.device),
            self.kd_factor if hasattr(self, 'kd_factor') else torch.ones(self.num_envs, self._num_actions, device=self.device),
        ]
        
        return torch.cat(priv_list, dim=-1)
    
    def _get_rewards(self) -> torch.Tensor:
        """Вычисление наград (из compute_reward)"""
        rew_buf = torch.zeros(self.num_envs, device=self.device)
        
        # Отслеживание скорости
        rew_buf += self.cfg.rewards.tracking_lin_vel * self._reward_tracking_lin_vel()
        rew_buf += self.cfg.rewards.tracking_ang_vel * self._reward_tracking_ang_vel()
        
        # Ориентация
        rew_buf += self.cfg.rewards.orientation_eular * self._reward_orientation_eular()
        
        # Высота базы
        rew_buf += self.cfg.rewards.base_height * self._reward_base_height()
        
        # Энергия
        rew_buf += self.cfg.rewards.powers * self._reward_powers()
        rew_buf += self.cfg.rewards.torques * self._reward_torques()
        
        # Сглаженность действий
        rew_buf += self.cfg.rewards.action_smoothness * self._reward_action_smoothness()
        
        # Контакты стоп
        rew_buf += self.cfg.rewards.feet_air_time * self._reward_feet_air_time()
        rew_buf += self.cfg.rewards.foot_clearance * self._reward_foot_clearance()
        
        # Стояние
        rew_buf += self.cfg.rewards.stand_2leg * self._reward_stand_2leg()
        
        # ... добавить остальные награды
        
        return rew_buf
    
    def _get_dones(self) -> Tuple[torch.Tensor, torch.Tensor]:
        """Определение терминации"""
        time_out = self.episode_length_buf >= self.max_episode_length - 1
        
        # Проверка падения
        base_height = self.robot.data.root_pos_w[:, 2]
        fall_out = base_height < 0.2  # Порог
        
        # Проверка контактов с запрещенными частями
        contact_forces = self.scene.sensors["contact_sensor"].data.net_forces_w
        termination_contacts = torch.any(
            torch.norm(contact_forces[:, self.termination_indices], dim=-1) > 1.0,
            dim=1
        )
        
        # Проверка скоростей
        lin_vel_high = torch.norm(self.robot.data.root_lin_vel_b, dim=-1) > 10.0
        ang_vel_high = torch.norm(self.robot.data.root_ang_vel_b, dim=-1) > 10.0
        
        reset_buf = fall_out | termination_contacts | lin_vel_high | ang_vel_high | time_out
        
        return reset_buf, time_out
    
    def _reset_idx(self, env_ids: torch.Tensor | None):
        """Сброс среды"""
        if env_ids is None:
            env_ids = torch.arange(self.num_envs, device=self.device)
        
        super()._reset_idx(env_ids)
        
        # Сброс буферов
        self.obs_history_buf[env_ids] = 0.0
        self.action_history_buf[env_ids] = 0.0
        self.contact_buf[env_ids] = 0.0
        
        # Сброс команд
        self._resample_commands(env_ids)
        
        # Рандомизация физических свойств
        self._randomize_physics(env_ids)
        
        # Сброс состояний
        self.feet_air_time[env_ids] = 0.0
        self.last_contacts[env_ids] = False
    
    def _post_physics_step(self):
        """Пост-обработка"""
        self._step_count += 1
        self.common_step_counter += 1
        
        # Обновление контактов
        contact = self.scene.sensors["contact_sensor"].data.net_forces_w[:, self.feet_indices, 2] > self.cfg.reward_params.touch_thr
        self.contact_filt = torch.logical_or(contact, self.last_contacts)
        self.last_contacts = contact
        
        # Время в воздухе
        self.feet_air_time += self.cfg.sim.dt
        self.feet_air_time *= ~self.contact_filt
        
        # Обновление буфера контактов
        self.contact_buf = torch.cat([
            self.contact_buf[:, 1:],
            self.contact_filt.float().unsqueeze(1)
        ], dim=1)
        
        # Ресемплинг команд
        if self._step_count % self._resample_step == 0:
            self._resample_commands(torch.arange(self.num_envs, device=self.device))
        
        # Толчки
        if self.cfg.domain_rand.push_robots:
            push_env_ids = self.episode_length_buf % int(self.cfg.domain_rand.push_interval_s / self.step_dt) == 0
            if push_env_ids.any():
                self._push_robots(push_env_ids.nonzero(as_tuple=False).flatten())
    
    def _resample_commands(self, env_ids: torch.Tensor):
        """Ресемплинг команд (как в _resample_commands)"""
        r = self.cfg.commands
        
        self.commands[env_ids, 0] = torch_rand_float(
            r.lin_vel_x[0], r.lin_vel_x[1], (len(env_ids), 1), device=self.device
        ).squeeze(1)
        
        self.commands[env_ids, 1] = torch_rand_float(
            r.lin_vel_y[0], r.lin_vel_y[1], (len(env_ids), 1), device=self.device
        ).squeeze(1)
        
        if r.heading_command:
            self.commands[env_ids, 3] = torch_rand_float(
                r.heading[0], r.heading[1], (len(env_ids), 1), device=self.device
            ).squeeze(1)
        else:
            self.commands[env_ids, 2] = torch_rand_float(
                r.ang_vel_yaw[0], r.ang_vel_yaw[1], (len(env_ids), 1), device=self.device
            ).squeeze(1)
        
        self.commands[env_ids, 4] = torch_rand_float(
            0, 1, (len(env_ids), 1), device=self.device
        ).squeeze(1)
        
        # Zero out small commands
        self.commands[env_ids, :2] *= (torch.norm(self.commands[env_ids, :2], dim=1) > self.cfg.reward_params.command_dead).unsqueeze(1)
    
    def _randomize_physics(self, env_ids: torch.Tensor):
        """Рандомизация физических свойств"""
        # Масса
        if self.cfg.domain_rand.randomize_base_mass:
            min_mass, max_mass = self.cfg.domain_rand.added_mass_range
            self.payload_masses[env_ids] = torch_rand_float(
                min_mass, max_mass, (len(env_ids), 1), device=self.device
            )
        
        # COM
        if self.cfg.domain_rand.randomize_base_com:
            min_com, max_com = self.cfg.domain_rand.added_com_range
            self.com_displacements[env_ids] = torch_rand_float(
                min_com, max_com, (len(env_ids), 3), device=self.device
            )
        
        # Motor strength
        if self.cfg.domain_rand.randomize_motor:
            min_str, max_str = self.cfg.domain_rand.motor_strength_range
            self.motor_strength[env_ids] = torch_rand_float(
                min_str, max_str, (len(env_ids), self._num_actions), device=self.device
            )
        
        # PD gains
        if self.cfg.domain_rand.randomize_kpkd:
            min_kp, max_kp = self.cfg.domain_rand.kp_range
            min_kd, max_kd = self.cfg.domain_rand.kd_range
            self.kp_factor[env_ids] = torch_rand_float(
                min_kp, max_kp, (len(env_ids), self._num_actions), device=self.device
            )
            self.kd_factor[env_ids] = torch_rand_float(
                min_kd, max_kd, (len(env_ids), self._num_actions), device=self.device
            )
        
        # Motor offsets
        if self.cfg.domain_rand.randomize_motor_offset:
            min_off, max_off = self.cfg.domain_rand.motor_offset_range
            self.motor_offsets[env_ids] = torch_rand_float(
                min_off, max_off, (len(env_ids), self._num_actions), device=self.device
            )
    
    def _push_robots(self, env_ids: torch.Tensor):
        """Приложение толчков"""
        max_vel = self.cfg.domain_rand.max_push_vel_xy
        max_ang = self.cfg.domain_rand.max_push_ang_vel
        
        # Случайные скорости
        lin_vel = torch_rand_float(-max_vel, max_vel, (len(env_ids), 3), device=self.device)
        ang_vel = torch_rand_float(-max_ang, max_ang, (len(env_ids), 3), device=self.device)
        
        # Обновляем состояния
        self.robot.write_root_velocity_to_sim(lin_vel, env_indices=env_ids)
        self.robot.write_root_angular_velocity_to_sim(ang_vel, env_indices=env_ids)
    
    # ---------- Reward Functions ----------
    def _reward_tracking_lin_vel(self):
        lin_vel_error = torch.sum(torch.square(
            self.commands[:, :2] - self.robot.data.root_lin_vel_b[:, :2]
        ), dim=1)
        return torch.exp(-lin_vel_error * self.cfg.reward_params.tracking_sigma)
    
    def _reward_tracking_ang_vel(self):
        ang_vel_error = torch.square(
            self.commands[:, 2] - self.robot.data.root_ang_vel_b[:, 2]
        )
        return torch.exp(-ang_vel_error * self.cfg.reward_params.tracking_sigma)
    
    def _reward_orientation_eular(self):
        base_euler = torch.stack(math_utils.get_euler_xyz(self.robot.data.root_quat_w), dim=-1)
        return torch.exp(-torch.sum(torch.abs(base_euler[:, :2]), dim=1) * 10)
    
    def _reward_base_height(self):
        base_height = self.robot.data.root_pos_w[:, 2]
        return torch.exp(-torch.abs(base_height - self.cfg.reward_params.base_height_target) * 100)
    
    def _reward_powers(self):
        return torch.sum(torch.abs(self.torques) * torch.abs(self.robot.data.joint_vel[:, :self._num_actions]), dim=1)
    
    def _reward_torques(self):
        return torch.sum(torch.square(self.torques), dim=1)
    
    def _reward_action_smoothness(self):
        term1 = torch.sum(torch.square(self.last_actions - self.actions), dim=1)
        term2 = torch.sum(torch.square(
            self.actions + self.last_last_actions - 2 * self.last_actions
        ), dim=1)
        return term1 + term2
    
    def _reward_feet_air_time(self):
        first_contact = (self.feet_air_time > 0.0) * self.contact_filt
        rew_air_time = torch.sum(
            (self.feet_air_time - self.cfg.reward_params.cycle_time) * first_contact,
            dim=1
        )
        rew_air_time *= (torch.norm(self.commands[:, :3], dim=1) > self.cfg.reward_params.command_dead)
        return rew_air_time
    
    def _reward_foot_clearance(self):
        # Упрощенная версия
        foot_height = self.robot.data.body_pos_w[:, self.feet_indices, 2]
        foot_vel = torch.norm(self.robot.data.body_vel_w[:, self.feet_indices, :2], dim=2)
        
        height_error = torch.square(foot_height - self.cfg.reward_params.clearance_height_target)
        return torch.sum(height_error * foot_vel, dim=1)
    
    def _reward_stand_2leg(self):
        contacts = self.contact_filt.float()
        double_contact = torch.sum(contacts, dim=1) == 2
        single_contact = torch.sum(contacts, dim=1) == 1
        no_contact = torch.sum(contacts, dim=1) == 0
        
        standing = (torch.norm(self.commands[:, :3], dim=1) < self.cfg.reward_params.command_dead)
        
        rew = (1.0 * double_contact - 0.5 * no_contact - 1.0 * single_contact) * standing
        return rew
    
    # ---------- Utility Functions ----------
    def _get_default_joint_pos(self) -> torch.Tensor:
        """Получение дефолтных позиций суставов"""
        default_pos = torch.zeros(self.num_dof, device=self.device)
        for i, name in enumerate(self.robot.data.joint_names):
            if name in self.cfg.robot.init_state.joint_pos:
                default_pos[i] = self.cfg.robot.init_state.joint_pos[name]
        return default_pos.unsqueeze(0)
    
    def _get_pd_gains(self, damping: bool = False) -> torch.Tensor:
        """Получение PD gains"""
        gains = torch.zeros(self._num_actions, device=self.device)
        cfg_dict = self.cfg.robot.actuators["legs"].damping if damping else self.cfg.robot.actuators["legs"].stiffness
        
        for i, name in enumerate(self.robot.data.joint_names[:self._num_actions]):
            for key, value in cfg_dict.items():
                if key in name:
                    gains[i] = value
                    break
        return gains.unsqueeze(0)
    
    def set_policy(self, policy: nn.Module, teacher_act: bool = False):
        """Установка RMA политики"""
        self.policy = policy
        self.teacher_act = teacher_act
    
    @property
    def feet_indices(self) -> List[int]:
        """Индексы стоп"""
        return [i for i, name in enumerate(self.robot.data.body_names) if "ankle" in name]
    
    @property
    def termination_indices(self) -> List[int]:
        """Индексы для терминации"""
        return [i for i, name in enumerate(self.robot.data.body_names) if "base" in name]


def torch_rand_float(lower, upper, size, device):
    """Вспомогательная функция для случайных чисел"""
    return (upper - lower) * torch.rand(*size, device=device) + lower