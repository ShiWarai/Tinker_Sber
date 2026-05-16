import os
import numpy as np
import yaml
import onnxruntime as ort
from scipy.spatial.transform import Rotation as R
from rclpy.node import Node


class InferenceController:

    def __init__(self, node: Node, model_dir: str, robot_type: str):
        self.node = node

        self.config_file = os.path.join(model_dir, 'params.yaml')
        self.model_file  = os.path.join(model_dir, 'policy', 'policy.onnx')

        self.load_config(self.config_file)

        self.policy_session      = ort.InferenceSession(self.model_file)
        self.policy_input_name   = self.policy_session.get_inputs()[0].name
        self.policy_output_name  = self.policy_session.get_outputs()[0].name

        inp = self.policy_session.get_inputs()[0]
        self.node.get_logger().info(
            f'ONNX loaded: input={self.policy_input_name} shape={inp.shape}')

        # Infer expected observation size from the ONNX model
        self._expected_obs_size = int(inp.shape[0])
        # Infer whether the model expects a batch dim (old policy: [1, N]) or flat (new: [N])
        self._obs_needs_batch = (len(inp.shape) == 2)

        self.actions      = np.zeros(self.actions_size)
        self.observations = np.zeros(self._expected_obs_size)
        self._first_obs_logged = False
        self._first_act_logged = False
        self._pending_debug_lines = []

        self.node.get_logger().info('Inference controller initialised')


    def load_config(self, config_file: str):
        with open(config_file) as f:
            config = yaml.safe_load(f)

        cfg = config['TinkerCfg']
        self.joint_names      = cfg['joint_names']
        self.init_state       = cfg['init_state']['default_joint_angle']
        self.stand_duration   = cfg['stand_mode']['stand_duration']
        self.control_cfg      = cfg['control']
        self.rl_cfg           = cfg['normalization']
        self.obs_scales       = cfg['normalization']['obs_scales']
        self.actions_size     = cfg['size']['actions_size']
        self.observations_size = cfg['size']['observations_size']
        self.imu_orientation_offset = np.array(
            list(cfg['imu_orientation_offset'].values()))
        self.user_cmd_cfg     = cfg['user_cmd_scales']
        self.loop_frequency   = cfg['loop_frequency']

        self.init_joint_angles = np.array(
            [self.init_state[n] for n in self.joint_names], dtype=np.float32)

        self.node.get_logger().info(
            f'Config loaded: obs_size={self.observations_size} '
            f'actions_size={self.actions_size}')

    def compute_observation(
        self,
        imu_quat:            np.ndarray,   # (4,) wxyz
        imu_rpy:             np.ndarray,   # (3,) [roll, pitch, yaw] from sim
        base_ang_vel:        np.ndarray,   # (3,)
        joint_positions:     np.ndarray,   # (10,)
        joint_velocities:    np.ndarray,   # (10,)
        commands:            np.ndarray,   # (3,) [vx, vy, yaw_rate]
        foot_states_right:   np.ndarray,   # (4,) from BDKinematics
        foot_states_left:    np.ndarray,   # (4,)
        step_cmd_right:      np.ndarray,   # (4,) from LIPMStepPlanner
        step_cmd_left:       np.ndarray,   # (4,)
        phase_sin:           float,
        phase_cos:           float,
    ):
        try:
            # Quaternion wxyz -> xyzw for scipy
            q_xyzw = np.array(
                [imu_quat[1], imu_quat[2], imu_quat[3], imu_quat[0]],
                dtype=np.float64)
            rot = R.from_quat(q_xyzw)


            # base_heading
            base_heading = np.float32(imu_rpy[2])

            # projected_gravity
            projected_gravity = rot.inv().apply(
                np.array([0., 0., -1.])).astype(np.float32)

            # command scaling
            cmd_scale = np.array([
                self.user_cmd_cfg['lin_vel_x'],
                self.user_cmd_cfg['lin_vel_y'],
                self.user_cmd_cfg['ang_vel_yaw'],
            ], dtype=np.float32)
            scaled_commands = (commands * cmd_scale).astype(np.float32)

            # joint state
            scaled_dof_pos = ( joint_positions * self.obs_scales['dof_pos']).astype(np.float32)
            scaled_dof_vel = ( joint_velocities * self.obs_scales['dof_vel']).astype(np.float32)
            scaled_ang_vel = ( base_ang_vel * self.obs_scales['ang_vel']).astype(np.float32)

            obs = np.concatenate([
                [base_heading],         # 1
                scaled_ang_vel,         # 3
                projected_gravity,      # 3
                foot_states_right,      # 4
                foot_states_left,       # 4
                step_cmd_right,         # 4
                step_cmd_left,          # 4
                scaled_commands,        # 3
                [phase_sin],            # 1
                [phase_cos],            # 1
                scaled_dof_pos,         # 10
                scaled_dof_vel,         # 10
            ]).astype(np.float32)

            self.observations = obs

        except Exception as e:
            self.node.get_logger().error(
                f'[Inference] compute_observation error: {e}')

    def compute_actions(self):
        try:
            clip = float(self.rl_cfg['clip_scales']['clip_observations'])
            obs  = np.clip(self.observations, -clip, clip).astype(np.float32)


            if self._obs_needs_batch:
                inp = obs.reshape(1, -1)
            else:
                inp = obs.flatten()

            output = self.policy_session.run(
                [self.policy_output_name],
                {self.policy_input_name: inp})

            self.actions = np.asarray(output[0], dtype=np.float32).flatten()

            if not self._first_act_logged:
                lines = getattr(self, '_pending_debug_lines', [])
                lines += ['=== First actions ===']
                for i, val in enumerate(self.actions):
                    lines.append(f'  [{i:2d}] action[{i}] = {val:.6f}')
                print('\n'.join(lines), flush=True)
                self._first_act_logged = True

        except Exception as e:
            self.node.get_logger().error(
                f'[Inference] compute_actions error: {e}')
