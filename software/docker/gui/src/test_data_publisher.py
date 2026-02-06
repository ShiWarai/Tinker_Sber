#!/usr/bin/env python3
import math
import random
from typing import List

import rclpy
from rclpy.node import Node
from builtin_interfaces.msg import Time

# using custom messages
from tinker_msgs.msg import (
    ControlCmd, IMUState, LowCmd, LowState,
    MotorCmd, MotorState, OneMotorCmd
)

MOTOR_COUNT = 10


class FakeStatesPublisher(Node):
    def __init__(self) -> None:
        super().__init__('fake_states_publisher')

        # Low level state publisher
        self.low_state_pub = self.create_publisher(LowState, '/low_level_state', 10)

        # Timer
        self.create_timer(0.10, self.publish_low_state)   # 10 Hz

        self.t = 0.0

    def publish_low_state(self) -> None:
        # Generate smooth pseudo data using sin/cos
        self.t += 0.1

        # Create LowState message
        msg = LowState()

        # Set timestamp
        now = self.get_clock().now()
        msg.timestamp_state = Time(sec=now.nanoseconds // 1000000000,
                                   nanosec=now.nanoseconds % 1000000000)

        msg.tick = int(self.t * 10)

        # Create motor states
        msg.motor_state = []
        for i in range(MOTOR_COUNT):
            motor_state = MotorState()
            motor_state.timestamp_state = msg.timestamp_state
            motor_state.position = 10.0 * math.sin(self.t + i)
            motor_state.velocity = 5.0 * math.cos(self.t * 0.8 + i * 0.3)
            motor_state.torque = 50.0 * math.sin(self.t * 0.5 + i * 0.6)
            motor_state.temperature_mosfet = 25 + random.randint(-5, 5)
            motor_state.temperature_rotor = 30 + random.randint(-5, 5)
            motor_state.error = 1  # ENABLE state
            msg.motor_state.append(motor_state)

        # Create IMU state
        msg.imu_state = IMUState()
        msg.imu_state.timestamp_state = msg.timestamp_state
        msg.imu_state.quaternion = [
            math.cos(self.t * 0.1),
            math.sin(self.t * 0.1) * 0.1,
            math.sin(self.t * 0.2) * 0.1,
            math.sin(self.t * 0.3) * 0.1
        ]
        msg.imu_state.gyroscope = [
            math.sin(self.t * 0.5),
            math.cos(self.t * 0.6),
            math.sin(self.t * 0.7)
        ]
        msg.imu_state.accelerometer = [
            math.sin(self.t * 0.3) * 0.5,
            math.cos(self.t * 0.4) * 0.5,
            9.81 + math.sin(self.t * 0.2) * 0.1
        ]
        msg.imu_state.rpy = [
            math.sin(self.t * 0.2) * 0.5,
            math.cos(self.t * 0.3) * 0.5,
            math.sin(self.t * 0.4)
        ]
        msg.imu_state.temperature = 25 + random.randint(-2, 2)

        self.low_state_pub.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    node = FakeStatesPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
