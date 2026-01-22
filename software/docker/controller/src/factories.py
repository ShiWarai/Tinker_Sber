from rclpy.node import Node
# from .adapters import BaseAdapter, IsaacAdapter, MujocoAdapter, RobotAdapter
from .devices import BaseDevice, KeyboardDevice, GamepadDevice
'''
class AdapterFactory:
    
    _ADAPTER_MAP = {
        'isaac': IsaacAdapter,
        'mujoco': MujocoAdapter,
        'robot': RobotAdapter
    }

    @staticmethod
    def get_adapter(adapter_type: str, node: Node) -> BaseAdapter:
        adater_class = AdapterFactory._ADAPTER_MAP.get(adapter_type.lower())

        if adater_class is None:
            raise ValueError(f'Unknown adapter (control object) type: {adapter_type}')
        
        return adater_class(node)

'''
class InputDeviceFactory:
    
    _DEVICE_MAP = {
        'keyboard': KeyboardDevice,
        'gamepad': GamepadDevice
    }

    @staticmethod
    def get_device(device_type: str, node: Node) -> BaseDevice:
        device_class = InputDeviceFactory._DEVICE_MAP.get(device_type.lower())

        if device_class is None:
            raise ValueError(f'Unknown input device type: {device_type}')
        
        return device_class(node)