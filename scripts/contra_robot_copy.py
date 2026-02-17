# SPDX-FileCopyrightText: Copyright (c) 2021 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-FileCopyrightText: Copyright (c) 2021 ETH Zurich, Nikita Rudin
# SPDX-License-Identifier: BSD-3-Clause

import cv2
import os
import sys
sys.path.append('..') 
from isaacgym import gymapi
from envs import LeggedRobot
from modules import *
from utils import get_args, export_policy_as_jit, task_registry, Logger
from configs import *
from utils.helpers import class_to_dict
from utils.task_registry import task_registry
import numpy as np
import torch
from global_config import ROOT_DIR, PLAY_DIR
from global_config import ROBOT_SEL
from PIL import Image as im
from configs.h1_constraint_him_trot import H1ConstraintHimRoughCfg, H1ConstraintHimRoughCfgPPO

from utils.ploter import Plotter, initCanvas
import matplotlib.pyplot as plt
import random
import time

en_plot = 0

class AdvancedRobotController:
    def __init__(self, env):
        self.env = env
        self.device = env.device
        
        # Команды для политики
        self.commands = torch.zeros((env.num_envs, 4), device=env.device)
        
        # Параметры управления
        self.linear_vel = 0.0
        self.lateral_vel = 0.0
        self.angular_vel = 0.0
        self.heading = 0.0
        
        # Режимы управления
        self.control_mode = "high_level"  # "high_level" или "simple"
        
        # Параметры для высокоуровневого управления
        self.high_level_linear_step = 0.1
        self.high_level_lateral_step = 0.05
        self.high_level_angular_step = 0.2
        
        # Параметры для простого управления
        self.simple_step_forward = False
        self.simple_step_backward = False
        self.simple_turn_right = False
        self.simple_turn_left = False
        self.simple_step_timer = 0
        self.simple_step_duration = 30  # количество кадров для шага
        self.simple_turn_duration = 20  # количество кадров для поворота
        
        # Ограничения
        self.max_linear_vel = 1.5
        self.max_lateral_vel = 0.5
        self.max_angular_vel = 1.0
        
        # Флаги состояния
        self.paused = False
        self.stopped = True
        self.force_stop = False  # Принудительная остановка

    def update_commands(self):
        """Обновление команд управления в зависимости от режима"""
        if self.force_stop:
            # Принудительная остановка - нулевые команды
            self.commands[:, 0] = 0.0
            self.commands[:, 1] = 0.0
            self.commands[:, 2] = 0.0
            self.commands[:, 3] = 0.0
        elif self.control_mode == "high_level":
            # Высокоуровневое управление - плавное изменение скоростей
            self.commands[:, 0] = self.linear_vel
            self.commands[:, 1] = self.lateral_vel
            self.commands[:, 2] = self.angular_vel
            self.commands[:, 3] = self.heading
            
        else:  # simple mode
            # Простое управление - дискретные команды
            if self.simple_step_forward and self.simple_step_timer > 0:
                # Делаем шаг вперед
                self.commands[:, 0] = 0.5  # умеренная скорость вперед
                self.simple_step_timer -= 1
                if self.simple_step_timer <= 0:
                    self.simple_step_forward = False
                    
            elif self.simple_step_backward and self.simple_step_timer > 0:
                # Двигаемся назад
                self.commands[:, 0] = -0.3  # медленная скорость назад
                self.simple_step_timer -= 1
                if self.simple_step_timer <= 0:
                    self.simple_step_backward = False
                    
            elif self.simple_turn_right and self.simple_step_timer > 0:
                # Поворачиваем вправо
                self.commands[:, 2] = -0.8  # поворот вправо
                self.simple_step_timer -= 1
                if self.simple_step_timer <= 0:
                    self.simple_turn_right = False
                    
            elif self.simple_turn_left and self.simple_step_timer > 0:
                # Поворачиваем влево
                self.commands[:, 2] = 0.8  # поворот влево
                self.simple_step_timer -= 1
                if self.simple_step_timer <= 0:
                    self.simple_turn_left = False
                    
            else:
                # Стоим на месте
                self.commands[:, 0] = 0.0
                self.commands[:, 1] = 0.0
                self.commands[:, 2] = 0.0
                self.commands[:, 3] = 0.0
        
        # Применяем команды к среде
        self.env.commands[:, 0] = self.commands[:, 0]
        self.env.commands[:, 1] = self.commands[:, 1]
        self.env.commands[:, 2] = self.commands[:, 2]
        self.env.commands[:, 3] = self.commands[:, 3]

    def handle_keyboard_events(self):
        """Обработка событий клавиатуры для обоих режимов"""
        if self.env.headless == False:
            for evt in self.env.gym.query_viewer_action_events(self.env.viewer):
                if evt.action == "QUIT" and evt.value > 0:
                    return False
                elif evt.action == "toggle_viewer_sync" and evt.value > 0:
                    self.env.gym.viewer_sync(self.env.viewer, not self.env.gym.get_viewer_sync(self.env.viewer))
                elif evt.action == "free_cam" and evt.value > 0:
                    self.env.free_cam = not self.env.free_cam
                elif evt.action == "pause" and evt.value > 0:
                    self.paused = not self.paused
                    print(f"Simulation {'paused' if self.paused else 'resumed'}")
                elif evt.action == "toggle_control_mode" and evt.value > 0:
                    self.control_mode = "simple" if self.control_mode == "high_level" else "high_level"
                    self.stop_all_movements()
                    print(f"Control mode: {self.control_mode}")
                
                # Обработка в зависимости от режима
                if self.control_mode == "high_level":
                    self._handle_high_level_events(evt)
                else:
                    self._handle_simple_events(evt)
                    
        return True

    def _handle_high_level_events(self, evt):
        """Обработка событий для высокоуровневого управления"""
        if evt.action == "move_forward" and evt.value > 0:
            self.linear_vel = min(self.linear_vel + self.high_level_linear_step, self.max_linear_vel)
            self.force_stop = False
            self.stopped = False
            self.update_commands()
            print(f"Linear velocity: {self.linear_vel:.2f}")
        elif evt.action == "move_backward" and evt.value > 0:
            self.linear_vel = max(self.linear_vel - self.high_level_linear_step, -self.max_linear_vel)
            self.force_stop = False
            self.stopped = False
            self.update_commands()
            print(f"Linear velocity: {self.linear_vel:.2f}")
        elif evt.action == "turn_left" and evt.value > 0:
            self.angular_vel = min(self.angular_vel + self.high_level_angular_step, self.max_angular_vel)
            self.force_stop = False
            self.stopped = False
            self.update_commands()
            print(f"Angular velocity: {self.angular_vel:.2f}")
        elif evt.action == "turn_right" and evt.value > 0:
            self.angular_vel = max(self.angular_vel - self.high_level_angular_step, -self.max_angular_vel)
            self.force_stop = False
            self.stopped = False
            self.update_commands()
            print(f"Angular velocity: {self.angular_vel:.2f}")
        elif evt.action == "move_left" and evt.value > 0:
            self.lateral_vel = min(self.lateral_vel + self.high_level_lateral_step, self.max_lateral_vel)
            self.force_stop = False
            self.stopped = False
            self.update_commands()
            print(f"Lateral velocity: {self.lateral_vel:.2f}")
        elif evt.action == "move_right" and evt.value > 0:
            self.lateral_vel = max(self.lateral_vel - self.high_level_lateral_step, -self.max_lateral_vel)
            self.force_stop = False
            self.stopped = False
            self.update_commands()
            print(f"Lateral velocity: {self.lateral_vel:.2f}")
        elif evt.action == "stop" and evt.value > 0:
            self.force_stop = True
            self.stop_all_movements()
            print("FORCE STOP: All movements stopped, standing still")

    def _handle_simple_events(self, evt):
        """Обработка событий для простого управления"""
        if evt.action == "step_forward" and evt.value > 0:
            if not self.simple_step_forward:
                self.simple_step_forward = True
                self.simple_step_timer = self.simple_step_duration
                self.force_stop = False
                self.stopped = False
                print("Making step forward")
        elif evt.action == "step_backward" and evt.value > 0:
            if not self.simple_step_backward:
                self.simple_step_backward = True
                self.simple_step_timer = self.simple_step_duration
                self.force_stop = False
                self.stopped = False
                print("Moving backward")
        elif evt.action == "turn_right" and evt.value > 0:
            if not self.simple_turn_right:
                self.simple_turn_right = True
                self.simple_step_timer = self.simple_turn_duration
                self.force_stop = False
                self.stopped = False
                print("Turning right")
        elif evt.action == "turn_left" and evt.value > 0:
            if not self.simple_turn_left:
                self.simple_turn_left = True
                self.simple_step_timer = self.simple_turn_duration
                self.force_stop = False
                self.stopped = False
                print("Turning left")
        elif evt.action == "stop" and evt.value > 0:
            self.force_stop = True
            self.stop_all_movements()
            print("FORCE STOP: Standing completely still")

    def stop_all_movements(self):
        """Полная остановка всех движений"""
        self.linear_vel = 0.0
        self.lateral_vel = 0.0
        self.angular_vel = 0.0
        self.heading = 0.0
        self.simple_step_forward = False
        self.simple_step_backward = False
        self.simple_turn_right = False
        self.simple_turn_left = False
        self.simple_step_timer = 0
        self.stopped = True
        self.update_commands()

    def setup_keyboard_shortcuts(self):
        """Настройка горячих клавиш для обоих режимов"""
        if self.env.headless == False:
            # Общие клавиши
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_SPACE, "pause")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_ESCAPE, "QUIT")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_V, "toggle_viewer_sync")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_F, "free_cam")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_C, "toggle_control_mode")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_X, "stop")
            
            # Высокоуровневое управление (WSAD + QE)
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_W, "move_forward")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_S, "move_backward")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_A, "turn_left")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_D, "turn_right")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_Q, "move_left")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_E, "move_right")
            
            # Простое управление (стрелки)
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_UP, "step_forward")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_DOWN, "step_backward")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_RIGHT, "turn_right")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_LEFT, "turn_left")

