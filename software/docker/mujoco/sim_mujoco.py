import mujoco
import mujoco.viewer
import os
import time
import rclpy
import torch
import numpy as np
from rclpy.node import Node
from rclpy.executors import MultiThreadedExecutor
from tinker_msgs.msg import LowState, LowCmd, MotorCmd

class MujocoSim(Node):
    def __init__(self):
        super().__init__("mujoco_sim")

        self.rpy = np.zeros(3)
        self.imu_quat = np.zeros(4)
        self.ang_vel = np.zeros(3)
        self.commands = np.zeros(3)
        self.positions = np.zeros(10)
        self.velocities = np.zeros(10)

        self.actions = np.zeros(10)

        self.cmd_subscriber = self.create_subscription(
            LowCmd,
            '/tinker_msgs/lowcmd',
            self.cmd_callback,
            10
        )

        self.state_publisher = self.create_publisher(
            LowState,
            '/tinker_msgs/lowstate',
            10
        )

    def cmd_callback(self, msg: LowCmd):
        for motor_cmd in msg.motor_cmd:
            # TODO: add motor command processing into actions
            pass

    def publish_state(self):
        msg = LowState()
        msg.imu_state.gyroscope = self.ang_vel
        msg.imu_state.rpy = self.rpy
        msg.imu_state.quaternion = self.imu_quat

        # TODO: add motor state publishing
        self.publish_state(msg)

    def control_loop(self):
        try:
            # TODO: add control loop logic:
            #  1. get actions from self.actions
            #  2. apply actions to the model - ?{add position control (PID)}?
            #  3. update positions and velocities
            #  4. publish state
            pass
        except Exception as e:
            print(f'Control loop error: {e}')

    def shutdown(self):
        self.destroy_node()


if __name__ == "__main__":
    rclpy.init()
    try:
        current_dir = os.path.dirname(os.path.abspath(__file__))
        xml_path = os.path.join(current_dir, "xml", "world.xml")

        model = mujoco.MjModel.from_xml_path(xml_path)
        data = mujoco.MjData(model)

        mujoco_sim = MujocoSim()

        executor = MultiThreadedExecutor()
        executor.add_node(mujoco_sim)

        with mujoco.viewer.launch(model, data) as viewer:
            viewer.cam.lookat[:] = [0, 0, 0.5]
            viewer.cam.distance = 2.0
            viewer.cam.azimuth = 45

            while viewer.is_running():
                # TODO: add control loop logic
                executor.spin()
                time.sleep(0.01)

    except Exception as e:
        print(f"Simulator script error: {e}")

    finally:
        if 'mojoco_sim' in locals():
            mujoco_sim.shutdown()
        rclpy.shutdown()
