#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from tinker_msgs.msg import LowCmd, MotorCmd, LowState, ControlCmd


class ConstantPositionTalker(Node):
    def __init__(self):
        super().__init__('constant_position_talker')

        # Параметры управления
        self.target_position = 1000.0
        self.target_velocity = 50.0
        self.kp = 0.0
        self.kd = 1.0
        self.torque_ff = 0.0
        self.rate_hz = 50.0
        self.num_motors = 10

        # ROS интерфейсы
        self.low_cmd_pub = self.create_publisher(LowCmd, '/tinker_msgs/lowcmd', 10)
        self.control_cmd_pub = self.create_publisher(ControlCmd, '/tinker_msgs/controlcmd', 10)

        self.lowstate_sub = self.create_subscription(
            LowState, '/tinker_msgs/lowstate', self.data_callback, 10
        )

        self.last_state = None
        self.timer = self.create_timer(1.0 / self.rate_hz, self.publish_message)

        self.get_logger().info(
            f"Started constant position control:\n"
            f"  - Target position: {self.target_position}\n"
            f"  - Target velocity: {self.target_velocity}\n"
            f"  - Kp: {self.kp}, Kd: {self.kd}\n"
            f"  - Torque FF: {self.torque_ff}\n"
            f"  - Publishing rate: {self.rate_hz} Hz\n"
            f"  - Controlling {self.num_motors} motors"
        )

    def data_callback(self, msg: LowState):
        self.last_state = msg

    def _motor_cmd(self, pos: float, vel: float) -> MotorCmd:
        m = MotorCmd()
        m.position = float(pos)
        m.velocity = float(vel)
        m.torque = float(self.torque_ff)
        m.kp = float(self.kp)
        m.kd = float(self.kd)
        return m

    def publish_message(self):
        #  Формируем и публикуем LowCmd 
        motor_cmds = []
        for i in range(self.num_motors):
            cmd = self._motor_cmd(self.target_position, self.target_velocity)
            motor_cmds.append(cmd)

        low_cmd = LowCmd()
        low_cmd.motor_cmd = motor_cmds
        self.low_cmd_pub.publish(low_cmd)

        # Лог LowCmd
        pos_str = ', '.join([f"{cmd.position:.2f}" for cmd in motor_cmds])
        vel_str = ', '.join([f"{cmd.velocity:.2f}" for cmd in motor_cmds])
        self.get_logger().debug(f"Published LowCmd - Pos: [{pos_str}] | Vel: [{vel_str}]")

        #  Публикуем по одному ControlCmd на мотор 
        for i in range(self.num_motors):
            ctrl_msg = ControlCmd()
            ctrl_msg.motor_id = i
            ctrl_msg.cmd = 252  # ENABLE
            self.control_cmd_pub.publish(ctrl_msg)
            self.get_logger().debug(f"Published ControlCmd: motor_id={i}, cmd=252 (ENABLE)")


def main():
    rclpy.init()
    node = ConstantPositionTalker()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("Shutting down...")
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()