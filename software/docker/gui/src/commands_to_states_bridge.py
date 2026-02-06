#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from tinker_msgs.msg import LowCmd, LowState


class CommandsToStatesBridge(Node):
    def __init__(self) -> None:
        super().__init__('commands_to_states_bridge')
        self._pub = self.create_publisher(LowState, '/low_level_state', 10)
        self._sub = self.create_subscription(LowCmd, '/low_level_command', self._on_cmd, 10)
        self.get_logger().info('Bridge started: /low_level_command -> /low_level_state')

    def _on_cmd(self, msg: LowCmd) -> None:
        # Convert LowCmd to LowState (simplified - in real implementation you'd
        # get actual state from hardware)
        state_msg = LowState()
        state_msg.timestamp_state = msg.timestamp_state
        state_msg.tick = 0

        # Create motor states from commands (simulated)
        state_msg.motor_state = []
        for motor_cmd in msg.motor_cmd:
            motor_state = MotorState()
            motor_state.timestamp_state = msg.timestamp_state
            motor_state.position = motor_cmd.position
            motor_state.velocity = motor_cmd.velocity
            motor_state.torque = motor_cmd.torque
            motor_state.temperature_mosfet = 25  # simulated
            motor_state.temperature_rotor = 30   # simulated
            motor_state.error = 1  # ENABLE state
            state_msg.motor_state.append(motor_state)

        # Create simulated IMU state
        state_msg.imu_state = IMUState()
        state_msg.imu_state.timestamp_state = msg.timestamp_state
        state_msg.imu_state.quaternion = [1.0, 0.0, 0.0, 0.0]  # identity quaternion
        state_msg.imu_state.gyroscope = [0.0, 0.0, 0.0]
        state_msg.imu_state.accelerometer = [0.0, 0.0, 9.81]
        state_msg.imu_state.rpy = [0.0, 0.0, 0.0]
        state_msg.imu_state.temperature = 25

        self._pub.publish(state_msg)


def main(args=None):
    rclpy.init(args=args)
    node = CommandsToStatesBridge()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
