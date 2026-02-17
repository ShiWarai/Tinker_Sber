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
from pynput import keyboard

class RobotController:
    def __init__(self, env):
        self.env = env
        self.commands = torch.zeros((env.num_envs, 5), device=env.device)
        self.linear_vel = 0.0
        self.lateral_vel = 0.0
        self.angular_vel = 0.0
        self.heading = 0.0
        self.height = 0.3
        self.linear_vel_step = 0.1
        self.lateral_vel_step = 0.1
        self.angular_vel_step = 1.5  # Для поворотов
        self.max_linear_vel = 1.5
        self.max_lateral_vel = 0.5
        self.max_angular_vel = 6.0  # Увеличено для поворотов
        self.max_heading = 3.14
        self.min_height = 0.12
        self.max_height = 0.2
        self.paused = False
        # Нейтральные углы суставов из логов
        self.neutral_dof_pos = torch.tensor(
            [0.0, 0.08, 0.56, -1.12, -0.57, 0.0, -0.08, -0.56, 1.12, 0.57],
            device=env.device
        )

    def update_commands(self):
        self.commands[:, 0] = self.linear_vel
        self.commands[:, 1] = self.lateral_vel
        self.commands[:, 2] = self.angular_vel
        self.commands[:, 3] = self.heading
        self.commands[:, 4] = self.height
        self.env.commands[:] = self.commands
        # Принудительная стабилизация
        if abs(self.linear_vel) < 0.01 and abs(self.lateral_vel) < 0.01 and abs(self.angular_vel) < 0.01:
            self.env.actions[:] = self.neutral_dof_pos
            self.env.dof_vel[:] = torch.zeros_like(self.env.dof_vel)
            print("Neutral pose applied, velocities zeroed")

    def handle_keyboard_events(self):
        def on_press(key):
            try:
                if key.char == 'w':
                    self.linear_vel = min(self.linear_vel + self.linear_vel_step, self.max_linear_vel)
                elif key.char == 's':
                    self.linear_vel = max(self.linear_vel - self.linear_vel_step, -self.max_linear_vel)
                elif key.char == 'a':
                    self.lateral_vel = max(self.lateral_vel - self.lateral_vel_step, -self.max_lateral_vel)  # Влево
                elif key.char == 'd':
                    self.lateral_vel = min(self.lateral_vel + self.lateral_vel_step, self.max_lateral_vel)  # Вправо
                elif key.char == 'q':
                    self.angular_vel = min(self.angular_vel + self.angular_vel_step, self.max_angular_vel)  # Поворот влево
                elif key.char == 'e':
                    self.angular_vel = max(self.angular_vel - self.angular_vel_step, -self.max_angular_vel)  # Поворот вправо
                elif key.char == 'r':
                    self.height = min(self.height + 0.01, self.max_height)
                elif key.char == 'f':
                    self.height = max(self.height - 0.01, self.min_height)
                elif key.char == 'x':
                    self.linear_vel = 0.0
                    self.lateral_vel = 0.0
                    self.angular_vel = 0.0
                    self.heading = 0.0
                    self.env.reset()  # Сбрасываем среду
                    print("All commands reset to zero, environment reset")
                elif key.char == ' ':
                    self.paused = not self.paused
                    print(f"Simulation {'paused' if self.paused else 'resumed'}")
            except AttributeError:
                pass
            self.update_commands()
            print(f"Commands: vx={self.linear_vel:.2f}, vy={self.lateral_vel:.2f}, wz={self.angular_vel:.2f}, height={self.height:.2f}")

        def on_release(key):
            if key == keyboard.Key.esc:
                return False
            try:
                if key.char in ['w', 's', 'a', 'd', 'q', 'e']:
                    self.linear_vel *= 0.9
                    self.lateral_vel *= 0.9
                    self.angular_vel *= 0.8
                    if abs(self.linear_vel) < 0.01:
                        self.linear_vel = 0.0
                    if abs(self.lateral_vel) < 0.01:
                        self.lateral_vel = 0.0
                    if abs(self.angular_vel) < 0.01:
                        self.angular_vel = 0.0
                    self.update_commands()
                    print(f"Commands: vx={self.linear_vel:.2f}, vy={self.lateral_vel:.2f}, wz={self.angular_vel:.2f}, height={self.height:.2f}")
            except AttributeError:
                pass
            return True

        listener = keyboard.Listener(on_press=on_press, on_release=on_release)
        listener.start()
        return listener

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
    env_cfg, train_cfg = task_registry.get_cfgs(name=args.task)
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
    env_cfg.domain_rand.randomize_friction = False
    env_cfg.domain_rand.randomize_restitution = False
    env_cfg.control.use_filter = True
    env_cfg.control.filter_coeff = 0.1
    env_cfg.init_state.pos = [0.0, 0.0, 0.33]  # Вернули высоту из исходных логов
    # Задаем stiffness и damping как словари
    env_cfg.control.stiffness = {
        'J_L0': 20.0, 'J_L1': 20.0, 'J_L2': 20.0, 'J_L3': 20.0, 'J_L4_ankle': 20.0,
        'J_R0': 20.0, 'J_R1': 20.0, 'J_R2': 20.0, 'J_R3': 20.0, 'J_R4_ankle': 20.0
    }
    env_cfg.control.damping = {
        'J_L0': 0.5, 'J_L1': 0.5, 'J_L2': 0.5, 'J_L3': 0.5, 'J_L4_ankle': 0.5,
        'J_R0': 0.5, 'J_R1': 0.5, 'J_R2': 0.5, 'J_R3': 0.5, 'J_R4_ankle': 0.5
    }
    # Увеличиваем диапазон угловой скорости
    env_cfg.commands.ranges.ang_vel_yaw = [-6.0, 6.0]
    env_cfg.commands.ranges.lin_vel_y = [-0.5, 0.5]

    # Создание среды
    env, _ = task_registry.make_env(name=args.task, args=args, env_cfg=env_cfg)
    
    # Инициализация контроллера
    controller = RobotController(env)
    listener = controller.handle_keyboard_events()

    # Загрузка модели
    policy_cfg_dict = class_to_dict(train_cfg.policy)
    runner_cfg_dict = class_to_dict(train_cfg.runner)
    actor_critic_class = eval(runner_cfg_dict["policy_class_name"])
    policy = actor_critic_class(
        env.cfg.env.n_proprio,
        env.cfg.env.n_scan,
        env.num_obs,
        env.cfg.env.n_priv_latent,
        env.cfg.env.history_len,
        env.num_actions,
        **policy_cfg_dict
    )
    model_dict = torch.load(os.path.join(ROOT_DIR, PLAY_DIR))
    policy.load_state_dict(model_dict['model_state_dict'])
    policy.half()
    policy = policy.to(env.device)

    # Настройка камеры
    camera_local_transform = gymapi.Transform()
    camera_local_transform.p = gymapi.Vec3(-0.5, -1, 0.1)
    camera_local_transform.r = gymapi.Quat.from_axis_angle(gymapi.Vec3(0, 0, 1), np.deg2rad(90))
    camera_props = gymapi.CameraProperties()
    camera_props.width = 512
    camera_props.height = 512
    cam_handle = env.gym.create_camera_sensor(env.envs[0], camera_props)
    body_handle = env.gym.get_actor_rigid_body_handle(env.envs[0], env.actor_handles[0], 0)
    env.gym.attach_camera_to_body(cam_handle, env.envs[0], body_handle, camera_local_transform, gymapi.FOLLOW_TRANSFORM)

    video_duration = 200
    num_frames = int(video_duration / env.dt)
    print(f'Gathering {num_frames} frames')
    video = None
    img_idx = 0

    print("=== Управление роботом ===")
    print("W/S - вперед/назад")
    print("A/D - влево/вправо (A: left, D: right)")
    print("Q/E - поворот влево/вправо (Q: left, E: right)")
    print("R/F - увеличить/уменьшить высоту")
    print("X - остановка")
    print("SPACE - пауза")
    print("ESC - выход")
    print("==========================")

    for i in range(num_frames):
        if controller.paused:
            env.gym.step_graphics(env.sim)
            env.gym.draw_viewer(env.viewer, env.sim, True)
            env.gym.sync_frame_time(env.sim)
            continue

        # Используем модель только при ненулевых командах
        if abs(controller.linear_vel) > 0.01 or abs(controller.lateral_vel) > 0.01 or abs(controller.angular_vel) > 0.01:
            obs = env.get_observations()
            actions = policy.act_teacher(obs.half())
        else:
            actions = controller.neutral_dof_pos.repeat(env.num_envs, 1)
            env.dof_vel[:] = torch.zeros_like(env.dof_vel)
            print("Using neutral pose actions")

        obs, privileged_obs, rewards, costs, dones, infos = env.step(actions)
        
        # Проверка на падение
        if env.base_lin_vel[0, 2].item() < -0.8 or env.base_euler_xyz[0, :2].abs().max().item() > 0.8:
            env.reset()
            print("Environment reset due to instability")
        
        # Логирование для отладки
        print(f"Base linear velocity: vx={env.base_lin_vel[0, 0].item():.2f}, vy={env.base_lin_vel[0, 1].item():.2f}, vz={env.base_lin_vel[0, 2].item():.2f}")
        print(f"Base angular velocity: wz={env.base_ang_vel[0, 2].item():.2f}, Yaw: {env.base_euler_xyz[0, 2].item():.2f} rad")
        print(f"Actions: {actions[0].detach().cpu().numpy()}")
        print(f"DOF velocities: {env.dof_vel[0].detach().cpu().numpy()}")

        env.gym.step_graphics(env.sim)
        env.gym.render_all_camera_sensors(env.sim)
        env.gym.draw_viewer(env.viewer, env.sim, True)
        env.gym.sync_frame_time(env.sim)

        if RECORD_FRAMES:
            img = env.gym.get_camera_image(env.sim, env.envs[0], cam_handle, gymapi.IMAGE_COLOR).reshape((512, 512, 4))[:, :, :3]
            if video is None:
                video = cv2.VideoWriter('record.mp4', cv2.VideoWriter_fourcc(*'MP4V'), int(1 / env.dt), (img.shape[1], img.shape[0]))
            video.write(img)
            img_idx += 1

    if RECORD_FRAMES:
        video.release()

    listener.stop()

if __name__ == '__main__':
    task_registry.register("H1", LeggedRobot, H1ConstraintHimRoughCfg(), H1ConstraintHimRoughCfgPPO())
    RECORD_FRAMES = False
    args = get_args()
    args.task = ROBOT_SEL
    play(args)