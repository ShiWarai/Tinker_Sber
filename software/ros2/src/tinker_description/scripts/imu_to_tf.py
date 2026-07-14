#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Quaternion, TransformStamped
from tf2_ros import TransformBroadcaster

# Height of base_link above ground in meters
BASE_HEIGHT = 0.35


class ImuToTf(Node):
    def __init__(self):
        super().__init__('imu_to_tf')
        self.tf_broadcaster = TransformBroadcaster(self)
        self._rotation = (1.0, 0.0, 0.0, 0.0)  # w, x, y, z
        self.sub = self.create_subscription(
            Quaternion, '/imu_orientation', self.on_orientation, 10)
        self.create_timer(0.05, self.publish)  # 20 Hz

    def on_orientation(self, msg: Quaternion):
        self._rotation = (float(msg.w), float(msg.x), float(msg.y), float(msg.z))

    def publish(self):
        t = TransformStamped()
        t.header.stamp = self.get_clock().now().to_msg()
        t.header.frame_id = 'world'
        t.child_frame_id = 'base_link'

        t.transform.translation.x = 0.0
        t.transform.translation.y = 0.0
        t.transform.translation.z = BASE_HEIGHT

        w, x, y, z = self._rotation
        t.transform.rotation.w = w
        t.transform.rotation.x = x
        t.transform.rotation.y = y
        t.transform.rotation.z = z

        self.tf_broadcaster.sendTransform(t)


def main():
    rclpy.init()
    rclpy.spin(ImuToTf())
    rclpy.shutdown()


if __name__ == '__main__':
    main()
