from abc import ABC, abstractmethod
import rclpy
from rclpy.node import Node
# from controller_msg.msg import RobotState
# from controller_msg.msg import IsaacObservation, IsaacAction

class BaseAdapter(ABC):
    def __init__(self, node: Node):
        self.node = node
        self.observation_subscriber= None
        self.control_publisher = None
        
        self.state = None

    @abstractmethod
    def initialize(self):
        pass

    @abstractmethod
    def publish_control(self, control_msg):
        pass
        
    @abstractmethod
    def shutdown(self):
        self.node.destroy_subscription(self.state_subscriber)
        self.node.destroy_publisher(self.control_publisher)

'''
class IsaacAdapter(BaseAdapter):
    def __init__(self, node: Node):
        super().__init__(node)
    
    def initialize(self):
        self.observation_subscriber = self.node.create_subscription(
            IsaacObservation,
            '/isaac/observation',
            self.observation_callback,
            10
        )
        self.control_publisher = self.node.create_publisher(
            IsaacAction,
            '/isaac/action',
            10
        )
        self.node.get_logger().info('Isaac adapter initialized')

    def observation_callback(self, msg: IsaacObservation):
        self.state = msg

    def publish_control(self, action):
        self.control_publisher.publish(action)

    def shutdown(self):
        return super().shutdown()
'''

class MujocoAdapter(BaseAdapter):
    def __init__(self, node: Node):
        super().__init__(node)


class RobotAdapter(BaseAdapter):
    def __init__(self, node: Node):
        super().__init__(node)