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

en_plot = 0

class RobotController:
    def __init__(self, env):
        self.env = env
        # Команды: [lin_vel_x, lin_vel_y, ang_vel_yaw, heading, height] - 5 параметров
        self.commands = torch.zeros((env.num_envs, 5), device=env.device)
        self.linear_vel = 0.0
        self.lateral_vel = 0.0
        self.angular_vel = 0.0
        self.heading = 0.0
        self.height = 0.3  # высота по умолчанию
        
        # Параметры управления
        self.linear_vel_step = 0.1
        self.lateral_vel_step = 0.1
        self.angular_vel_step = 0.2
        self.heading_step = 0.5
        self.height_step = 0.01
        self.max_linear_vel = 1.5
        self.max_lateral_vel = 0.5
        self.max_angular_vel = 1.0
        self.max_heading = 3.14  # ±180 градусов
        self.min_height = 0.12
        self.max_height = 0.2
        
        # Флаги состояния
        self.paused = False

    def update_commands(self):
        """Обновление команд управления"""
        self.commands[:, 0] = self.linear_vel    # линейная скорость X
        self.commands[:, 1] = self.lateral_vel   # линейная скорость Y  
        self.commands[:, 2] = self.angular_vel   # угловая скорость
        self.commands[:, 3] = self.heading       # направление
        self.commands[:, 4] = self.height        # высота
        
        # Применяем команды к среде
        self.env.commands[:, 0] = self.linear_vel
        self.env.commands[:, 1] = self.lateral_vel
        self.env.commands[:, 2] = self.angular_vel
        self.env.commands[:, 3] = self.heading
        # height команда может быть в другом месте, зависит от реализации env

    def handle_keyboard_events(self):
        """Обработка событий клавиатуры"""
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
                
                # Управление движением вперед/назад
                elif evt.action == "vx_plus" and evt.value > 0:
                    self.linear_vel = min(self.linear_vel + self.linear_vel_step, self.max_linear_vel)
                    self.update_commands()
                    print(f"Linear velocity: {self.linear_vel:.2f}")
                elif evt.action == "vx_minus" and evt.value > 0:
                    self.linear_vel = max(self.linear_vel - self.linear_vel_step, -self.max_linear_vel)
                    self.update_commands()
                    print(f"Linear velocity: {self.linear_vel:.2f}")
                
                # Управление боковым движением (влево/вправо)
                elif evt.action == "vy_plus" and evt.value > 0:
                    self.lateral_vel = min(self.lateral_vel + self.lateral_vel_step, self.max_lateral_vel)
                    self.update_commands()
                    print(f"Lateral velocity: {self.lateral_vel:.2f}")
                elif evt.action == "vy_minus" and evt.value > 0:
                    self.lateral_vel = max(self.lateral_vel - self.lateral_vel_step, -self.max_lateral_vel)
                    self.update_commands()
                    print(f"Lateral velocity: {self.lateral_vel:.2f}")
                
                # Управление поворотом (влево/вправо) - ИСПРАВЛЕНО
                elif evt.action == "yaw_plus" and evt.value > 0:
                    # Для поворота влево - положительная угловая скорость
                    self.angular_vel = min(self.angular_vel + self.angular_vel_step, self.max_angular_vel)
                    self.update_commands()
                    print(f"Angular velocity: {self.angular_vel:.2f}")
                elif evt.action == "yaw_minus" and evt.value > 0:
                    # Для поворота вправо - отрицательная угловая скорость
                    self.angular_vel = max(self.angular_vel - self.angular_vel_step, -self.max_angular_vel)
                    self.update_commands()
                    print(f"Angular velocity: {self.angular_vel:.2f}")
                
                # Управление высотой
                elif evt.action == "height_plus" and evt.value > 0:
                    self.height = min(self.height + self.height_step, self.max_height)
                    self.update_commands()
                    print(f"Height: {self.height:.2f}")
                elif evt.action == "height_minus" and evt.value > 0:
                    self.height = max(self.height - self.height_step, self.min_height)
                    self.update_commands()
                    print(f"Height: {self.height:.2f}")
                
                # Сброс команд
                elif evt.action == "stop" and evt.value > 0:
                    self.linear_vel = 0.0
                    self.lateral_vel = 0.0
                    self.angular_vel = 0.0
                    self.heading = 0.0
                    self.update_commands()
                    print("All commands reset to zero")
                    
                # Постепенное уменьшение скорости при отпускании клавиш
                elif evt.action == "decelerate" and evt.value == 0:
                    # Плавное уменьшение скоростей
                    self.linear_vel *= 0.9
                    self.lateral_vel *= 0.9
                    self.angular_vel *= 0.8
                    if abs(self.linear_vel) < 0.01: self.linear_vel = 0.0
                    if abs(self.lateral_vel) < 0.01: self.lateral_vel = 0.0
                    if abs(self.angular_vel) < 0.01: self.angular_vel = 0.0
                    self.update_commands()
        return True

    def setup_keyboard_shortcuts(self):
        """Настройка горячих клавиш"""
        if self.env.headless == False:
            # Основное управление движением
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_W, "vx_plus")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_S, "vx_minus")
            
            # Боковое движение (A/D для движения влево/вправо)
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_A, "vy_minus")  # A - влево
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_D, "vy_plus")   # D - вправо
            
            # Поворот (Q/E для поворота влево/вправо) - ИСПРАВЛЕНО
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_Q, "yaw_plus")  # Q - поворот влево
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_E, "yaw_minus") # E - поворот вправо
            
            # Управление высотой (R/F)
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_R, "height_plus")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_F, "height_minus")
            
            # Сброс команд (X)
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_X, "stop")
            
            # Пауза (SPACE)
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_SPACE, "pause")
            
            # Управление камерой и выход
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_ESCAPE, "QUIT")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_V, "toggle_viewer_sync")
            self.env.gym.subscribe_viewer_keyboard_event(
                self.env.viewer, gymapi.KEY_C, "free_cam")
            
            # События отпускания клавиш для плавного замедления
            for key in [gymapi.KEY_W, gymapi.KEY_S, gymapi.KEY_A, gymapi.KEY_D, gymapi.KEY_Q, gymapi.KEY_E]:
                self.env.gym.subscribe_viewer_keyboard_event(
                    self.env.viewer, key, "decelerate")

