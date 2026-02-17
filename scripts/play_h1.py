# SPDX-FileCopyrightText: Copyright (c) 2025
# SPDX-License-Identifier: BSD-3-Clause

import os
import sys
import argparse
import torch
import cv2
import numpy as np
import random
import matplotlib.pyplot as plt
from typing import Optional

from omni.isaac.lab.app import AppLauncher
from omni.isaac.lab.utils.dict import class_to_dict
from omni.isaac.lab.utils.io import dump_pickle, dump_yaml

# Добавляем пути для импорта
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from h1_rough_env import H1RoughEnv
from h1_rough_env_cfg import H1RoughEnvCfg
from rma_policy import ActorCriticRMA

# Глобальные конфиги (из global_config.py)
ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLAY_DIR = '/home/dzirt/RL/Alpha_Human_gym-main/logs/h1_constraint_trot/Oct09_15-54-01_/model_30000.pt'


def rand_commands(command_ranges, commands):
    """Случайная генерация команд (как в play.py)"""
    commands[0] = random.uniform(command_ranges.lin_vel_x[0], command_ranges.lin_vel_x[1])
    commands[1] = random.uniform(command_ranges.lin_vel_y[0], command_ranges.lin_vel_y[1])
    commands[2] = 0
    commands[3] = random.uniform(command_ranges.heading[0], command_ranges.heading[1])


def play_h1(
    checkpoint_path: str = PLAY_DIR,
    task_name: str = "H1",
    num_envs: int = 1,
    video_duration: float = 200.0,
    record_frames: bool = False,
    en_plot: bool = False,
):
    """Основная функция для запуска политики"""
    
    # Создаем конфигурацию
    env_cfg = H1RoughEnvCfg()
    env_cfg.scene.num_envs = num_envs
    env_cfg.terrain.num_rows = 3
    env_cfg.terrain.num_cols = 3
    env_cfg.terrain.curriculum = False
    
    # Отключаем шум и рандомизацию для play
    env_cfg.observation_noise_model = None
    env_cfg.domain_rand.push_robots = False
    env_cfg.domain_rand.randomize_friction = False
    env_cfg.domain_rand.randomize_base_com = False
    env_cfg.domain_rand.randomize_base_mass = False
    env_cfg.domain_rand.randomize_motor = False
    env_cfg.domain_rand.randomize_lag_timesteps = False
    
    # Включаем фильтр действий
    env_cfg.control.use_filter = True
    
    # Ограничиваем команды для play
    env_cfg.commands.lin_vel_x = [-0.0, 0.5]
    env_cfg.commands.lin_vel_y = [-0.5, 0.5]
    env_cfg.commands.ang_vel_yaw = [-1.0, 1.0]
    env_cfg.commands.heading = [-1.0, 1.0]
    
    # Создаем среду
    env = H1RoughEnv(cfg=env_cfg)
    
    # Загружаем политику
    policy = load_policy(checkpoint_path, env)
    env.set_policy(policy, teacher_act=True)  # Используем teacher для play
    
    # Сброс среды
    obs_dict = env.reset()
    obs = obs_dict["policy"]
    
    # Настройка записи видео
    video = None
    num_frames = int(video_duration / env.step_dt)
    print(f'Gathering {num_frames} frames')
    
    # Метрики
    action_rate = 0.0
    z_vel = 0.0
    xy_vel = 0.0
    
    # Настройка команд
    env.commands[:, 0] = 0
    env.commands[:, 1] = 0
    env.commands[:, 2] = 0
    env.commands[:, 3] = 0
    
    # Основной цикл
    for i in range(num_frames):
        # Обновление метрик
        action_rate += torch.sum(torch.abs(env.last_actions - env.actions))
        z_vel += torch.square(env.robot.data.root_lin_vel_w[:, 2])
        xy_vel += torch.sum(torch.square(env.robot.data.root_ang_vel_w[:, :2]), dim=1)
        
        # Ресемплинг команд каждые 500 шагов
        if i % 500 == 0 or i == 0:
            commands = [0] * 4
            rand_commands(env_cfg.commands, commands)
            print(f"Resample command at step {i}: {commands}")
            env.commands[:, 0] = commands[0]
            env.commands[:, 1] = commands[1]
            env.commands[:, 2] = commands[2]
            env.commands[:, 3] = commands[3]
        
        # Получение действий от политики
        actions = policy.act_teacher(obs.half())
        
        # Шаг среды
        obs_dict, rewards, dones, infos = env.step(actions)
        obs = obs_dict["policy"]
        
        # Рендеринг
        env.sim.render()
        
        # Запись видео
        if record_frames:
            frame = render_frame(env)
            if video is None:
                video = cv2.VideoWriter(
                    'record.mp4',
                    cv2.VideoWriter_fourcc(*'MP4V'),
                    int(1 / env.step_dt),
                    (frame.shape[1], frame.shape[0])
                )
            video.write(frame)
        
        # Обновление графиков
        if en_plot and i % 10 == 0:
            update_plots(env, i)
    
    # Вывод метрик
    print(f"Action rate: {action_rate / num_frames}")
    print(f"Z velocity: {z_vel / num_frames}")
    print(f"XY velocity: {xy_vel / num_frames}")
    
    if record_frames:
        video.release()
    
    # Профилирование
    profile_policy(policy, obs)


