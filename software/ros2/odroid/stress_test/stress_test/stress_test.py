#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from hardware_msg.msg import MotorsCommands, MotorsStates
import time
import random

class StressTestNode(Node):
    def __init__(self):
        super().__init__('stress_node')
        
        self.publisher = self.create_publisher(MotorsCommands, '/motors/comands', 10)
        self.subscription = self.create_subscription(MotorsStates, '/motors/states', self.listener_callback, 10)

        # Временные метки
        self.command_send_time = None
        self.movement_start_time = None
        self.target_reached_time = None

        # Состояние двигателя
        self.current_target_angle = None   # текущий целевой угол
        self.current_position = 0.0        # текущая позиция мотора
        self.previous_position = 0.0       # предыдущая позиция для определения движения
        self.command_send = False          # флаг что команда отправлена
        self.target_reached = False        # флаг достижения цели
        self.is_moving = False             # флаг что мотор движется

        # Параметры
        self.movement_threshold = 1.0     # порог для определения начала движения
        self.target_tolerance = 3.0      # допуск для определения достижения цели
        
        # Для отправки нескольких углов
        self.target_angles = []
        self.current_angle_index = 0
        
        # Запускаем таймер и первую команду
        self.timer = self.create_timer(0.1, self.timer_callback)
        self.get_logger().info('Stress test node started')

    def listener_callback(self, msg):
        self.current_position = msg.current_pos[0]

        if self.command_send == True and not self.target_reached:
            # Если команда отправлена и цель еще не достигнута
            
            # Проверяем начало движения
            if not self.is_moving:
                position_diff = abs(self.current_position - self.previous_position)
                if position_diff > self.movement_threshold:
                    self.movement_start_time = time.time()
                    self.is_moving = True
                    movement_delay = self.movement_start_time - self.command_send_time
                    self.get_logger().info(f'Movement started! Delay: {movement_delay:.4f} seconds')
            
            # Проверяем достижение цели
            if self.is_moving and self.current_target_angle is not None:
                position_error = abs(self.current_position - self.current_target_angle)
                if position_error <= self.target_tolerance:
                    self.target_reached_time = time.time()
                    self.target_reached = True
                    self.is_moving = False
                    
                    # Логируем результаты
                    movement_time = self.target_reached_time - self.movement_start_time
                    total_time = self.target_reached_time - self.command_send_time
                    
                    self.get_logger().info(f'Target reached! Angle: {self.current_target_angle:.3f} rad')
                    self.get_logger().info(f'Movement time: {movement_time:.4f}s, Total time: {total_time:.4f}s')
                    
                    # Готовимся к следующей команде
                    self.command_send = False
        
        # Сохраняем текущую позицию для следующего сравнения
        self.previous_position = self.current_position

    def timer_callback(self):
        # Инициализируем список углов если он пустой
        if not self.target_angles:
            self.target_angles = [random.uniform(-60, 60) for _ in range(1)]
            self.current_angle_index = 0
            self.get_logger().info(f'Generated {len(self.target_angles)} target angles')
        
        # Если предыдущая команда еще выполняется, ждем
        if self.command_send and not self.target_reached:
            self.get_logger().info('Waiting for previous command to complete...')
            return
        
        # Если все углы отправлены, завершаем работу
        if self.current_angle_index >= len(self.target_angles):
            self.get_logger().info('All target angles completed!')
            self.timer.cancel()
            return
        
        # Отправляем следующую команду
        msg = MotorsCommands()
        self.current_target_angle = self.target_angles[self.current_angle_index]
        msg.target_pos[0] = self.current_target_angle  # предполагаем массив
        
        self.command_send_time = time.time()
        self.command_send = True
        self.target_reached = False
        self.is_moving = False
        self.movement_start_time = None
        self.target_reached_time = None
        
        self.publisher.publish(msg)
        self.get_logger().info(f'[{self.current_angle_index + 1}/{len(self.target_angles)}] Command sent: {self.current_target_angle:.3f} rad')
        
        self.current_angle_index += 1

def main(args=None):
    rclpy.init(args=args)
    node = StressTestNode()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()