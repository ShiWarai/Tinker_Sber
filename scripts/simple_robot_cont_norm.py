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
from pynput import keyboard  # Добавляем pynput для обработки клавиш

class RobotController:
    def __init__(self, env):
        self.env = env
        self.commands = torch.zeros((env.num_envs, 5), device=env.device)
        self.linear_vel = 0.0
        self.lateral_vel = 0.0
        self.angular_vel = 0.0
        self.heading = 0.0
        self.height = 0.3
        # Увеличенные параметры для поворотов
        self.linear_vel_step = 0.1
        self.lateral_vel_step = 0.1
        self.angular_vel_step = 0.8  # Увеличено для более выраженных поворотов
        self.max_linear_vel = 1.5
        self.max_lateral_vel = 0.5
        self.max_angular_vel = 2.0  # Увеличено
        self.max_heading = 3.14
        self.min_height = 0.12
        self.max_height = 0.2
        self.paused = False
        # Нейтральные углы суставов для стояния на месте
        self.neutral_dof_pos = torch.zeros(env.num_actions, device=env.device)

    def update_commands(self):
        self.commands[:, 0] = self.linear_vel
        self.commands[:, 1] = self.lateral_vel
        self.commands[:, 2] = self.angular_vel
        self.commands[:, 3] = self.heading
        self.commands[:, 4] = self.height
        self.env.commands[:] = self.commands
        # Если все скорости нулевые, принудительно задаем нейтральные углы
        if abs(self.linear_vel) < 0.01 and abs(self.lateral_vel) < 0.01 and abs(self.angular_vel) < 0.01:
            self.env.actions[:] = self.neutral_dof_pos

    def handle_keyboard_events(self):
        def on_press(key):
            try:
                if key.char == 'w':
                    self.linear_vel = min(self.linear_vel + self.linear_vel_step, self.max_linear_vel)
                elif key.char == 's':
                    self.linear_vel = max(self.linear_vel - self.linear_vel_step, -self.max_linear_vel)
                elif key.char == 'a':
                    self.lateral_vel = max(self.lateral_vel - self.lateral_vel_step, -self.max_lateral_vel)
                elif key.char == 'd':
                    self.lateral_vel = min(self.lateral_vel + self.lateral_vel_step, self.max_lateral_vel)
                elif key.char == 'q':
                    self.angular_vel = min(self.angular_vel + self.angular_vel_step, self.max_angular_vel)
                elif key.char == 'e':
                    self.angular_vel = max(self.angular_vel - self.angular_vel_step, -self.max_angular_vel)
                elif key.char == 'r':
                    self.height = min(self.height + 0.01, self.max_height)
                elif key.char == 'f':
                    self.height = max(self.height - 0.01, self.min_height)
                elif key.char == 'x':
                    self.linear_vel = 0.0
                    self.lateral_vel = 0.0
                    self.angular_vel = 0.0
                    self.heading = 0.0
                    print("All commands reset to zero")
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
    env_cfg.noise.add_noise = True
    env_cfg.domain_rand.push_robots = True
    env_cfg.domain_rand.randomize_base_com = False
    env_cfg.domain_rand.randomize_base_mass = False
    env_cfg.domain_rand.randomize_motor = False
    env_cfg.domain_rand.randomize_lag_timesteps = False
    env_cfg.domain_rand.randomize_friction = False
    env_cfg.domain_rand.randomize_restitution = False
    env_cfg.control.use_filter = True

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
    print("A/D - влево/вправо")
    print("Q/E - поворот влево/вправо")
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

        obs = env.get_observations()
        actions = policy.act_teacher(obs.half())
        obs, privileged_obs, rewards, costs, dones, infos = env.step(actions)
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