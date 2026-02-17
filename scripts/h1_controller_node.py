#!/usr/bin/env python3
# h1_controller/h1_controller_node.py

import rclpy
from rclpy.node import Node
from rclpy.executors import MultiThreadedExecutor
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy

import math
import numpy as np
import mujoco
import threading
import time
from collections import deque
from scipy.spatial.transform import Rotation as R
import torch
import sys
import os

# Добавляем путь для импорта локальных модулей
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# ROS 2 сообщения
from sensor_msgs.msg import JointState, Imu
from geometry_msgs.msg import Twist, Pose, Vector3
from nav_msgs.msg import Odometry
from std_msgs.msg import Float32MultiArray, Header, Empty

# Локальные модули (теперь абсолютные импорты)
from mujoco_interface import MuJoCoInterface
from policy_loader import PolicyLoader

class H1ControllerNode(Node):
    """
    Основной узел для управления роботом H1 в MuJoCo
    """
    
    def __init__(self):
        super().__init__('h1_controller')
        
        # Параметры ROS 2
        self.declare_parameters(
            namespace='',
            parameters=[
                ('model_path', 'models/h1/world.xml'),
                ('policy_path', 'models/trot.pt'),
                ('sim_duration', 60.0),
                ('dt', 0.001),
                ('decimation', 20),
                ('control_frequency', 100.0),
                ('render', True),
                ('use_policy', True),
                ('default_dof_pos', [0.0, 0.08, 0.56, -1.12, -0.57, 0.0, -0.08, -0.56, 1.12, 0.57]),
                ('spd_x', 0.0),
                ('spd_y', 0.0),
                ('spd_yaw', 0.0),
                ('kp', [13, 15, 15, 15, 13, 13, 15, 15, 15, 13]),
                ('kd', [0.3, 0.65, 0.65, 0.65, 0.3, 0.3, 0.65, 0.65, 0.65, 0.3]),
                ('tau_limit', 20.0)
            ]
        )
        
        # Получаем параметры
        self.model_path = self.get_parameter('model_path').value
        self.policy_path = self.get_parameter('policy_path').value
        self.dt = self.get_parameter('dt').value
        self.decimation = self.get_parameter('decimation').value
        self.control_frequency = self.get_parameter('control_frequency').value
        self.render = self.get_parameter('render').value
        self.use_policy = self.get_parameter('use_policy').value
        self.default_dof_pos = np.array(self.get_parameter('default_dof_pos').value, dtype=np.float32)
        self.spd_x = self.get_parameter('spd_x').value
        self.spd_y = self.get_parameter('spd_y').value
        self.spd_yaw = self.get_parameter('spd_yaw').value
        self.kp = np.array(self.get_parameter('kp').value, dtype=np.float32)
        self.kd = np.array(self.get_parameter('kd').value, dtype=np.float32)
        self.tau_limit = self.get_parameter('tau_limit').value
        
        # QoS профиль
        qos_profile = QoSProfile(
            depth=10,
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST
        )
        
        # Инициализация
        self.get_logger().info("🔄 Инициализация H1 Controller...")
        
        # Загрузка модели политики
        self.policy_loader = PolicyLoader(self.policy_path)
        self.policy = self.policy_loader.load_policy() if self.use_policy else None
        
        # Инициализация MuJoCo
        self.mujoco_interface = MuJoCoInterface(
            model_path=self.model_path,
            dt=self.dt,
            decimation=self.decimation,
            default_dof_pos=self.default_dof_pos,
            kp=self.kp,
            kd=self.kd,
            tau_limit=self.tau_limit,
            policy=self.policy
        )
        
        # Инициализация команд
        self.cmd_vel = Twist()
        self.last_cmd_time = time.time()
        
        # Publisher'ы
        self.joint_state_pub = self.create_publisher(
            JointState,
            '/h1/joint_states',
            qos_profile
        )
        
        self.imu_pub = self.create_publisher(
            Imu,
            '/h1/imu',
            qos_profile
        )
        
        self.odom_pub = self.create_publisher(
            Odometry,
            '/h1/odom',
            qos_profile
        )
        
        self.action_pub = self.create_publisher(
            Float32MultiArray,
            '/h1/actions',
            qos_profile
        )
        
        self.observation_pub = self.create_publisher(
            Float32MultiArray,
            '/h1/observations',
            qos_profile
        )
        
        # Subscriber'ы
        self.cmd_vel_sub = self.create_subscription(
            Twist,
            '/h1/cmd_vel',
            self.cmd_vel_callback,
            10
        )
        
        self.reset_sub = self.create_subscription(
            Empty,
            '/h1/reset',
            self.reset_callback,
            10
        )
        
        # Таймеры
        control_period = 1.0 / self.control_frequency
        self.control_timer = self.create_timer(control_period, self.control_callback)
        self.publish_timer = self.create_timer(0.01, self.publish_state_callback)  # 100 Hz
        
        # Состояние
        self.is_running = True
        self.sim_thread = None
        
        # Поток для симуляции
        self.start_simulation_thread()
        
        self.get_logger().info("✅ H1 Controller Node запущен")
        self.get_logger().info(f"   - Модель: {self.model_path}")
        self.get_logger().info(f"   - Политика: {self.policy_path}")
    
    def start_simulation_thread(self):
        """Запуск потока симуляции MuJoCo"""
        self.sim_thread = threading.Thread(target=self.simulation_loop, daemon=True)
        self.sim_thread.start()
    
    def simulation_loop(self):
        """Основной цикл симуляции MuJoCo"""
        self.get_logger().info("🚀 Запуск потока симуляции MuJoCo")
        
        try:
            if self.render:
                import mujoco.viewer
                with mujoco.viewer.launch_passive(
                    self.mujoco_interface.model,
                    self.mujoco_interface.data
                ) as viewer:
                    self._run_simulation_with_viewer(viewer)
            else:
                self._run_simulation_headless()
                
        except Exception as e:
            self.get_logger().error(f"❌ Ошибка в симуляции: {e}")
    
    def _run_simulation_with_viewer(self, viewer):
        """Запуск симуляции с viewer"""
        while self.is_running and viewer.is_running():
            step_start = time.time()
            
            # Обновление команд
            self.mujoco_interface.update_commands(
                self.spd_x, 
                self.spd_y, 
                self.spd_yaw
            )
            
            # Шаг симуляции
            self.mujoco_interface.step()
            
            # Обновление viewer
            viewer.sync()
            
            # Контроль времени
            elapsed = time.time() - step_start
            sleep_time = self.dt - elapsed
            if sleep_time > 0:
                time.sleep(sleep_time)
    
    def _run_simulation_headless(self):
        """Запуск симуляции без viewer"""
        while self.is_running:
            step_start = time.time()
            
            # Обновление команд
            self.mujoco_interface.update_commands(
                self.spd_x, 
                self.spd_y, 
                self.spd_yaw
            )
            
            # Шаг симуляции
            self.mujoco_interface.step()
            
            # Контроль времени
            elapsed = time.time() - step_start
            sleep_time = self.dt - elapsed
            if sleep_time > 0:
                time.sleep(sleep_time)
    
    def cmd_vel_callback(self, msg):
        """Обработка команд скорости"""
        self.cmd_vel = msg
        self.spd_x = msg.linear.x
        self.spd_y = msg.linear.y
        self.spd_yaw = msg.angular.z
        self.last_cmd_time = time.time()
        
        self.get_logger().debug(
            f"📥 Команда: vx={self.spd_x:.2f}, vy={self.spd_y:.2f}, ω={self.spd_yaw:.2f}"
        )
    
    def reset_callback(self, msg):
        """Сброс симуляции"""
        self.get_logger().info("🔄 Сброс симуляции")
        self.mujoco_interface.reset()
    
    def control_callback(self):
        """Контрольный цикл"""
        try:
            # Получаем текущее состояние
            obs = self.mujoco_interface.get_observation()
            
            # Если используем политику, вычисляем действие
            if self.use_policy and self.policy is not None:
                action = self.mujoco_interface.compute_action(obs)
                
                # Публикуем действие
                action_msg = Float32MultiArray()
                action_msg.data = action.tolist()
                self.action_pub.publish(action_msg)
            
            # Публикуем наблюдения
            obs_msg = Float32MultiArray()
            obs_msg.data = obs.flatten().tolist()
            self.observation_pub.publish(obs_msg)
            
        except Exception as e:
            self.get_logger().error(f"❌ Ошибка в control_callback: {e}")
    
    def publish_state_callback(self):
        """Публикация состояния робота"""
        try:
            # Публикация состояния суставов
            self.publish_joint_state()
            
            # Публикация IMU данных
            self.publish_imu()
            
            # Публикация одометрии
            self.publish_odometry()
            
        except Exception as e:
            self.get_logger().error(f"❌ Ошибка публикации состояния: {e}")
    
    def publish_joint_state(self):
        """Публикация состояния суставов"""
        joint_state = self.mujoco_interface.get_joint_state()
        
        msg = JointState()
        msg.header = Header()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = "base_link"
        
        # Имена суставов
        joint_names = [
            'FL_hip', 'FL_thigh', 'FL_calf', 'FL_foot',
            'FR_hip', 'FR_thigh', 'FR_calf', 'FR_foot', 
            'RL_hip', 'RL_thigh', 'RL_calf', 'RL_foot',
            'RR_hip', 'RR_thigh', 'RR_calf', 'RR_foot'
        ]
        
        msg.name = joint_names[:10]
        msg.position = joint_state['position'][:10].tolist()
        msg.velocity = joint_state['velocity'][:10].tolist()
        msg.effort = joint_state['effort'][:10].tolist()
        
        self.joint_state_pub.publish(msg)
    
    def publish_imu(self):
        """Публикация IMU данных"""
        imu_data = self.mujoco_interface.get_imu_data()
        
        msg = Imu()
        msg.header = Header()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = "base_link"
        
        msg.orientation.x = imu_data['quat'][0]
        msg.orientation.y = imu_data['quat'][1]
        msg.orientation.z = imu_data['quat'][2]
        msg.orientation.w = imu_data['quat'][3]
        
        msg.angular_velocity.x = imu_data['angular_vel'][0]
        msg.angular_velocity.y = imu_data['angular_vel'][1]
        msg.angular_velocity.z = imu_data['angular_vel'][2]
        
        msg.linear_acceleration.x = imu_data['linear_acc'][0]
        msg.linear_acceleration.y = imu_data['linear_acc'][1]
        msg.linear_acceleration.z = imu_data['linear_acc'][2]
        
        self.imu_pub.publish(msg)
    
    def publish_odometry(self):
        """Публикация одометрии"""
        odom_data = self.mujoco_interface.get_odometry()
        
        msg = Odometry()
        msg.header = Header()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = "odom"
        msg.child_frame_id = "base_link"
        
        msg.pose.pose.position.x = odom_data['position'][0]
        msg.pose.pose.position.y = odom_data['position'][1]
        msg.pose.pose.position.z = odom_data['position'][2]
        
        msg.pose.pose.orientation.x = odom_data['orientation'][0]
        msg.pose.pose.orientation.y = odom_data['orientation'][1]
        msg.pose.pose.orientation.z = odom_data['orientation'][2]
        msg.pose.pose.orientation.w = odom_data['orientation'][3]
        
        msg.twist.twist.linear.x = odom_data['linear_vel'][0]
        msg.twist.twist.linear.y = odom_data['linear_vel'][1]
        msg.twist.twist.linear.z = odom_data['linear_vel'][2]
        
        msg.twist.twist.angular.x = odom_data['angular_vel'][0]
        msg.twist.twist.angular.y = odom_data['angular_vel'][1]
        msg.twist.twist.angular.z = odom_data['angular_vel'][2]
        
        self.odom_pub.publish(msg)
    
    def destroy_node(self):
        """Корректное завершение работы"""
        self.get_logger().info("🛑 Завершение работы H1 Controller")
        self.is_running = False
        
        if self.sim_thread:
            self.sim_thread.join(timeout=2.0)
        
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    
    try:
        node = H1ControllerNode()
        executor = MultiThreadedExecutor()
        executor.add_node(node)
        executor.spin()
        
    except KeyboardInterrupt:
        pass
    except Exception as e:
        print(f"❌ Ошибка: {e}")
    finally:
        if 'node' in locals():
            node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()