def delete_files_in_directory(directory_path):
    try:
        files = os.listdir(directory_path)
        for file in files:
            file_path = os.path.join(directory_path, file)
            if os.path.isfile(file_path):
                os.remove(file_path)
        print("All files deleted successfully.")
    except OSError:
        print("Error occurred while deleting files.")

def play(args):
    print(torch.__version__)
    print(torch.cuda.is_available())
    print(torch.cuda.device_count())
    
    env_cfg, train_cfg = task_registry.get_cfgs(name=args.task)
    # override some parameters for testing
    video_duration = 200 #总体时间s
    
    env_cfg.env.num_envs = min(env_cfg.env.num_envs, 1)
    env_cfg.terrain.num_rows = 3
    env_cfg.terrain.num_cols = 3
    env_cfg.terrain.curriculum = False
    env_cfg.noise.add_noise = False
    env_cfg.domain_rand.push_robots = True
    env_cfg.domain_rand.randomize_base_com = False
    env_cfg.domain_rand.randomize_base_mass = False
    env_cfg.domain_rand.randomize_motor = False
    env_cfg.domain_rand.randomize_lag_timesteps = False
    env_cfg.noise.add_noise = True
    env_cfg.domain_rand.randomize_friction = False
    env_cfg.domain_rand.randomize_restitution = False 
    env_cfg.control.use_filter = True
    
    # prepare environment
    env, _ = task_registry.make_env(name=args.task, args=args, env_cfg=env_cfg)
    
    # Инициализация контроллера
    controller = RobotController(env)
    controller.setup_keyboard_shortcuts()
    
    # 策略输入
    obs = env.get_observations()
    # load policy
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
    print(f'gathering {num_frames} frames')
    video = None

    action_rate = 0
    z_vel = 0
    xy_vel = 0
    feet_air_time = 0

    if en_plot:
        plt.ion()
        initCanvas(3, 2, 100)
        plotter0 = Plotter(0, 'base_velx')
        plotter1 = Plotter(1, 'header')
        plotter2 = Plotter(2, 'joint hip')
        plotter3 = Plotter(3, 'joint thigh')
        plotter4 = Plotter(4, 'joint calf')

    print("=== Управление роботом ===")
    print("W/S - вперед/назад")
    print("A/D - влево/вправо")
    print("Q/E - поворот влево/вправо")
    print("R/F - увеличить/уменьшить высоту")
    print("X - остановка")
    print("SPACE - пауза")
    print("ESC - выход")
    print("==========================")

    # Основной цикл симуляции
    for i in range(num_frames):
        # Обработка событий клавиатуры
        if not controller.handle_keyboard_events():
            break
            
        # Пропуск шага симуляции если пауза
        if controller.paused:
            continue
            
        # Обновление статистики
        action_rate += torch.sum(torch.abs(env.last_actions - env.actions), dim=1)
        z_vel += torch.square(env.base_lin_vel[:, 2])
        xy_vel += torch.sum(torch.square(env.base_ang_vel[:, :2]), dim=1)

        # Получение действий от политики
        actions = policy.act_teacher(obs.half())
        
        # Шаг симуляции
        obs, privileged_obs, rewards, costs, dones, infos = env.step(actions)
        env.gym.step_graphics(env.sim)
        env.gym.render_all_camera_sensors(env.sim)

        # Визуализация
        if en_plot:
            plotter0.plotLine(env.base_lin_vel[0, 0].item(), env.commands[0, 0].item(), labels=['actual', 'command'])
            plotter1.plotLine(env.base_euler_xyz[0, 2].item(), env.commands[0, 3].item(), labels=['actual', 'command'])
            plotter2.plotLine(env.dof_pos[0, 0].item(), env.action_avg[0, 0].item(), labels=['q', 'exp'])
            plotter3.plotLine(env.dof_pos[0, 1].item(), env.action_avg[0, 1].item(), labels=['q', 'exp'])
            plotter4.plotLine(env.dof_pos[0, 2].item(), env.action_avg[0, 2].item(), labels=['q', 'exp'])
            
        if RECORD_FRAMES:
            img = env.gym.get_camera_image(env.sim, env.envs[0], cam_handle, gymapi.IMAGE_COLOR).reshape((512,512,4))[:,:,:3]
            if video is None:
                video = cv2.VideoWriter('record.mp4', cv2.VideoWriter_fourcc(*'MP4V'), int(1 / env.dt), (img.shape[1],img.shape[0]))
            video.write(img)
            img_idx += 1
            
    # Вывод статистики
    print("Action rate:", action_rate/num_frames)
    print("Z vel:", z_vel/num_frames)
    print("XY vel:", xy_vel/num_frames)
    print("Feet air reward", feet_air_time/num_frames)
    
    if RECORD_FRAMES:
        video.release()

    # Профилирование модели
    with torch.profiler.profile(activities=[torch.profiler.ProfilerActivity.CPU, torch.profiler.ProfilerActivity.CUDA]) as prof:
        for i in range(1000):
            with torch.no_grad():
                actions = policy.act_teacher(obs.half())
    print(prof.key_averages().table(sort_by="self_cuda_time_total", row_limit=10))

if __name__ == '__main__':
    task_registry.register("H1", LeggedRobot, H1ConstraintHimRoughCfg(), H1ConstraintHimRoughCfgPPO())
    RECORD_FRAMES = False
    args = get_args()
    args.task = ROBOT_SEL
    play(args)