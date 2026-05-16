import os
import numpy as np
import rclpy
import rclpy.parameter
from rclpy.node import Node
from scipy.spatial.transform import Rotation as R

from .factories import InputDeviceFactory
from .inference_controller import InferenceController
from .bd_kinematics import BDKinematics
from .lipm_planner import LIPMStepPlanner
from tinker_msgs.msg import LowState, LowCmd, MotorCmd


class GaitController(Node):
    # BD-specific constants
    _INIT_STANCE_HALF_WIDTH = 0.054   # half hip-to-hip distance [m]
    _COM_HEIGHT_NOMINAL     = 0.34    # nominal CoM height above ground [m]
    # Velocity estimation: low-pass decay on integrated accelerometer velocity.
    # Reduces drift while preserving short-term dynamics.
    _VEL_DECAY             = 0.98

    def __init__(self, device_type: str, model_path: str):
        super().__init__(
            'gait_controller',
            parameter_overrides=[
                rclpy.parameter.Parameter(
                    'use_sim_time',
                    rclpy.parameter.Parameter.Type.BOOL,
                    True)
            ]
        )

        self.device = InputDeviceFactory.get_device(device_type, node=self)
        self.device.initialize()

        self.inference_controller = InferenceController(
            node=self, model_dir=model_path, robot_type='tinker')

        # Forward kinematics (Pinocchio)
        self.kinematics = BDKinematics()

        # LIPM step planner
        loop_freq   = float(self.inference_controller.loop_frequency)
        dt          = 1.0 / loop_freq
        step_period = 25   # half-cycle in controller steps = 0.25 s at 100 Hz
        self.lipm   = LIPMStepPlanner(dt=dt, step_period=step_period,
                                      dstep_width=0.24, dstep_length=0.05)
        self.lipm.reset(init_stance_half_width=self._INIT_STANCE_HALF_WIDTH)

        # Sensor state
        self.imu_quat  = np.array([1., 0., 0., 0.], dtype=np.float32)  # wxyz
        self.ang_vel   = np.zeros(3, dtype=np.float32)
        self.accel     = np.zeros(3, dtype=np.float32)
        self.rpy       = np.zeros(3, dtype=np.float32)
        self.positions = np.zeros(10, dtype=np.float32)
        self.velocities = np.zeros(10, dtype=np.float32)
        self.commands  = np.zeros(3, dtype=np.float32)

        # Velocity / position estimate (world frame, relative to start)
        self._dt            = dt
        self._base_vel_world = np.zeros(3, dtype=np.float64)
        self._base_pos_world = np.zeros(3, dtype=np.float64)
        self._base_pos_world[2] = self._COM_HEIGHT_NOMINAL  # start at nominal height

        self.first_state_received = False
        self._step_count = 0
        self._LOG_EVERY = 50  # print every N control steps (1 s at 50 Hz)

        self.lowstate_subscriber = self.create_subscription(
            LowState, '/low_level_state', self.lowstate_callback, 10)
        self.lowcmd_publisher = self.create_publisher(
            LowCmd, '/low_level_cmd', 10)

        control_dt = 1.0 / loop_freq
        self.control_timer = self.create_timer(control_dt, self.control_loop)

    def lowstate_callback(self, msg: LowState):
        imu = msg.imu_state
        self.ang_vel   = np.array(imu.gyroscope,    dtype=np.float32)
        self.imu_quat  = np.array(imu.quaternion,   dtype=np.float32)  # wxyz
        self.accel     = np.array(imu.accelerometer, dtype=np.float32)
        self.rpy       = np.array(imu.rpy,          dtype=np.float32)
        self.positions = np.array([m.position for m in msg.motor_state], dtype=np.float32)
        self.velocities = np.array([m.velocity for m in msg.motor_state], dtype=np.float32)

        if not self.first_state_received:
            self.get_logger().info('First state received — control loop active.')
            self.first_state_received = True

        if not self.first_state_received:
            print(f'first state: positions={[m.position for m in msg.motor_state]}')


    def control_loop(self):
        
        try:
            if not self.first_state_received:
                return

            self.commands = np.array(self.device.get_commands(), dtype=np.float32)
            # TODO: replace with keyboard / joystick commands
            self.commands = np.array([0.2, 0., 0.], dtype=np.float32)

            # State estimation
            self._update_velocity_estimate()

            base_pos = self._base_pos_world.astype(np.float32)
            base_vel = self._base_vel_world.astype(np.float32)

            # Quaternion xyzw for scipy
            q_xyzw = np.array([
                self.imu_quat[1], self.imu_quat[2],
                self.imu_quat[3], self.imu_quat[0]], dtype=np.float64)

            # Base heading (yaw from IMU RPY, published by sim)
            base_heading = float(self.rpy[2])

            # Forward kinematics
            foot_right, foot_left, hip_right, hip_left = self.kinematics.compute(self.positions)

            # Convert foot states (body-frame from FK) to world-frame for LIPM
            R_base = R.from_quat(q_xyzw).as_matrix()
            foot_pos_right_world = base_pos + R_base @ foot_right[:3]
            foot_pos_left_world  = base_pos + R_base @ foot_left[:3]
            foot_head_right_world = float(foot_right[3]) + base_heading
            foot_head_left_world  = float(foot_left[3]) + base_heading

            # Actual CoM in world frame (weighted rigid-body sum via Pinocchio)
            com_body  = self.kinematics.compute_com(self.positions).astype(np.float64)
            com_world = base_pos + R_base @ com_body

            # LIPM step planner update
            self.lipm.update(
                base_pos=base_pos.astype(float),
                base_vel=base_vel.astype(float),
                base_heading=base_heading,
                commands=self.commands.astype(float),
                foot_pos_right_world=foot_pos_right_world.astype(float),
                foot_pos_left_world=foot_pos_left_world.astype(float),
                foot_heading_right=foot_head_right_world,
                foot_heading_left=foot_head_left_world,
                com=com_world,
            )

            step_cmd_right, step_cmd_left = self.lipm.get_step_commands_body(base_pos.astype(float), q_xyzw)

            phase_sin, phase_cos = self.lipm.get_phase_obs()

            # Observations
            self.inference_controller.compute_observation(
                imu_quat=self.imu_quat,
                imu_rpy=self.rpy,
                base_ang_vel=self.ang_vel,
                joint_positions=self.positions,
                joint_velocities=self.velocities,
                commands=self.commands,
                foot_states_right=foot_right,
                foot_states_left=foot_left,
                step_cmd_right=step_cmd_right,
                step_cmd_left=step_cmd_left,
                phase_sin=phase_sin,
                phase_cos=phase_cos,
                
            )

            # self._log_observations(
            #     base_heading, foot_right, foot_left,
            #     step_cmd_right, step_cmd_left,
            #     phase_sin, phase_cos,
            # )

            self._step_count += 1

            # Policy inference
            self.inference_controller.compute_actions()
            actions = self.inference_controller.actions.copy()

            clip_act = float(
                self.inference_controller.rl_cfg['clip_scales']['clip_actions'])
            actions = np.clip(actions, -clip_act, clip_act)

            q_des = (actions * self.inference_controller.control_cfg['action_scale_pos']
                        + self.inference_controller.init_joint_angles)

            self.publish_lowcmd_action(q_des)

        except Exception as e:
            self.get_logger().error(f'Control loop error: {e}')

    # Just function for logging
    def _log_observations(
        self,
        base_heading,
        foot_right, foot_left,
        step_cmd_right, step_cmd_left,
        phase_sin, phase_cos
    ):
        ic = self.inference_controller
        dof_pos = self.positions * ic.obs_scales['dof_pos']
        dof_vel = self.velocities * ic.obs_scales['dof_vel']
        ang_vel = self.ang_vel * ic.obs_scales['ang_vel']

        q_xyzw = np.array([
            self.imu_quat[1], self.imu_quat[2],
            self.imu_quat[3], self.imu_quat[0]], dtype=np.float64)
        from scipy.spatial.transform import Rotation as _R
        proj_grav = _R.from_quat(q_xyzw).inv().apply([0., 0., -1.])

        lines = [
            f'=== step {self._step_count} ===',
            f'  base_heading:      {base_heading:.4f}',
            f'  base_ang_vel:      {ang_vel}',
            f'  projected_gravity: {proj_grav}',
            f'  foot_states_right: {foot_right}',
            f'  foot_states_left:  {foot_left}',
            f'  step_cmd_right:    {step_cmd_right}',
            f'  step_cmd_left:     {step_cmd_left}',
            f'  commands:          {self.commands}',
            f'  phase_sin:         {phase_sin:.4f}',
            f'  phase_cos:         {phase_cos:.4f}',
            f'  dof_pos: {dof_pos}',
            f'  dof_vel: {dof_vel}',
        ]
        print('\n'.join(lines), flush=True)


    def _update_velocity_estimate(self):

        q_xyzw = np.array([
            self.imu_quat[1], self.imu_quat[2],
            self.imu_quat[3], self.imu_quat[0]], dtype=np.float64)
        R_mat = R.from_quat(q_xyzw).as_matrix()

        # Rotate accelerometer reading to world frame, then remove gravity
        acc_world = R_mat @ self.accel.astype(np.float64)
        acc_world[2] -= 9.81

        # Integrate with decay to limit long-term drift
        self._base_vel_world = (
            self._base_vel_world * self._VEL_DECAY + acc_world * self._dt)
        self._base_pos_world += self._base_vel_world * self._dt

        # Keep z at nominal CoM height (flat ground assumption)
        self._base_pos_world[2] = self._COM_HEIGHT_NOMINAL


    def publish_lowcmd_action(self, action: np.ndarray):
        msg = LowCmd()
        msg.motor_cmd = [MotorCmd() for _ in range(10)]

        for i in range(5):
            msg.motor_cmd[i].position     = float(action[2 * i])
            msg.motor_cmd[i + 5].position = float(action[2 * i + 1])

        self.lowcmd_publisher.publish(msg)

    def shutdown(self):
        self.device.shutdown()
        self.destroy_node()
