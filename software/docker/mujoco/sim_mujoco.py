import mujoco
import mujoco.viewer
import os
import time
import rclpy
import numpy as np
from rclpy.node import Node
from rclpy.executors import MultiThreadedExecutor
from tinker_msgs.msg import LowState, LowCmd, MotorCmd
from rosgraph_msgs.msg import Clock # Для передачи шага моделирования в инференс

class MujocoSim(Node):
    def __init__(self, xml_path):
        super().__init__("mujoco_sim")

        self.rpy = np.zeros(3)
        self.imu_quat = np.zeros(4)
        self.ang_vel = np.zeros(3)
        self.accel = np.zeros(3)
        self.commands = np.zeros(3)
        self.positions = np.zeros(10)
        self.velocities = np.zeros(10)

        self.actions = np.zeros(10)
        self.init_ctrl = np.array([0.0, 0.08, 0.56, -1.12, -0.57, 0.0, -0.08, -0.56, 1.12, 0.57])
        self.ctrl = self.init_ctrl.copy()

        self.IS_ACTIONS = False
        
        self.model = mujoco.MjModel.from_xml_path(xml_path)
        self.data = mujoco.MjData(self.model)

        self.ctrl_range = self.model.actuator_ctrlrange.copy()

        self.cmd_subscriber = self.create_subscription(
            LowCmd,
            '/low_level_cmd',
            self.cmd_callback,
            10
        )

        self.state_publisher = self.create_publisher(
            LowState,
            '/low_level_state',
            10
        )

        # В инференесе не создаётся листенер, он автоматически подписывается на симуляционное время
        self.clock_publisher = self.create_publisher(Clock, '/clock', 10)


    def cmd_callback(self, msg: LowCmd):
        cmd = [msg.motor_cmd[i].position for i in range(10)]
        for i in range(5):
            self.actions[2 * i]     = cmd[i]
            self.actions[2 * i + 1] = cmd[i + 5]
        self.IS_ACTIONS = True


    def publish_state(self):
        msg = LowState()
        sim_time = float(self.data.time)
        msg.timestamp_state.sec = int(sim_time)
        msg.timestamp_state.nanosec = int((sim_time % 1.0) * 1e9)
        msg.imu_state.gyroscope = self.ang_vel
        msg.imu_state.accelerometer = self.accel
        msg.imu_state.rpy = self.rpy
        msg.imu_state.quaternion = self.imu_quat

        for i in range(10):
            msg.motor_state[i].position = float(self.positions[i])
            msg.motor_state[i].velocity = float(self.velocities[i])

        self.state_publisher.publish(msg)


    def control_loop(self):
        try:

            if not self.IS_ACTIONS:
                # Manual mode: viewer sliders drive data.ctrl directly — don't overwrite.
                # Sync self.ctrl so there's no jump when policy takes over.
                self.ctrl = self.data.ctrl.copy()
            else:
                self.ctrl = np.clip(self.actions, self.ctrl_range[:, 0], self.ctrl_range[:, 1])
                self.data.ctrl[:] = self.ctrl

            if self.data.qpos[2] < -0.35:
                mujoco.mj_resetData(self.model, self.data)
                self.data.qpos[7:17] = self.init_ctrl.copy()
                self.data.ctrl[:] = self.init_ctrl.copy()
                self.ctrl = self.init_ctrl.copy()
                self.actions = self.init_ctrl.copy()
                mujoco.mj_forward(self.model, self.data)
            mujoco.mj_step(self.model, self.data)

            # Observations
            self.imu_quat = self.data.sensor('orientation').data.copy()  # [w, x, y, z]
            w, x, y, z = self.imu_quat
            self.ang_vel = self.data.sensor('angular-velocity').data.copy()
            self.accel   = self.data.sensor('linear-acceleration').data.copy()

            self.rpy = np.array([
                np.arctan2(2 * (w * x + y * z), 1 - 2 * (x**2 + y**2)),
                np.arcsin(np.clip(2 * (w * y - z * x), -1.0, 1.0)),
                np.arctan2(2 * (w * z + x * y), 1 - 2 * (y**2 + z**2))
            ], dtype=np.float32)
       
            self.positions = self.data.qpos[7:17]
            self.velocities = self.data.qvel[6:16]
            
            self.publish_state()
            self.publish_clock()

        except Exception as e:
            print(f'sim2sim control loop error: {e}')


    def publish_clock(self):
        sim_time = float(self.data.time)
        msg = Clock()
        msg.clock.sec = int(sim_time)
        msg.clock.nanosec = int((sim_time % 1.0) * 1e9)
        self.clock_publisher.publish(msg)

    def shutdown(self):
        self.destroy_node()


if __name__ == "__main__":
    rclpy.init()
    try:
        current_dir = os.path.dirname(os.path.abspath(__file__))
        xml_path = os.path.join(current_dir, "xml", "world.xml")

        # model = mujoco.MjModel.from_xml_path(xml_path)
        # data = mujoco.MjData(model)

        mujoco_sim = MujocoSim(xml_path)
        mujoco_sim.data.qpos[7:17] = mujoco_sim.init_ctrl.copy()
        mujoco_sim.data.ctrl[:] = mujoco_sim.init_ctrl.copy()
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

        # sim_dt = float(mujoco_sim.model.opt.timestep)

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
