#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
import math
import threading
from hardware_msg.msg import MotorsCommands

class MotorCommander(Node):
    def __init__(self):
        super().__init__('motor_commander')
        
        # Создаем издатель для команд мотора
        self.motor_pub = self.create_publisher(
            MotorsCommands, 
            'motors/commands', 
            10
        )
        
        # Количество двигателей
        self.num_motors = 10
        
        # Таймер для отправки команд каждые 5 секунд
        self.timer = self.create_timer(5.0, self.send_motor_command)
        
        # Флаг для определения направления вращения
        self.positive_direction = True
        
        # Угол поворота в градусах (60 градусов)
        self.target_angle_degrees = 60.0
        
        self.get_logger().info('Motor Commander started - will send +/-60 degree commands every 5 seconds')

    def send_motor_command(self):
        msg = MotorsCommands()
        
        if self.positive_direction:
            # Поворот на +60 градусов
            msg.target_pos = self.target_angle_degrees
            direction_str = "+60 degrees"
        else:
            # Поворот на -60 градусов
            msg.target_pos = -self.target_angle_degrees
            direction_str = "-60 degrees"
        
        # Устанавливаем нулевую скорость и крутящий момент
        msg.target_vel = 0.0
        msg.target_trq = 0.0
        
        # Публикуем сообщение
        self.motor_pub.publish(msg)
        
        # Меняем направление для следующей команды
        self.positive_direction = not self.positive_direction
        
        self.get_logger().info(f'Sent motor command: {direction_str} ({msg.target_pos:.1f} degrees)')

def main(args=None):
    rclpy.init(args=args)
    
    motor_commander = MotorCommander()
    
    try:
        rclpy.spin(motor_commander)
    except KeyboardInterrupt:
        motor_commander.get_logger().info('Motor Commander shutting down...')
    finally:
        motor_commander.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()