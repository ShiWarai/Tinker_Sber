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
        
        self.base_ang_vel_queue = deque(maxlen=self.history_length)
        self.projected_gravity_queue = deque(maxlen=self.history_length)
        self.joint_positions_queue = deque(maxlen=self.history_length)
        self.joint_velocities_queue = deque(maxlen=self.history_length)
        self.last_actions_queue = deque(maxlen=self.history_length)
        self.scaled_commands_queue = deque(maxlen=self.history_length)
        self.gait_phase_queue = deque(maxlen=self.history_length)
        self.gait_command_queue = deque(maxlen=self.history_length)

        self.node.get_logger().info('Inference model initialized')

        '''# Prepare robot command structure with default values for mode, q, dq, tau, Kp, Kd
        self.robot_cmd = datatypes.RobotCmd()
        self.robot_cmd.mode = [0. for x in range(0, self.joint_num)]
        self.robot_cmd.q = [0. for x in range(0, self.joint_num)]
        self.robot_cmd.dq = [0. for x in range(0, self.joint_num)]
        self.robot_cmd.tau = [0. for x in range(0, self.joint_num)]
        self.robot_cmd.Kp = [self.control_cfg['stiffness'] for x in range(0, self.joint_num)]
        self.robot_cmd.Kd = [self.control_cfg['damping'] for x in range(0, self.joint_num)]'''

        '''# Prepare robot state structure
        self.robot_state = datatypes.RobotState()
        self.robot_state.tau = [0. for x in range(0, self.joint_num)]
        self.robot_state.q = [0. for x in range(0, self.joint_num)]
        self.robot_state.dq = [0. for x in range(0, self.joint_num)]
        self.robot_state_tmp = copy.deepcopy(self.robot_state)

        # Initialize IMU (Inertial Measurement Unit) data structure
        self.imu_data = datatypes.ImuData()
        self.imu_data.quat[0] = 0
        self.imu_data.quat[1] = 0
        self.imu_data.quat[2] = 0
        self.imu_data.quat[3] = 1
        self.imu_data_tmp = copy.deepcopy(self.imu_data)'''

        '''# Set up a callback to receive updated robot state data
        self.robot_state_callback_partial = partial(self.robot_state_callback)
        self.robot.subscribeRobotState(self.robot_state_callback_partial)

        # Set up a callback to receive updated IMU data
        self.imu_data_callback_partial = partial(self.imu_data_callback)
        self.robot.subscribeImuData(self.imu_data_callback_partial)

        # Set up a callback to receive updated SensorJoy
        self.sensor_joy_callback_partial = partial(self.sensor_joy_callback)
        self.robot.subscribeSensorJoy(self.sensor_joy_callback_partial)'''

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
        

    # Main control loop
    '''def run(self):
        # Initialize default joint angles for standing
        self.default_joint_angles = np.array([0.0] * len(self.joint_names))
        self.stand_percent += 1 / (self.stand_duration * self.loop_frequency)
        self.mode = "STAND"
        self.loop_count = 0

        # Set the loop rate based on the frequency in the configuration
        rate = Rate(self.loop_frequency)
        while True:
            self.update()
            rate.sleep()'''
        

    # Handle the stand mode for smoothly transitioning the robot into standing
    '''def handle_stand_mode(self):
        if self.stand_percent < 1:
            for j in range(len(self.joint_names)):
                # Interpolate between initial and default joint angles during stand mode
                pos_des = self.default_joint_angles[j] * (1 - self.stand_percent) + self.init_state[self.joint_names[j]] * self.stand_percent
                self.set_joint_command_aligned(j, pos_des)
            # Increment the stand percentage over time
            self.stand_percent += 1 / (self.stand_duration * self.loop_frequency)
        else:
            # Switch to walk mode after standing
            self.mode = "WALK"'''

    '''def align_robot_state(self, robot_state: datatypes.RobotState):
        aligned_robot_state = copy.deepcopy(robot_state)
        aligned_robot_state.q[1] = robot_state.q[3]
        aligned_robot_state.dq[1] = robot_state.dq[3]
        aligned_robot_state.tau[1] = robot_state.tau[3]
        
        aligned_robot_state.q[2] = robot_state.q[1]
        aligned_robot_state.dq[2] = robot_state.dq[1]
        aligned_robot_state.tau[2] = robot_state.tau[1]
        
        aligned_robot_state.q[3] = robot_state.q[4]
        aligned_robot_state.dq[3] = robot_state.dq[4]
        aligned_robot_state.tau[3] = robot_state.tau[4]
        
        aligned_robot_state.q[4] = robot_state.q[2]
        aligned_robot_state.dq[4] = robot_state.dq[2]
        aligned_robot_state.tau[4] = robot_state.tau[2]
        
        return aligned_robot_state'''
    
    # Handle the walk mode where the robot moves based on computed actions
    '''def handle_walk_mode(self):
        # Update the temporary robot state and IMU data
        self.robot_state_tmp = self.align_robot_state(copy.deepcopy(self.robot_state))
        self.imu_data_tmp = copy.deepcopy(self.imu_data)

        # Execute actions every 'decimation' iterations
        if self.loop_count % self.control_cfg['decimation'] == 0:
            self.compute_observation()
            self.compute_actions()
            # Clip the actions within predefined limits
            action_min = -self.rl_cfg['clip_scales']['clip_actions']
            action_max = self.rl_cfg['clip_scales']['clip_actions']
            self.actions = np.clip(self.actions, action_min, action_max)

        # Iterate over the joints and set commands based on actions
        joint_pos = np.array(self.robot_state_tmp.q)
        joint_vel = np.array(self.robot_state_tmp.dq)

        for i in range(len(joint_pos)):
            # Compute the limits for the action based on joint position and velocity
            action_min = (joint_pos[i] - self.init_joint_angles[i] +
                          (self.control_cfg['damping'] * joint_vel[i] - self.control_cfg['user_torque_limit']) /
                          self.control_cfg['stiffness'])
            action_max = (joint_pos[i] - self.init_joint_angles[i] +
                          (self.control_cfg['damping'] * joint_vel[i] + self.control_cfg['user_torque_limit']) /
                          self.control_cfg['stiffness'])

            # Clip action within limits
            self.actions[i] = max(action_min / self.control_cfg['action_scale_pos'],
                                  min(action_max / self.control_cfg['action_scale_pos'], self.actions[i]))

            # Compute the desired joint position and set it
            pos_des = self.actions[i] * self.control_cfg['action_scale_pos'] + self.init_joint_angles[i]
            self.set_joint_command_aligned(i, pos_des)

            # Save the last action for reference
            self.last_actions[i] = self.actions[i]'''
    
    def compute_gait_phase(self):
        loop_count = self.loop_count
        gait_indices = (loop_count / 100.0) * self.gait_command[0] % 1.0

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
            q_wi = R.from_quat(imu_quat).as_euler('zyx')  # Quaternion to Euler ZYX conversion
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
            scaled_base_ang_vel = base_ang_vel * self.obs_scales['ang_vel']
            scaled_joint_pos = (joint_positions - self.init_joint_angles) * self.obs_scales['dof_pos']
            scaled_joint_vel = joint_velocities * self.obs_scales['dof_vel']

            # Initialize queues if empty
            if len(self.base_ang_vel_queue) == 0:
                for _ in range(self.history_length):
                    self.base_ang_vel_queue.append(scaled_base_ang_vel)
                    self.projected_gravity_queue.append(projected_gravity)
                    self.joint_positions_queue.append(scaled_joint_pos)
                    self.joint_velocities_queue.append(scaled_joint_vel)
                    self.last_actions_queue.append(last_actions)
                    self.scaled_commands_queue.append(scaled_commands)
                    self.gait_phase_queue.append(gait_phase)
                    self.gait_command_queue.append(gait_command)

            # Add current values to queues
            self.base_ang_vel_queue.append(scaled_base_ang_vel)
            self.projected_gravity_queue.append(projected_gravity)
            self.joint_positions_queue.append(scaled_joint_pos)
            self.joint_velocities_queue.append(scaled_joint_vel)
            self.last_actions_queue.append(last_actions)
            self.scaled_commands_queue.append(scaled_commands)
            self.gait_phase_queue.append(gait_phase)
            self.gait_command_queue.append(gait_command)

            # Create observation from entire history
            history_obs = np.concatenate([
                np.array(self.base_ang_vel_queue).flatten(),
                np.array(self.projected_gravity_queue).flatten(),
                np.array(self.joint_positions_queue).flatten(),
                np.array(self.joint_velocities_queue).flatten(),
                np.array(self.last_actions_queue).flatten(),
                np.array(self.scaled_commands_queue).flatten(),
                np.array(self.gait_phase_queue).flatten(),
                np.array(self.gait_command_queue).flatten()
            ])

            self.observations = np.clip(
                history_obs,
                -self.rl_cfg['clip_scales']['clip_observations'],
                self.rl_cfg['clip_scales']['clip_observations']
            )

            self.loop_count += 1
        
        except Exception as e:
            self.node.get_logger().error(f"[Inference] Error in compute_observation: {e}")

    def compute_actions(self):
        """
        Computes the actions based on the current observations using the policy session.
        """
        try:
            input_tensor = self.observations.astype(np.float32).reshape(1, -1)
            inputs = {self.policy_input_names[0]: input_tensor}
            output = self.policy_session.run(self.policy_output_names, inputs)
            self.actions = np.array(output).flatten()
            
        except Exception as e:
            self.node.get_logger().error(f"[Inference] Error in compute_actions: {e}")