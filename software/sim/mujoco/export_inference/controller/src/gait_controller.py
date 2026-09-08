import rclpy
import numpy as np
from rclpy.node import Node
from sensor_msgs.msg import Imu
from .factories import InputDeviceFactory
from .inference_controller import InferenceController
from tinker_msgs.msg import LowState, LowCmd, MotorCmd

LOW_LEVEL_STATE = "/low_level_state"
LOW_LEVEL_COMMAND = "/low_level_command"
IMU_STATE = "/imu_state"


class GaitController(Node):
    def __init__(self,
                 device_type: str,
                 model_path: str):
        super().__init__('gait_controller')

        self.device_type = device_type
        self.device = InputDeviceFactory.get_device(device_type, node=self)
        self.device.initialize()

        self.inference_controller = InferenceController(
            node=self, model_dir=model_path, robot_type='tinker'
        )

        self.imu_quat = np.array([0.0, 0.0, 0.0, 1.0])
        self.ang_vel = np.zeros(3)
        self.commands = np.zeros(3)
        self.positions = np.zeros(10)
        self.velocities = np.zeros(10)
        self.prev_action = np.zeros(10)

        self.first_state_received = False
        self.first_imu_received = False

        self.lowstate_subscriber = self.create_subscription(
            LowState,
            LOW_LEVEL_STATE,
            self.lowstate_callback,
            10,
        )
        self.imu_subscriber = self.create_subscription(
            Imu,
            IMU_STATE,
            self.imu_callback,
            10,
        )
        self.lowcmd_publisher = self.create_publisher(
            LowCmd,
            LOW_LEVEL_COMMAND,
            10,
        )

        self.control_timer = self.create_timer(0.01, self.control_loop)

        self.get_logger().info(
            f"Gait controller initialized with {device_type} input"
        )

    def lowstate_callback(self, msg: LowState):
        self.positions = np.array([motor.position for motor in msg.motor_state])
        self.velocities = np.array([motor.velocity for motor in msg.motor_state])

        if not self.first_state_received:
            self.get_logger().info("Received first LowState.")
            self.first_state_received = True

    def imu_callback(self, msg: Imu):
        self.imu_quat = np.array([
            msg.orientation.x,
            msg.orientation.y,
            msg.orientation.z,
            msg.orientation.w,
        ])
        self.ang_vel = np.array([
            msg.angular_velocity.x,
            msg.angular_velocity.y,
            msg.angular_velocity.z,
        ])

        if not self.first_imu_received:
            self.get_logger().info("Received first IMU state.")
            self.first_imu_received = True

    def publish_lowcmd_action(self, action):
        msg = LowCmd()
        msg.timestamp_state = self.get_clock().now()
        msg.motor_cmd = [MotorCmd() for _ in range(10)]

        for i, pos in enumerate(action):
            msg.motor_cmd[i].position = float(pos)

        self.lowcmd_publisher.publish(msg)

    def control_loop(self):
        try:
            if not self.first_state_received or not self.first_imu_received:
                return

            self.commands = self.device.get_commands()

            self.inference_controller.compute_observation(
                imu_quat=self.imu_quat,
                base_ang_vel=self.ang_vel,
                joint_positions=self.positions,
                joint_velocities=self.velocities,
                last_actions=self.prev_action,
                commands=self.commands,
            )
            self.inference_controller.compute_actions()
            actions = self.inference_controller.actions
            self.publish_lowcmd_action(actions)
            self.prev_action = actions

        except Exception as e:
            self.get_logger().error(f"Control loop error: {e}")

    def shutdown(self):
        self.device.shutdown()
        self.destroy_node()
