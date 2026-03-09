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
    def __init__(self, xml_path, control_decimation=4, max_delta_per_update=0.15):
        super().__init__("mujoco_sim")

        self.rpy = np.zeros(3)
        self.imu_quat = np.zeros(4)
        self.ang_vel = np.zeros(3)
        self.commands = np.zeros(3)
        self.positions = np.zeros(10)
        self.velocities = np.zeros(10)

        self.actions = np.zeros(10)
        self.init_ctrl = np.array([0.0, 0.08, 0.56, -1.12, -0.57, 
                                    0.0, -0.08, -0.56, 1.12, 0.57])
        self.ctrl = self.init_ctrl.copy()

        self.control_decimation = control_decimation
        self.step_idx = 0 
        self.max_delta = max_delta_per_update

        self.IS_ACTIONS = False
        
        self.model = mujoco.MjModel.from_xml_path(xml_path)
        self.data = mujoco.MjData(self.model)

        self.ctrl_range = self.model.actuator_ctrlrange.copy()

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
        # self.actions = np.zeros(10)
        for i in range(10):
            self.actions[i] = msg.motor_cmd[i].position
        # print(self.actions, '\n')
        self.IS_ACTIONS = True


    def publish_state(self):
        msg = LowState()
        msg.imu_state.gyroscope = self.ang_vel
        msg.imu_state.rpy = self.rpy
        msg.imu_state.quaternion = self.imu_quat

        for i in range(5):
            msg.motor_state[2*i].position = float(self.positions[i])
            msg.motor_state[2*i+1].velocity = float(self.velocities[i+5])

        self.state_publisher.publish(msg)


    def control_loop(self):
        try:
            # self.data.qpos[0:3] = [0, 0, -0.15]
            # self.data.qpos[3:7] = [1, 0, 0, 0]
            # self.data.qpos[7:17] = np.array([0.0, 0.08, 0.56, -1.12, -0.57, 
            #                                     0.0, -0.08, -0.56, 1.12, 0.57])
            # self.data.qvel[0:6] = 0

            if self.IS_ACTIONS and self.step_idx == 0:
                # self.ctrl = self.actions.copy()

                desired = np.clip(self.actions, self.ctrl_range[:, 0], self.ctrl_range[:, 1])
                delta = desired - self.ctrl
                delta = np.clip(delta, -self.max_delta, self.max_delta)
                self.ctrl += delta

            if self.data.qpos[2] < -0.35:
                mujoco.mj_resetData(self.model, self.data)
                self.data.qpos[7:17] = self.init_ctrl.copy()
                mujoco.mj_forward(self.model, self.data)
                self.actions = self.init_ctrl.copy()
                self.ctrl = self.init_ctrl.copy()

            self.data.ctrl[:] = self.ctrl
            mujoco.mj_step(self.model, self.data)
            print(self.step_idx, ': ', self.ctrl, '\n')

            ### Collect observations ###
            self.ang_vel = self.data.qvel[3:6]
            self.imu_quat = self.data.qpos[3:7]

            w, x, y, z = self.imu_quat
            self.rpy = np.array([
                np.arctan2(2 * (w * x + y * z), 1 - 2 * (x**2 + y**2)),
                np.arcsin(2 * (w * y - z * x)),
                np.arctan2(2 * (w * z + x * y), 1 - 2 * (y**2 + z**2))
            ])
            
            self.positions = self.data.qpos[7:17]
            self.velocities = self.data.qvel[6:16]

            self.publish_state()

            self.step_idx = (self.step_idx + 1) % self.control_decimation
            
        except Exception as e:
            print(f'sim2sim control loop error: {e}')


    def shutdown(self):
        self.destroy_node()


if __name__ == "__main__":
    rclpy.init()
    try:
        current_dir = os.path.dirname(os.path.abspath(__file__))
        xml_path = os.path.join(current_dir, "xml", "world.xml")

        # model = mujoco.MjModel.from_xml_path(xml_path)
        # data = mujoco.MjData(model)

        mujoco_sim = MujocoSim(xml_path, control_decimation=4)
        mujoco_sim.data.qpos[7:17] = np.array([0.0, 0.08, 0.56, -1.12, -0.57, 
                                               0.0, -0.08, -0.56, 1.12, 0.57])
        mujoco.mj_forward(mujoco_sim.model, mujoco_sim.data)

        executor = MultiThreadedExecutor()
        executor.add_node(mujoco_sim)

        sim_dt = float(mujoco_sim.model.opt.timestep)

        print('executor started')
        # with mujoco.viewer.launch(mujoco_sim.model, mujoco_sim.data) as viewer:
        viewer = mujoco.viewer.launch_passive(mujoco_sim.model, mujoco_sim.data)

        viewer.cam.lookat[:] = [0, 0, 0]
        viewer.cam.distance = 2.0
        viewer.cam.azimuth = 135

        print('viewer started')

        last_print_time = time.time()

        sim_dt = float(mujoco_sim.model.opt.timestep)

        while viewer.is_running():
            step_start = time.perf_counter()
            mujoco_sim.control_loop()
            executor.spin_once(timeout_sec=0)
            viewer.sync()
            
            elapsed = time.perf_counter() - step_start
            if elapsed < sim_dt:
                time.sleep(sim_dt - elapsed)
                
            # time.sleep(0.01)
            # current_time = time.time()

            # if current_time - last_print_time >= 3.0:
            #     print("Positions:", mujoco_sim.positions[2])
            #     print("Velocities:", mujoco_sim.velocities[2])
            #     print("Ctrl:", mujoco_sim.data.ctrl[2])
            #     last_print_time = current_time

    except Exception as e:
        print(f"Simulator script error: {e}")

    finally:
        if 'mojoco_sim' in locals():
            mujoco_sim.shutdown()
        rclpy.shutdown()
