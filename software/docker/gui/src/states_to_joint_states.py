#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from tinker_msgs.msg import LowState, LowCmd


class StatesToCommandsBridge(Node):
    def __init__(self) -> None:
        super().__init__('states_to_commands_bridge')
        self._pub = self.create_publisher(LowCmd, '/low_level_command', 10)
        self._sub = self.create_subscription(LowState, '/low_level_state', self._on_states, 10)
        self.get_logger().info('Bridge started: /low_level_state -> /low_level_command')

    def _on_states(self, msg: LowState) -> None:
        # Create LowCmd from LowState (for simulation/echo purposes)
        cmd_msg = LowCmd()
        cmd_msg.timestamp_state = msg.timestamp_state

        # Create motor commands from motor states
        cmd_msg.motor_cmd = []
        for motor_state in msg.motor_state:
            motor_cmd = MotorCmd()
            motor_cmd.position = motor_state.position
            motor_cmd.velocity = motor_state.velocity
            motor_cmd.torque = motor_state.torque
            motor_cmd.kp = 0.0  # Default values
            motor_cmd.kd = 0.0  # Default values
            cmd_msg.motor_cmd.append(motor_cmd)

        self._pub.publish(cmd_msg)


def main(args=None):
    rclpy.init(args=args)
    node = StatesToCommandsBridge()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
