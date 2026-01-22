from abc import ABC, abstractmethod
import rclpy
import torch
import numpy as np
from rclpy.node import Node
from pynput import keyboard

class BaseDevice(ABC):
    def __init__(self, node: Node):
        self.node = node
        self.target_subscriber = None
        self.control_command = None

    @abstractmethod
    def initialize(self):
        pass

    @abstractmethod
    def shutdown(self):
        pass


class KeyboardDevice(BaseDevice):
    def __init__(self, node: Node):
        super().__init__(node)
        self.commands = np.zeros((3))

        self.linear_vel = 0.0
        self.lateral_vel = 0.0
        self.angular_vel = 0.0
        self.heading = 0.0

        self.linear_vel_step = 0.1
        self.lateral_vel_step = 0.1
        self.angular_vel_step = 0.8
        self.max_linear_vel = 1.5
        self.max_lateral_vel = 0.5
        self.max_angular_vel = 2.0
        self.max_heading = 3.14
        self.min_height = 0.12
        self.max_height = 0.2
        self.paused = False
    
    def initialize(self):
        self.keyboard_listener = keyboard.Listener(on_press=self.on_press, on_release=self.on_release)
        
        self.node.get_logger().info('Keyboard device initialized')

    def on_press(self, key):
        try:
            if key.char == 'w':
                self.linear_vel = min(self.linear_vel + self.linear_vel_step, self.max_linear_vel)
            elif key.char == 's':
                self.linear_vel = max(self.linear_vel - self.linear_vel_step, -self.max_linear_vel)
            elif key.char == 'a':
                self.lateral_vel = max(self.lateral_vel - self.lateral_vel_step, -self.max_lateral_vel)
            elif key.char == 'd':
                self.lateral_vel = min(self.lateral_vel + self.lateral_vel_step, self.max_lateral_vel)
            elif key.char == 'q':
                self.angular_vel = min(self.angular_vel + self.angular_vel_step, self.max_angular_vel)
            elif key.char == 'e':
                self.angular_vel = max(self.angular_vel - self.angular_vel_step, -self.max_angular_vel)
            elif key.char == 'r':
                self.height = min(self.height + 0.01, self.max_height)
            elif key.char == 'f':
                self.height = max(self.height - 0.01, self.min_height)
            elif key.char == 'x':
                self.linear_vel = 0.0
                self.lateral_vel = 0.0
                self.angular_vel = 0.0
                self.heading = 0.0
                print("All commands reset to zero")
            elif key.char == ' ':
                self.paused = not self.paused
                print(f"Simulation {'paused' if self.paused else 'resumed'}")
        except AttributeError:
            pass
        self.update_commands()

    def on_release(self, key):
        if key == keyboard.Key.esc:
            return False
        try:
            if key.char in ['w', 's', 'a', 'd', 'q', 'e']:
                self.linear_vel *= 0.9
                self.lateral_vel *= 0.9
                self.angular_vel *= 0.8
                if abs(self.linear_vel) < 0.01:
                    self.linear_vel = 0.0
                if abs(self.lateral_vel) < 0.01:
                    self.lateral_vel = 0.0
                if abs(self.angular_vel) < 0.01:
                    self.angular_vel = 0.0
                self.update_commands()
        except AttributeError:
            pass
        return True

    def update_commands(self):
        self.commands[0] = self.linear_vel
        self.commands[1] = self.lateral_vel
        self.commands[2] = self.angular_vel
        # self.commands[3] = self.heading
        # self.commands[4] = self.height

    def get_commands(self):
        return self.commands
    
    def shutdown(self):
        pass


class GamepadDevice(BaseDevice):
    def __init__(self, node: Node):
        super().__init__(node)

    def initialize(self):
        # self.target_subscriber = self.create_subscription(
        #     TargetCommand,
        #     '/control/target',
        #     10
        # )
        pass

        self.node.get_logger().info('Gamepad device initialized')

    def convert(self):
        pass

    def shutdown(self):
        self.node.destroy_subscription(self.target_subscriber)
        self.shutdown()

