#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from tinker_msgs.msg import LowCmd, MotorCmd
import numpy as np
import time

class ManualControlNode(Node):
    def __init__(self):
        super().__init__('manual_control_node')

        self.cmd_publisher = self.create_publisher(LowCmd, '/tinker_msgs/lowcmd', 10)

        self.timer = self.create_timer(0.01, self.control_loop)

        self.dt = 0.01
        self.kp = 15.0
        self.kd = 0.65

        self.get_logger().info("Manual control node started. Publishing to /tinker_msgs/lowcmd.")

    def control_loop(self):

        msg = LowCmd()
        msg.motor_cmd = [MotorCmd() for _ in range(10)]
        t_disc = self.dt * np.floor(time.time() / self.dt)

        pos_1 = 0.55 * np.sin(t_disc)
        pos_2 = 0.55 * np.sin(t_disc + 2.0)

        actions = np.zeros(10)
        actions[2] = pos_1
        actions[7] = pos_2

        for i in range(10):
            msg.motor_cmd[i].position = float(actions[i])
            msg.motor_cmd[i].velocity = 0.0
            msg.motor_cmd[i].torque = 0.0
            msg.motor_cmd[i].kp = float(self.kp)
            msg.motor_cmd[i].kd = float(self.kd)

        self.cmd_publisher.publish(msg)


def main():
    rclpy.init()
    node = ManualControlNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()