def load_policy(checkpoint_path: str, env: H1RoughEnv) -> ActorCriticRMA:
    """Загрузка RMA политики из чекпоинта"""
    
    # Создаем политику с правильными параметрами
    policy = ActorCriticRMA(
        num_prop=env._num_proprio,
        num_scan=env._num_scan,
        num_priv_latent=env._num_priv_latent,
        num_hist=env._history_len,
        num_actions=env._num_actions,
        num_costs=5,  # Из конфига
        scan_encoder_dims=None,
        actor_hidden_dims=[512, 256, 128],
        critic_hidden_dims=[512, 256, 128],
        priv_encoder_dims=[],
        activation='elu',
        init_noise_std=1.0,
        teacher_act=True,
        imi_flag=False,
    )
    
    # Загружаем веса
    checkpoint = torch.load(checkpoint_path, map_location=env.device)
    
    # Загружаем state dict (может потребоваться частичная загрузка)
    if 'model_state_dict' in checkpoint:
        policy.load_state_dict(checkpoint['model_state_dict'], strict=False)
    else:
        policy.load_state_dict(checkpoint, strict=False)
    
    policy = policy.half().to(env.device)
    policy.eval()
    
    # Сохраняем JIT для деплоя
    policy.save_torch_jit_policy('model/trot_jitt.pt', env.device)
    print(f"Policy loaded from {checkpoint_path}")
    
    return policy


def render_frame(env: H1RoughEnv) -> np.ndarray:
    """Рендеринг кадра для видео"""
    # В Isaac Lab кадры можно получать через render()
    # Это упрощенная версия
    rgb_data = env.sim.render()
    if rgb_data is not None:
        return rgb_data[0].cpu().numpy()
    return np.zeros((512, 512, 3), dtype=np.uint8)


def update_plots(env: H1RoughEnv, step: int):
    """Обновление графиков"""
    # Здесь можно реализовать plotting как в оригинале
    pass


def profile_policy(policy: torch.nn.Module, obs: torch.Tensor):
    """Профилирование политики"""
    with torch.profiler.profile(
        activities=[
            torch.profiler.ProfilerActivity.CPU,
            torch.profiler.ProfilerActivity.CUDA
        ]
    ) as prof:
        for _ in range(1000):
            with torch.no_grad():
                _ = policy.act_teacher(obs.half())
    
    print(prof.key_averages().table(
        sort_by="self_cuda_time_total",
        row_limit=10
    ))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--task", type=str, default="H1")
    parser.add_argument("--checkpoint", type=str, default=PLAY_DIR)
    parser.add_argument("--record", action="store_true", default=False)
    parser.add_argument("--plot", action="store_true", default=False)
    parser.add_argument("--headless", action="store_true", default=False)
    args = parser.parse_args()
    
    # Запускаем среду
    play_h1(
        checkpoint_path=args.checkpoint,
        task_name=args.task,
        num_envs=1,
        record_frames=args.record,
        en_plot=args.plot,
    )