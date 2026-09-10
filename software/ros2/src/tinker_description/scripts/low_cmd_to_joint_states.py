#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState
from tinker_msgs.msg import LowCmd

JOINT_NAMES_1 = [
    'J_L0', 'J_L1', 'J_L2', 'J_L3', 'J_L4_ankle',
    'J_R0', 'J_R1', 'J_R2', 'J_R3', 'J_R4_ankle',
]
JOINT_NAMES_2 = [
    'J_L0', 'J_R0', 'J_L1', 'J_R1', 'J_L2',
    'J_R2', 'J_L3', 'J_R3', 'J_L4_ankle', 'J_R4_ankle',
]


class LowCmdBridge(Node):
    def __init__(self):
        super().__init__('low_cmd_to_joint_states')
        # self._positions = [0.0, 0.08, 0.56, -1.12, -0.57, 0.0, -0.08, -0.56, 1.12, 0.57]
        self._positions = [0.0, 0.0, 0.08, -0.08, 0.56, -0.56, -1.12, 1.12, -0.57, 0.57]
        self.pub = self.create_publisher(JointState, '/joint_states', 10)
        self.sub = self.create_subscription(LowCmd, '/low_level_command', self.on_cmd, 10)
        self.create_timer(0.05, self.publish)  # 20 Hz

    def on_cmd(self, msg: LowCmd):
        self._positions = [msg.motor_cmd[i].position for i in range(len(JOINT_NAMES_2))]
        # for i in range(len(JOINT_NAMES_2)):
        #     if i == 0:
        #         self._positions[i] = - msg.motor_cmd[i].position
        #     elif i == 1:
        #         self._positions[i] = msg.motor_cmd[i].position

    def publish(self):
        js = JointState()
        js.header.stamp = self.get_clock().now().to_msg()
        js.name = JOINT_NAMES_2
        js.position = self._positions
        self.pub.publish(js)


def main():
    rclpy.init()
    rclpy.spin(LowCmdBridge())
    rclpy.shutdown()


if __name__ == '__main__':
    main()
