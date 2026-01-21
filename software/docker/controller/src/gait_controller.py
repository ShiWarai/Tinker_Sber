import rclpy
import torch
import numpy as np
from rclpy.node import Node
from .factories import InputDeviceFactory
from .inference_model import InferenceModel
from tinker_msgs.msg import LowState, LowCmd, MotorCmd

class GaitController(Node):
    def __init__(self, 
                 device_type: str = 'keyboard',
                 model_path: str = './models/model_30000.pt'):
        super().__init__('gait_controller')

        # self.adapter_type = adapter_type
        self.device_type = device_type

        # self.adapter = AdapterFactory.get_adapter(adapter_type, node=self)
        # self.adapter.initialize()

        self.device = InputDeviceFactory.get_device(device_type, node=self)
        self.device.initialize()

        self.inference_model = InferenceModel(model_path)

        self.rpy = np.zeros(3)
        self.omega = np.zeros(3)
        self.commands = np.zeros(3)
        self.positions = np.zeros(10)
        self.velocities = np.zeros(10)
        self.prev_action = np.zeros(10)
        
        self.obs_buf = np.zeros(39)
        self.obs_tensor = torch.empty(39)
        # self.prev_action_tensor = torch.zeros(10)

        self.lowstate_subscriber = self.create_subscription(
            LowState,
            '/tinker_msgs/lowstate',
            self.lowstate_callback,
            10
        )

        self.lowcmd_publisher = self.create_publisher(
            LowCmd, 
            '/tinker_msgs/lowcmd',
            10)

        self.control_timer = self.create_timer(0.01, self.control_loop)
        
        self.get_logger().info(
            f"Gait controller initialized with {device_type} input"
        )

    def lowstate_callback(self, msg: LowState):
        # self.get_logger().info(f"Got state: omega={self.omega}, positions={self.positions[:2]}")
        imu_state = msg.imu_state
        self.omega = imu_state.gyroscope
        self.rpy = imu_state.rpy
        self.positions = np.array([motor.position for motor in msg.motor_state])
        self.velocities = np.array([motor.velocity for motor in msg.motor_state])

    def publish_lowcmd_action(self, action):
        msg = LowCmd()
        msg.motor_cmd = [MotorCmd() for _ in range(10)]
    
        for i, pos in enumerate(action):
            msg.motor_cmd[i].position = float(pos)

        self.lowcmd_publisher.publish(msg)

    def control_loop(self):
        try:
            self.commands = self.device.get_commands()

            self.obs_buf = np.concatenate([self.omega, 
                                           self.rpy, 
                                           self.commands,
                                           self.positions,
                                           self.velocities,
                                           self.prev_action])
            

            self.obs_tensor = torch.from_numpy(self.obs_buf).float().unsqueeze(0)

            # Run model, publish actions
            action = self.inference_model.run(self.obs_tensor)

            action = action.flatten()
            print(f'action: {action}')
            action = np.clip(action, -15, 15)
            self.publish_lowcmd_action(action)
            self.prev_action = action

        except Exception as e:
            self.get_logger().error(f"Control loop error: {e}")

    def shutdown(self):
        # self.adapter.shutdown()
        self.device.shutdown()
        self.destroy_node()

