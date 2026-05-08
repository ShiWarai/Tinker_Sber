#!/usr/bin/env python3
"""Publish synthetic tinker_msgs/LowState on /low_level_state for testing button_control."""

import rclpy
from rclpy.node import Node
from builtin_interfaces.msg import Time

from tinker_msgs.msg import IMUState, LowState, MotorState


class FakeLowStatePublisher(Node):
    def __init__(self):
        super().__init__('fake_low_state_publisher')
        self.pub = self.create_publisher(LowState, '/low_level_state', 10)
        self.timer = self.create_timer(1.0 / 50.0, self.publish_cb)
        self._seq = 0

    def publish_cb(self):
        msg = LowState()
        now = self.get_clock().now()
        ns = now.nanoseconds
        ts = Time(sec=ns // 1_000_000_000, nanosec=ns % 1_000_000_000)

        msg.timestamp_state = ts
        msg.tick = self._seq
        self._seq += 1

        imu = IMUState()
        imu.timestamp_state = ts
        imu.quaternion = [1.0, 0.0, 0.0, 0.0]
        imu.gyroscope = [0.0, 0.0, 0.0]
        imu.accelerometer = [0.0, 0.0, 9.81]
        imu.rpy = [0.0, 0.0, 0.0]
        imu.temperature = 25
        msg.imu_state = imu

        for i in range(10):
            m = MotorState()
            m.timestamp_state = ts
            m.position = 0.0
            m.velocity = 0.0
            m.torque = 0.0
            m.temperature_mosfet = 30
            m.temperature_rotor = 35
            m.error = 0
            msg.motor_state[i] = m

        self.pub.publish(msg)


def main():
    rclpy.init()
    node = FakeLowStatePublisher()
    node.get_logger().info('Publishing fake LowState on /low_level_state at 50 Hz')
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