class StablePolicyWrapper:
    """Обертка для политики, обеспечивающая стабильное стояние на месте"""
    def __init__(self, policy, env):
        self.policy = policy
        self.env = env
        self.stand_still_actions = None
        self.calculate_stand_still_pose()
        
    def calculate_stand_still_pose(self):
        """Вычисляет позу для статического стояния"""
        # Нейтральная поза для стояния (значения могут потребовать настройки под конкретного робота)
        if self.env.num_actions == 12:  # типично для четвероногих роботов
            self.stand_still_actions = torch.zeros((self.env.num_envs, self.env.num_actions), 
                                                  device=self.env.device)
            # Можете настроить конкретные углы для стабильного стояния
            
    def act_teacher(self, obs, force_stand_still=False):
        """Получение действий с возможностью принудительного стояния"""
        if force_stand_still and self.stand_still_actions is not None:
            return self.stand_still_actions.clone()
        else:
            return self.policy.act_teacher(obs)

def play(args):
    print("Advanced Robot Control with Stable Standing")
    print(f"PyTorch version: {torch.__version__}")
    print(f"CUDA available: {torch.cuda.is_available()}")
    print(f"CUDA devices: {torch.cuda.device_count()}")
    
    env_cfg, train_cfg = task_registry.get_cfgs(name=args.task)
    
    # Настройки для оптимального управления и стабильности
    video_duration = 200
    env_cfg.env.num_envs = min(env_cfg.env.num_envs, 1)
    env_cfg.terrain.num_rows = 3
    env_cfg.terrain.num_cols = 3
    env_cfg.terrain.curriculum = False
    env_cfg.noise.add_noise = False
    env_cfg.domain_rand.push_robots = False
    env_cfg.domain_rand.randomize_base_com = False
    env_cfg.domain_rand.randomize_base_mass = False
    env_cfg.domain_rand.randomize_motor = False
    env_cfg.domain_rand.randomize_lag_timesteps = False
    env_cfg.noise.add_noise = False
    env_cfg.domain_rand.randomize_friction = False
    env_cfg.domain_rand.randomize_restitution = False 
    env_cfg.control.use_filter = True
    
    # Критические настройки для уменьшения движения ног при стоянии
    env_cfg.rewards.feet_air_time = 0.0  # Уменьшаем поощрение за поднятие ног
    env_cfg.rewards.stand_still = 1.0    # Увеличиваем поощрение за стояние на месте
    
    # Подготовка среды
    env, _ = task_registry.make_env(name=args.task, args=args, env_cfg=env_cfg)
    
    # Инициализация расширенного контроллера
    controller = AdvancedRobotController(env)
    controller.setup_keyboard_shortcuts()
    
    # Загрузка политики
    obs = env.get_observations()
    policy_cfg_dict = class_to_dict(train_cfg.policy)
    runner_cfg_dict = class_to_dict(train_cfg.runner)
    actor_critic_class = eval(runner_cfg_dict["policy_class_name"])
    policy: ActorCriticRMA = actor_critic_class(env.cfg.env.n_proprio,
                                                      env.cfg.env.n_scan,
                                                      env.num_obs,
                                                      env.cfg.env.n_priv_latent,
                                                      env.cfg.env.history_len,
                                                      env.num_actions,
                                                      **policy_cfg_dict)
 
    model_dict = torch.load(os.path.join(ROOT_DIR, PLAY_DIR))
    policy.load_state_dict(model_dict['model_state_dict'])
    policy.half()
    policy = policy.to(env.device)
    
    # Обертываем политику для стабильного стояния
    policy_wrapper = StablePolicyWrapper(policy, env)
    
    print("\n=== Advanced Robot Control ===")
    print("Control Modes:")
    print("  C - Switch between High-level and Simple control")
    print("  X - FORCE STOP: Stand completely still")

    print("\nHigh-level Control (WSAD + QE):")
    print("  W/S - Move forward/backward")
    print("  A/D - Turn left/right") 
    print("  Q/E - Move left/right")

    print("\nSimple Control (Arrow keys):")
    print("  UP - Make a step forward")
    print("  DOWN - Move backward") 
    print("  LEFT/RIGHT - Turn left/right")

    print("\nGeneral:")
    print("  SPACE - Pause simulation")
    print("  ESC - Quit")
    print("==============================\n")
    
    # Настройка камеры
    camera_local_transform = gymapi.Transform()
    camera_local_transform.p = gymapi.Vec3(-0.5, -1, 0.1)
    camera_local_transform.r = gymapi.Quat.from_axis_angle(gymapi.Vec3(0,0,1), np.deg2rad(90))
    camera_props = gymapi.CameraProperties()
    camera_props.width = 512
    camera_props.height = 512

    cam_handle = env.gym.create_camera_sensor(env.envs[0], camera_props)
    body_handle = env.gym.get_actor_rigid_body_handle(env.envs[0], env.actor_handles[0], 0)
    env.gym.attach_camera_to_body(cam_handle, env.envs[0], body_handle, camera_local_transform, gymapi.FOLLOW_TRANSFORM)

    img_idx = 0
    num_frames = int(video_duration / env.dt)
    print(f'Running for {num_frames} frames')
    video = None

    # Основной цикл симуляции
    for i in range(num_frames):
        # Обработка событий клавиатуры
        if not controller.handle_keyboard_events():
            break
            
        # Пропуск шага симуляции если пауза
        if controller.paused:
            continue
            
        # Обновление команд контроллера
        controller.update_commands()
        
        # Получение действий от политики с учетом принудительной остановки
        if controller.force_stop:
            # Используем специальную позу для статического стояния
            actions = policy_wrapper.act_teacher(obs.half(), force_stand_still=True)
        else:
            # Обычное управление через политику
            actions = policy_wrapper.act_teacher(obs.half())
        
        # Шаг симуляции
        obs, privileged_obs, rewards, costs, dones, infos = env.step(actions)
        env.gym.step_graphics(env.sim)
        env.gym.render_all_camera_sensors(env.sim)
            
        if RECORD_FRAMES:
            img = env.gym.get_camera_image(env.sim, env.envs[0], cam_handle, gymapi.IMAGE_COLOR).reshape((512,512,4))[:,:,:3]
            if video is None:
                video = cv2.VideoWriter('stable_control.mp4', cv2.VideoWriter_fourcc(*'MP4V'), int(1 / env.dt), (img.shape[1],img.shape[0]))
            video.write(img)
            img_idx += 1
            
    if RECORD_FRAMES:
        video.release()
        print("Recording saved as 'stable_control.mp4'")

if __name__ == '__main__':
    task_registry.register("H1", LeggedRobot, H1ConstraintHimRoughCfg(), H1ConstraintHimRoughCfgPPO())
    RECORD_FRAMES = False
    args = get_args()
    args.task = ROBOT_SEL
    play(args)