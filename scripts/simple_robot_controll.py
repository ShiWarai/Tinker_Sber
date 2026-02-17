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
# from global_config import ROOT_DIR, PLAY_DIR
from global_config import ROBOT_SEL
from pynput import keyboard  # Добавляем pynput для обработки клавиш

import threading
import time

import rclpy
from rclpy.node import Node
from tinker_msgs.msg import LowState, LowCmd, MotorState
from rclpy.executors import MultiThreadedExecutor


class SimNode(Node):
    def __init__(self):
        super().__init__('Simulator_node')

        self.actions = torch.empty(10)

        self.latest_actions = torch.zeros(10)  # Храним последние полученные actions
        self.obs_received = False
        self.obs_to_send = None
        self.actions_received = False
        self.lock = threading.Lock()

        self.lowstate_subscriber = self.create_subscription(
            LowCmd,
            '/tinker_msgs/lowcmd',
            self.lowcmd_callback,
            10
        )

        self.lowstate_publisher = self.create_publisher(LowState, '/tinker_msgs/lowstate', 10)

        # self.control_timer = self.create_timer(0.01, self.control_loop)
        
        self.get_logger().info(f"Simulator node initialized")

    def lowcmd_callback(self, msg: LowCmd):
        actions_list = []
        for i in range(len(msg.motor_cmd)):
            actions_list.append(msg.motor_cmd[i].position)
        
        self.actions = torch.tensor(actions_list, dtype=torch.float32)
        self.actions_received = True

    def publish_lowstate(self, obs):
        msg = LowState()
        # msg.motor_state = []

        if isinstance(obs, torch.Tensor):
            obs_np = obs.cpu().numpy() if obs.is_cuda else obs.numpy()
        else:
            obs_np = obs

        msg.imu_state.gyroscope = obs_np[0:3]
        msg.imu_state.rpy = obs_np[3:6]

        for i in range(10):
            # motor_state = MotorState()
            msg.motor_state[i].position = float(obs_np[9+i])
            msg.motor_state[i].velocity = float(obs_np[19+i])
            # motor_state.torque = 0.0
            # motor_state.temperature_mosfet = 0
            # motor_state.temperature_rotor = 0
            # motor_state.error = 0
            # msg.motor_state.append(motor_state)

        self.lowstate_publisher.publish(msg)

    def wait_actions(self, obs):
        with self.lock:
            self.obs_to_send = obs
            self.obs_received = True
            self.actions_received = False

        self.publish_lowstate(obs)

        timeout = time.time() + 0.2
        while not self.actions_received and time.time() < timeout:
            rclpy.spin_once(self, timeout_sec=0.01)
        
        if self.actions_received:
            with self.lock:
                actions = self.latest_actions.clone()
                self.obs_received = False
            return actions
        else:
            self.get_logger().warn("Timeout waiting for actions, returning zeros")
            return torch.zeros(10)


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

    rclpy.init()

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
    # model_dict = torch.load(os.path.join(ROOT_DIR, PLAY_DIR))
    # policy.load_state_dict(model_dict['model_state_dict'])
    # policy.half()
    # policy = policy.to(env.device)

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

    sim_node = SimNode()
    # executor = MultiThreadedExecutor()
    # executor.add_node(sim_node)
    # executor.spin()

    for i in range(num_frames):
        rclpy.spin_once(sim_node, timeout_sec=0.001) 

        obs = env.get_observations()
        # actions = policy.act_teacher(obs.half())

        if hasattr(obs, 'obs'):
            obs_tensor = obs.obs
        else:
            obs_tensor = obs

        obs_for_ros = obs_tensor[0].cpu() if obs_tensor.is_cuda else obs_tensor[0]
        
        actions = sim_node.wait_actions(obs_for_ros)
        
        actions_tensor = actions.to(env.device).unsqueeze(0)

        obs, privileged_obs, rewards, costs, dones, infos = env.step(actions_tensor)
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

    rclpy.shutdown()

    if RECORD_FRAMES:
        video.release()

if __name__ == '__main__':
    task_registry.register("H1", LeggedRobot, H1ConstraintHimRoughCfg(), H1ConstraintHimRoughCfgPPO())
    RECORD_FRAMES = False
    args = get_args()
    args.task = ROBOT_SEL
    play(args)