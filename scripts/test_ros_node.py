# test_ros_node.py
#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32MultiArray
import time

class TestSubscriber(Node):
    def __init__(self):
        super().__init__('test_subscriber')
        self.joint_sub = self.create_subscription(
            Float32MultiArray,
            '/h1/joint_states',
            self.joint_callback,
            10
        )
        self.action_sub = self.create_subscription(
            Float32MultiArray, 
            '/h1/rl_actions',
            self.action_callback,
            10
        )
        self.get_logger().info('Test subscriber ready')

    def joint_callback(self, msg):
        self.get_logger().info(f'Received joint states: {len(msg.data)} values')

    def action_callback(self, msg):
        self.get_logger().info(f'Received actions: {len(msg.data)} values')

def main():
    rclpy.init()
    node = TestSubscriber()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()