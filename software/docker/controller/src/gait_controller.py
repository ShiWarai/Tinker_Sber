import rclpy
import torch
import numpy as np
from rclpy.node import Node
from .factories import InputDeviceFactory
from .inference_controller import InferenceController
from tinker_msgs.msg import LowState, LowCmd, MotorCmd

class GaitController(Node):
    def __init__(self, 
                 device_type: str,
                 model_path: str):
        super().__init__('gait_controller')

        # self.adapter_type = adapter_type
        self.device_type = device_type

        # self.adapter = AdapterFactory.get_adapter(adapter_type, node=self)
        # self.adapter.initialize()

        self.device = InputDeviceFactory.get_device(device_type, node=self)
        self.device.initialize()

        self.inference_controller = InferenceController(node=self, model_dir=model_path, robot_type='tinker')
        # self.inference_controller.load_config(config_file=f'{model_path}/params.yaml')

        self.rpy = np.zeros(3)
        self.imu_quat = np.array([0, 0, 0, 1])
        self.ang_vel = np.zeros(3)
        self.commands = np.zeros(3)
        self.positions = np.zeros(10)
        self.velocities = np.zeros(10)
        self.prev_action = np.zeros(10)
        
        self.observations = np.zeros(self.inference_controller.observations_size)

        self.first_state_received = False

        self.lowstate_subscriber = self.create_subscription(
            LowState,
            '/tinker_msgs/lowstate',
            self.lowstate_callback,
            10
        )

        self.lowcmd_publisher = self.create_publisher(
            LowCmd, 
            '/tinker_msgs/lowcmd',
            10
        )

        self.control_timer = self.create_timer(0.01, self.control_loop)
        
        self.get_logger().info(
            f"Gait controller initialized with {device_type} input"
        )

    def lowstate_callback(self, msg: LowState):
        # self.get_logger().info(f"Got state: omega={self.omega}, positions={self.positions[:2]}")
        imu_state = msg.imu_state
        self.ang_vel = imu_state.gyroscope
        self.rpy = imu_state.rpy
        self.imu_quat = imu_state.quaternion
        self.positions = np.array([motor.position for motor in msg.motor_state])
        self.velocities = np.array([motor.velocity for motor in msg.motor_state])

        if not self.first_state_received:
            self.get_logger().info("Gait controller ready to start control loop.")
            self.first_state_received = True

    def publish_lowcmd_action(self, action):
        msg = LowCmd()
        msg.motor_cmd = [MotorCmd() for _ in range(10)]
    
        for i, pos in enumerate(action):
            msg.motor_cmd[i].position = float(pos)

        self.lowcmd_publisher.publish(msg)

    def control_loop(self):
        try:
            # if not self.first_state_received:
            #     self.get_logger().debug("Gait controller is waiting for first LowState message...")
            #     return
            
            self.commands = self.device.get_commands()

            '''self.obs_buf = np.concatenate([self.omega, 
                                           self.rpy, 
                                           self.commands,
                                           self.positions,
                                           self.velocities,
                                           self.prev_action])
            

            self.obs_tensor = torch.from_numpy(self.obs_buf).float().unsqueeze(0)

            # Run model, publish actions
            action = self.inference_model.run(self.obs_tensor)'''
            self.inference_controller.compute_observation(imu_quat=self.imu_quat,
                                                          base_ang_vel=self.ang_vel,
                                                          joint_positions=self.positions,
                                                          joint_velocities=self.velocities,
                                                          last_actions=self.prev_action,
                                                          commands=self.commands)
            self.inference_controller.compute_actions()
            actions = self.inference_controller.actions
            print(f'controller ouput actions: {actions}')
            self.publish_lowcmd_action(actions)
            self.prev_action = actions

        except Exception as e:
            self.get_logger().error(f"Control loop error: {e}")

    def shutdown(self):
        self.device.shutdown()
        self.destroy_node()

