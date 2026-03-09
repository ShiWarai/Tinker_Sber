import os
import sys
import copy
import numpy as np
import yaml
import onnxruntime as ort
from scipy.spatial.transform import Rotation as R
from functools import partial
from rclpy.node import Node
from collections import deque
# import limxsdk
# import limxsdk.robot.Rate as Rate
# import limxsdk.robot.Robot as Robot
# import limxsdk.robot.RobotType as RobotType
# import limxsdk.datatypes as datatypes

class InferenceController:
    def __init__(self, node: Node, model_dir, robot_type):
        # super().__init__(node)
        self.node = node
        # Initialize robot and type information
        self.robot_type = robot_type
        # Load configuration and model file paths based on robot type
        # self.config_file = f'{model_dir}/{self.robot_type}/params.yaml'
        # self.model_file = f'{model_dir}/{self.robot_type}/policy/policy.onnx'
        self.config_file = f'{model_dir}/params.yaml'
        self.model_file = f'{model_dir}/policy/policy.onnx'

        # Load configuration settings from the YAML file
        self.load_config(self.config_file)

        # Load the ONNX model and set up input and output names
        self.policy_session = ort.InferenceSession(self.model_file)
        self.policy_input_names = [self.policy_session.get_inputs()[0].name]
        self.policy_output_names = [self.policy_session.get_outputs()[0].name]
        self.node.get_logger().info(f'ONNX model loaded: input {self.policy_input_names[0]} with shape {self.policy_session.get_inputs()[0].shape}, output {self.policy_output_names[0]}')

        self.history_length = 5
        self.loop_count = 0
        self.gait_command = np.array([2.0, 0.5, 0.5])  # freq, offset, contact_duration
        
        self.obs_queue = deque(maxlen=self.history_length)

        self.base_ang_vel_queue = deque(maxlen=self.history_length)
        self.projected_gravity_queue = deque(maxlen=self.history_length)
        self.joint_positions_queue = deque(maxlen=self.history_length)
        self.joint_velocities_queue = deque(maxlen=self.history_length)
        self.actions_queue = deque(maxlen=self.history_length)
        self.scaled_commands_queue = deque(maxlen=self.history_length)
        self.gait_phase_queue = deque(maxlen=self.history_length)
        self.gait_command_queue = deque(maxlen=self.history_length)

        self.node.get_logger().info('Inference model initialized')


    # Load the configuration from a YAML file
    def load_config(self, config_file):
        with open(config_file, 'r') as f:
            config = yaml.safe_load(f)

        # Assign configuration parameters to controller variables
        self.joint_names = config['TinkerCfg']['joint_names']
        self.init_state = config['TinkerCfg']['init_state']['default_joint_angle']
        self.stand_duration = config['TinkerCfg']['stand_mode']['stand_duration']
        self.control_cfg = config['TinkerCfg']['control']
        self.rl_cfg = config['TinkerCfg']['normalization']
        self.obs_scales = config['TinkerCfg']['normalization']['obs_scales']
        self.actions_size = config['TinkerCfg']['size']['actions_size']
        self.observations_size = config['TinkerCfg']['size']['observations_size']
        self.imu_orientation_offset = np.array(list(config['TinkerCfg']['imu_orientation_offset'].values()))
        self.user_cmd_cfg = config['TinkerCfg']['user_cmd_scales']
        self.loop_frequency = config['TinkerCfg']['loop_frequency']
        
        # Initialize variables for actions, observations, and commands
        self.actions = np.zeros(self.actions_size)
        self.observations = np.zeros(self.observations_size)
        self.last_actions = np.zeros(self.actions_size)
        self.commands = np.zeros(3)  # command to the robot (e.g., velocity, rotation)
        self.scaled_commands = np.zeros(3)
        self.base_lin_vel = np.zeros(3)  # base linear velocity
        self.base_position = np.zeros(3)  # robot base position
        self.loop_count = 0  # loop iteration count
        self.stand_percent = 0  # percentage of time the robot has spent in stand mode
        self.policy_session = None  # ONNX model session for policy inference
        self.joint_num = len(self.joint_names)  # number of joints
        self.node.get_logger().info(f'Observation size: {self.observations_size}, Actions size: {self.actions_size}')

        # Initialize joint angles based on the initial configuration
        self.init_joint_angles = np.zeros(len(self.joint_names))
        for i in range(len(self.joint_names)):
            self.init_joint_angles[i] = self.init_state[self.joint_names[i]]
        
        # Set initial mode to "STAND"
        # self.mode = "STAND"
        self.node.get_logger().info('Inference config loaded')
        
    
    def compute_gait_phase(self):
        loop_count = self.loop_count
        gait_indices = (loop_count / self.loop_frequency) * self.gait_command[0]

        sin_phase = np.sin(2 * np.pi * gait_indices)
        cos_phase = np.cos(2 * np.pi * gait_indices)

        return np.array([sin_phase, cos_phase])
        
    def compute_gait_command(self):
        return self.gait_command

    def compute_observation(self,
                            imu_quat,
                            base_ang_vel,
                            joint_positions,
                            joint_velocities,
                            last_actions,
                            commands):

        try:
            # Convert IMU orientation from quaternion to Euler angles (ZYX convention)
            '''imu_orientation = np.array(self.imu_data_tmp.quat)'''

            imu_quat = np.asarray(imu_quat, dtype=np.float32)
            imu_quat_xyzw = np.array([imu_quat[1], imu_quat[2], imu_quat[3], imu_quat[0]], dtype=np.float32)
            q_wi = R.from_quat(imu_quat_xyzw).as_euler("zyx")

            # q_wi = R.from_quat(imu_quat).as_euler('zyx')  # Quaternion to Euler ZYX conversion
            inverse_rot = R.from_euler('zyx', q_wi).inv().as_matrix()  # Get the inverse rotation matrix

            # Project the gravity vector (pointing downwards) into the body frame
            gravity_vector = np.array([0, 0, -1])  # Gravity in world frame (z-axis down)
            projected_gravity = np.dot(inverse_rot, gravity_vector)  # Transform gravity into body frame

            # Create a command scaler matrix for linear and angular velocities
            command_scaler = np.diag([
                self.user_cmd_cfg['lin_vel_x'],  # Scale factor for linear velocity in x direction
                self.user_cmd_cfg['lin_vel_y'],  # Scale factor for linear velocity in y direction
                self.user_cmd_cfg['ang_vel_yaw']  # Scale factor for yaw (angular velocity)
            ])

            # Apply scaling to the command inputs (velocity commands)
            scaled_commands = np.dot(command_scaler, commands)

            # Compute gait phase
            gait_phase = self.compute_gait_phase()
            gait_command = self.gait_command

            # Scale current values
            # scaled_base_ang_vel = base_ang_vel * self.obs_scales['ang_vel']
            # rot = R.from_euler("zyx", self.imu_orientation_offset).as_matrix().astype(np.float32)
            # scaled_base_ang_vel = (rot @ base_ang_vel) * self.obs_scales["ang_vel"]
            scaled_base_ang_vel = base_ang_vel * self.obs_scales["ang_vel"]

            # scaled_joint_pos = self.init_joint_angles * self.obs_scales['dof_pos']
            scaled_joint_pos = (joint_positions - self.init_joint_angles) * self.obs_scales["dof_pos"]
            scaled_joint_vel = joint_velocities * self.obs_scales['dof_vel']

            obs = np.concatenate([scaled_base_ang_vel,
                                  projected_gravity,
                                  scaled_joint_pos, 
                                  scaled_joint_vel, 
                                  last_actions, 
                                  scaled_commands, 
                                  gait_phase,
                                  gait_command], 
                                axis=0).astype(np.float32)

            if len(self.obs_queue) == 0:
                for _ in range(self.history_length):
                    self.obs_queue.appendleft(np.zeros(44))

            self.obs_queue.appendleft(obs)
            
            # Fill the history queue with the current observation if it is empty
            '''if len(self.obs_queue) == 0:
                for _ in range(self.history_length):
                    self.base_ang_vel_queue.append(base_ang_vel * self.obs_scales['ang_vel'])
                    self.projected_gravity_queue.append(projected_gravity)
                    self.joint_positions_queue.append((joint_positions - self.init_joint_angles) * self.obs_scales['dof_pos'])
                    self.joint_velocities_queue.append(joint_velocities * self.obs_scales['dof_vel'])
                    self.actions_queue.append(last_actions)
                    self.scaled_commands_queue.append(scaled_commands)
                    self.gait_phase_queue.append(gait_phase)
                    self.gait_command_queue.append(gait_command)

            # Append the current observation to the history queue
            self.base_ang_vel_queue.append(base_ang_vel * self.obs_scales['ang_vel'])
            self.projected_gravity_queue.append(projected_gravity)
            self.joint_positions_queue.append((joint_positions - self.init_joint_angles) * self.obs_scales['dof_pos'])
            self.joint_velocities_queue.append(joint_velocities * self.obs_scales['dof_vel'])
            self.actions_queue.append(last_actions)
            self.scaled_commands_queue.append(scaled_commands)
            self.gait_phase_queue.append(gait_phase)
            self.gait_command_queue.append(gait_command)
            
            history_obs = np.concatenate([
                np.array(self.base_ang_vel_queue).flatten(),
                np.array(self.projected_gravity_queue).flatten(),
                np.array(self.joint_positions_queue).flatten(),
                np.array(self.joint_velocities_queue).flatten(),
                np.array(self.actions_queue).flatten(),
                np.array(self.scaled_commands_queue).flatten(),
                np.array(self.gait_phase_queue).flatten(),
                np.array(self.gait_command_queue).flatten()
            ])
            
            self.obs_queue = np.clip(
                history_obs,
                -self.rl_cfg['clip_scales']['clip_observations'],
                self.rl_cfg['clip_scales']['clip_observations']
            )'''

            # self.node.get_logger().info(f"[Inference] obs_queue: {self.obs_queue}")

            self.loop_count += 1
            
        
        except Exception as e:
            self.node.get_logger().error(f"\n[Inference] Error in compute_observation: {e}")

    def compute_actions(self):
        """
        Computes the actions based on the current observations using the policy session.
        """
        try:
            history_obs = np.concatenate(list(self.obs_queue), axis=0).astype(np.float32)
            # history_obs = self.obs_queue.astype(np.float32)

            clip = float(self.rl_cfg["clip_scales"]["clip_observations"])
            history_obs = np.clip(history_obs, -clip, clip).astype(np.float32)

            input_tensor = history_obs.reshape(1, -1)

            # self.node.get_logger().info(f"[Inference] input_tensor::\n{np.round(input_tensor, 2)}")

            inputs = {self.policy_input_names[0]: input_tensor}
            output = self.policy_session.run(self.policy_output_names, inputs)

            # self.actions = np.array(output).flatten()
            self.actions = np.asarray(output[0], dtype=np.float32).flatten()
            # print(self.actions)
            
            
        except Exception as e:
            self.node.get_logger().error(f"[Inference] Error in compute_actions: {e}")