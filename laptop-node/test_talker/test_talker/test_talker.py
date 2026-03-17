#!/usr/bin/env python3
import math
import rclpy
from rclpy.node import Node
from tinker_msgs.msg import LowCmd, MotorCmd, LowState, ControlCmd

class SinusoidalTrajectoryTalker(Node):
    def __init__(self):
        super().__init__('sinusoidal_trajectory_talker')

        self.amplitude = 0.5 # амлпитуда синусоиды
        self.frequency = 0.33 # частоты
        self.offset = 0.0 # начальное отклонение от 0

        self.kp_little = 13.0
        self.kd_little = 0.65

        self.kp_big = 15.0
        self.kd_big = 0.65

        self.torque_ff = 0.0
        self.rate_hz = 1000.0
        self.num_motors = 10

        self.low_cmd_pub = self.create_publisher(LowCmd, '/low_level_command', 10)
        self.control_cmd_pub = self.create_publisher(ControlCmd, '/control_command', 10)

        self.lowstate_sub = self.create_subscription(
            LowState, '/low_level_state', self.data_callback, 10
        )

        self.last_state = None
        self.timer = self.create_timer(1.0 / self.rate_hz, self.publish_message)

        self.get_logger().info(
            f"Started sinusoidal trajectory control:\n"
            f"  - Offset: {self.offset}, Amplitude: {self.amplitude}\n"
            f"  - Frequency: {self.frequency} Hz\n"
            f"  - Kp common: {self.kp_little}, Kd common: {self.kd_little}\n"
            f"  - Kp special (motors 1,4,5,9): {self.kp_big}, Kd special: {self.kd_big}\n"
            f"  - Torque FF: {self.torque_ff}\n"
            f"  - Publishing rate: {self.rate_hz} Hz\n"
            f"  - Controlling {self.num_motors} motors"
        )

    def data_callback(self, msg: LowState):
        self.last_state = msg

    def _motor_cmd(self, pos: float, vel: float, kp: float, kd: float) -> MotorCmd:
        m = MotorCmd()
        m.position = float(pos)
        m.velocity = float(vel)
        m.torque = float(self.torque_ff)
        m.kp = float(kp)
        m.kd = float(kd)
        return m

    def publish_message(self):
        t = self.get_clock().now().nanoseconds / 1e9

        motor_cmds = []
        for i in range(self.num_motors):
            if i in [1, 4, 5, 9]:
                kp = self.kp_big
                kd = self.kd_big
            else:
                kp = self.kp_little
                kd = self.kd_little

            pos = self.offset + self.amplitude * math.sin(2.0 * math.pi * self.frequency * t)
            vel = 0

            cmd = self._motor_cmd(pos, vel, kp, kd)
            motor_cmds.append(cmd)

        low_cmd = LowCmd()
        low_cmd.motor_cmd = motor_cmds
        self.low_cmd_pub.publish(low_cmd)

        self.get_logger().debug(
            f"Published LowCmd: t={t:.2f}, pos[0]={motor_cmds[0].position:.3f}, "
            f"vel[0]={motor_cmds[0].velocity:.3f}"
        )

        for i in range(self.num_motors):
            ctrl_msg = ControlCmd()
            ctrl_msg.motor_id = i
            ctrl_msg.cmd = 252
            self.control_cmd_pub.publish(ctrl_msg)
            self.get_logger().debug(f"Published ControlCmd: motor_id={i}, cmd=252")

def main():
    rclpy.init()
    node = SinusoidalTrajectoryTalker()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("Shutting down...")
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()