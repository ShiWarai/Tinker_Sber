import mujoco
import mujoco.viewer
import os
import time
import rclpy
import numpy as np
from rclpy.node import Node
from rclpy.executors import MultiThreadedExecutor
from sensor_msgs.msg import Imu
from tinker_msgs.msg import LowState, LowCmd

# Same topic names as software/ros2/src/motor_control (ros2-node/unified_messages)
LOW_LEVEL_STATE = "/low_level_state"
LOW_LEVEL_COMMAND = "/low_level_command"
IMU_STATE = "/imu_state"


class MujocoSim(Node):
    def __init__(self, xml_path):
        super().__init__("mujoco_sim")

        self.tick = 0
        self.imu_quat = np.array([0.0, 0.0, 0.0, 1.0])
        self.ang_vel = np.zeros(3)
        self.positions = np.zeros(10)
        self.velocities = np.zeros(10)

        self.actions = np.zeros(10)
        self.ctrl = np.zeros(10)

        self.model = mujoco.MjModel.from_xml_path(xml_path)
        self.data = mujoco.MjData(self.model)

        self.cmd_subscriber = self.create_subscription(
            LowCmd,
            LOW_LEVEL_COMMAND,
            self.cmd_callback,
            10,
        )

        self.state_publisher = self.create_publisher(
            LowState,
            LOW_LEVEL_STATE,
            10,
        )
        self.imu_publisher = self.create_publisher(
            Imu,
            IMU_STATE,
            10,
        )

    def cmd_callback(self, msg: LowCmd):
        for i in range(10):
            self.actions[i] = msg.motor_cmd[i].position

    def publish_state(self):
        stamp = self.get_clock().now()

        low_state = LowState()
        low_state.timestamp_state = stamp
        low_state.tick = self.tick
        self.tick += 1

        for i in range(10):
            low_state.motor_state[i].timestamp_state = stamp
            low_state.motor_state[i].position = float(self.positions[i])
            low_state.motor_state[i].velocity = float(self.velocities[i])
            low_state.motor_state[i].torque = 0.0
            low_state.motor_state[i].temperature_mosfet = 0
            low_state.motor_state[i].temperature_rotor = 0
            low_state.motor_state[i].connected = True
            low_state.motor_state[i].enabled = True

        self.state_publisher.publish(low_state)

        # MuJoCo free joint quaternion is w,x,y,z; sensor_msgs uses x,y,z,w
        w, x, y, z = self.imu_quat
        imu_msg = Imu()
        imu_msg.header.stamp = stamp
        imu_msg.header.frame_id = "imu_link"
        imu_msg.orientation.x = float(x)
        imu_msg.orientation.y = float(y)
        imu_msg.orientation.z = float(z)
        imu_msg.orientation.w = float(w)
        imu_msg.angular_velocity.x = float(self.ang_vel[0])
        imu_msg.angular_velocity.y = float(self.ang_vel[1])
        imu_msg.angular_velocity.z = float(self.ang_vel[2])
        self.imu_publisher.publish(imu_msg)

    def control_loop(self):
        try:
            self.data.qpos[0:3] = [0, 0, 0.4]
            self.data.qpos[3:7] = [1, 0, 0, 0]
            self.data.qvel[0:6] = 0

            dt = 0.01
            t_disc = dt * np.floor(time.time() / dt)
            self.ctrl[2] = 0.55 * np.sin(t_disc)
            self.ctrl[7] = 0.55 * np.sin(t_disc + 2)

            self.ctrl = np.clip(self.ctrl, -1.57, 1.57)

            self.data.ctrl[:] = self.ctrl

            mujoco.mj_step(self.model, self.data)

            self.ang_vel = self.data.qvel[3:6]
            self.imu_quat = self.data.qpos[3:7]
            self.positions = self.data.qpos[7:17]
            self.velocities = self.data.qvel[6:16]

            self.publish_state()

        except Exception as e:
            print(f'sim2sim control loop error: {e}')

    def shutdown(self):
        self.destroy_node()


if __name__ == "__main__":
    rclpy.init()
    try:
        current_dir = os.path.dirname(os.path.abspath(__file__))
        xml_path = os.path.join(current_dir, "xml", "world.xml")

        mujoco_sim = MujocoSim(xml_path)

        executor = MultiThreadedExecutor()
        executor.add_node(mujoco_sim)
        print('executor started')
        viewer = mujoco.viewer.launch_passive(mujoco_sim.model, mujoco_sim.data)
        viewer.cam.lookat[:] = [0, 0, 0.5]
        viewer.cam.distance = 2.0
        viewer.cam.azimuth = 45
        print('viewer started')
        last_print_time = time.time()

        while viewer.is_running():
            mujoco_sim.control_loop()
            executor.spin_once(timeout_sec=0)
            viewer.sync()
            current_time = time.time()

            if current_time - last_print_time >= 3.0:
                print("Positions:", mujoco_sim.positions[2])
                print("Velocities:", mujoco_sim.velocities[2])
                print("Ctrl:", mujoco_sim.data.ctrl[2])
                last_print_time = current_time

    except Exception as e:
        print(f"Simulator script error: {e}")

    finally:
        if 'mujoco_sim' in locals():
            mujoco_sim.shutdown()
        rclpy.shutdown()
