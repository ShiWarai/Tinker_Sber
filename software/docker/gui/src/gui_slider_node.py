#!/usr/bin/env python3
import sys
from typing import List, Tuple
from PyQt5.QtWidgets import (
    QApplication,
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QGridLayout,
    QSlider,
    QLabel,
    QGroupBox,
    QScrollArea,
    QSpinBox,
    QDoubleSpinBox,
    QCheckBox,
    QPushButton,
)
from PyQt5.QtCore import Qt, QTimer
import rclpy
from rclpy.node import Node
from builtin_interfaces.msg import Time

# Import custom messages from tinker_msgs
from tinker_msgs.msg import ControlCmd, IMUState, LowCmd, LowState, MotorCmd, MotorState, OneMotorCmd

from ament_index_python.packages import get_package_share_directory
import os
import xml.etree.ElementTree as ET
import yaml
import signal
import sys
import math

# Changed to 10 motors
MOTOR_COUNT = 10
JOINT_NAMES_10 = [
    "joint_l_yaw",
    "joint_l_roll",
    "joint_l_pitch",
    "joint_l_knee",
    "joint_l_ankle",
    "joint_r_yaw",
    "joint_r_roll",
    "joint_r_pitch",
    "joint_r_knee",
    "joint_r_ankle",
]

# Default values for motors
DEFAULT_POSITION = 0.0
DEFAULT_VELOCITY = 0.0
DEFAULT_TORQUE = 0.0
DEFAULT_KP = 1.0
DEFAULT_KD = 0.1

class MotorSliderNode(Node):
    def __init__(self, motor_count: int):
        super().__init__('gui_slider_node')
        self.motor_count = motor_count
        
        # Publishers for custom messages
        self.control_cmd_publisher = self.create_publisher(
            ControlCmd, "/control_command", 10
        )
        self.low_cmd_publisher = self.create_publisher(
            LowCmd, "/low_level_command", 10
        )
        self.one_motor_cmd_publisher = self.create_publisher(
            OneMotorCmd, "/single_motor_command", 10
        )
        
        # Subscriber for low level state
        self.low_state_subscriber = self.create_subscription(
            LowState, "/low_level_state", self.low_state_callback, 10
        )
        
        # Store current state
        self.current_low_state = LowState()
        self.motor_states = [MotorState() for _ in range(10)]
        
        self.get_logger().info('Tinker GUI started with custom messages')

    def low_state_callback(self, msg: LowState):
        """Callback for LowState messages"""
        self.current_low_state = msg
        # Convert the fixed array to a list for easier handling
        self.motor_states = list(msg.motor_state)

    def publish_control_command(self, motor_id: int, cmd: int):
        """Publish ControlCmd message"""
        msg = ControlCmd()
        msg.motor_id = motor_id
        msg.cmd = cmd
        self.control_cmd_publisher.publish(msg)

    def publish_low_command(self, motor_commands: List[MotorCmd]):
        """Publish LowCmd message for all motors"""
        msg = LowCmd()
        
        # Set timestamp
        now = self.get_clock().now()
        msg.timestamp_state = Time(sec=now.nanoseconds // 1000000000, 
                                  nanosec=now.nanoseconds % 1000000000)
        
        # Convert list to fixed-size array
        for i, motor_cmd in enumerate(motor_commands):
            if i < len(msg.motor_cmd):
                msg.motor_cmd[i] = motor_cmd
        
        self.low_cmd_publisher.publish(msg)

    def publish_single_motor_command(self, motor_id: int, position: float, velocity: float, 
                                   torque: float, kp: float, kd: float):
        """Publish OneMotorCmd message"""
        msg = OneMotorCmd()
        
        # Set timestamp
        now = self.get_clock().now()
        msg.timestamp_state = Time(sec=now.nanoseconds // 1000000000, 
                                  nanosec=now.nanoseconds % 1000000000)
        
        msg.motor_id = motor_id
        msg.motor_group = 0  # Default group
        msg.position = position
        msg.velocity = velocity
        msg.torque = torque
        msg.kp = kp
        msg.kd = kd
        
        self.one_motor_cmd_publisher.publish(msg)


class SliderWindow(QWidget):
    def __init__(self, ros_node: MotorSliderNode):
        super().__init__()
        self.ros_node = ros_node
        self.setWindowTitle("Tinker RQT - Custom Messages")
        self.resize(1200, 800)

        # Загружаем конфигурацию из YAML файла
        self.config = self._load_config()
        self.motor_count = self.config.get('motors_number', MOTOR_COUNT)  # Use MOTOR_COUNT constant
        
        # Загружаем лимиты позиций из конфига
        self.pos_limits: List[Tuple[float, float]] = self._load_limits_from_config()

        # Контейнер со скроллом
        outer_layout = QVBoxLayout()
        
        # Красная кнопка СТОП в верхней части
        stop_button = QPushButton("EMERGENCY STOP")
        stop_button.setStyleSheet("QPushButton { background-color: red; color: white; font-weight: bold; font-size: 16px; padding: 10px; }")
        stop_button.clicked.connect(self._emergency_stop)
        outer_layout.addWidget(stop_button)
        
        # Create a main horizontal layout for left and right sections
        main_horizontal_layout = QHBoxLayout()
        
        # Left side: Motor controls
        left_widget = QWidget()
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        content = QWidget()
        self.grid = QGridLayout(content)
        self.grid.setHorizontalSpacing(16)
        self.grid.setVerticalSpacing(12)
        scroll.setWidget(content)
        left_widget.setLayout(QVBoxLayout())
        left_widget.layout().addWidget(scroll)
        
        # Right side: Global parameters (KP, KD) and System Status
        right_widget = QWidget()
        right_layout = QVBoxLayout(right_widget)
        right_layout.setSpacing(20)
        
        # Global Gains Group (KP, KD)
        gains_group = QGroupBox("Global Gains")
        gains_layout = QGridLayout(gains_group)
        
        # KP control
        gains_layout.addWidget(QLabel("KP:"), 0, 0)
        self.global_kp_spinbox = QDoubleSpinBox()
        self.global_kp_spinbox.setMinimum(0.0)
        self.global_kp_spinbox.setMaximum(100.0)
        self.global_kp_spinbox.setDecimals(3)
        self.global_kp_spinbox.setValue(DEFAULT_KP)
        self.global_kp_spinbox.valueChanged.connect(self._on_global_gains_changed)
        gains_layout.addWidget(self.global_kp_spinbox, 0, 1)
        
        # KD control
        gains_layout.addWidget(QLabel("KD:"), 1, 0)
        self.global_kd_spinbox = QDoubleSpinBox()
        self.global_kd_spinbox.setMinimum(0.0)
        self.global_kd_spinbox.setMaximum(10.0)
        self.global_kd_spinbox.setDecimals(3)
        self.global_kd_spinbox.setValue(DEFAULT_KD)
        self.global_kd_spinbox.valueChanged.connect(self._on_global_gains_changed)
        gains_layout.addWidget(self.global_kd_spinbox, 1, 1)
        
        # Apply Gains button
        self.apply_gains_btn = QPushButton("Apply Gains to All Motors")
        self.apply_gains_btn.clicked.connect(self._apply_global_gains_to_all)
        gains_layout.addWidget(self.apply_gains_btn, 2, 0, 1, 2)
        
        right_layout.addWidget(gains_group)
        
        # System Status Group
        status_group = QGroupBox("System Status")
        status_layout = QVBoxLayout(status_group)
        
        self.imu_status_label = QLabel("IMU: Quaternion=[-, -, -, -] | RPY=[-, -, -]°")
        self.tick_label = QLabel("Tick: -")
        self.timestamp_label = QLabel("Last Update: -")
        
        status_layout.addWidget(self.imu_status_label)
        status_layout.addWidget(self.tick_label)
        status_layout.addWidget(self.timestamp_label)
        
        right_layout.addWidget(status_group)
        
        # Add stretch to push everything to top
        right_layout.addStretch()
        
        # Add left and right widgets to main layout
        main_horizontal_layout.addWidget(left_widget, 3)  # 3 parts width for left
        main_horizontal_layout.addWidget(right_widget, 1)  # 1 part width for right
        
        outer_layout.addLayout(main_horizontal_layout)
        self.setLayout(outer_layout)

        # Хранилища UI-элементов по моторам
        self.motor_cmd_spinboxes: List[List[QDoubleSpinBox]] = []
        self.motor_cmd_data_labels: List[List[QLabel]] = []
        
        # Блок Control Commands в первой строке левой части
        control_group = QGroupBox("Control Commands")
        control_layout = QGridLayout(control_group)
        
        # Motor ID selection
        control_layout.addWidget(QLabel("Motor ID:"), 0, 0)
        self.control_motor_id = QSpinBox()
        self.control_motor_id.setRange(0, self.motor_count-1)
        control_layout.addWidget(self.control_motor_id, 0, 1)
        
        # Control command buttons
        self.enable_btn = QPushButton("ENABLE")
        self.disable_btn = QPushButton("DISABLE")
        self.zero_btn = QPushButton("SET ZERO")
        self.clear_error_btn = QPushButton("CLEAR ERROR")
        self.send_all_btn = QPushButton("SEND ALL MOTORS")  
        
        self.enable_btn.clicked.connect(lambda: self._send_control_cmd(252))
        self.disable_btn.clicked.connect(lambda: self._send_control_cmd(253))
        self.zero_btn.clicked.connect(lambda: self._send_control_cmd(254))
        self.clear_error_btn.clicked.connect(lambda: self._send_control_cmd(251))
        self.send_all_btn.clicked.connect(self._send_all_motors_cmd)
        
        # First row: 4 buttons
        control_layout.addWidget(self.enable_btn, 1, 0)
        control_layout.addWidget(self.disable_btn, 1, 1)
        control_layout.addWidget(self.zero_btn, 1, 2)
        control_layout.addWidget(self.clear_error_btn, 1, 3)
        # Second row: SEND ALL MOTORS spans all 4 columns
        control_layout.addWidget(self.send_all_btn, 2, 0, 1, 4)
        
        self.grid.addWidget(control_group, 0, 0, 1, 2)

        # Создаём группы моторов и раскладываем по колонкам, начиная со 2-й строки
        columns = 2
        for motor_idx in range(self.motor_count):
            group = self._build_motor_group(motor_idx)
            row = (motor_idx // columns) + 1
            col = motor_idx % columns
            self.grid.addWidget(group, row, col)

        # Таймер
        self.display_timer = QTimer(self)
        self.display_timer.timeout.connect(self._on_timer)
        self.display_timer.start(100)

        self.init_ui()

    def init_ui(self):
        """Initialize the UI - this method is called after all widgets are created"""
        pass

    def _load_config(self) -> dict:
        """Загрузить конфигурацию из YAML файла"""
        try:
            pkg_share = get_package_share_directory("tinker_gui")
            config_path = os.path.join(pkg_share, "config", "gui_node.yaml")
            with open(config_path, 'r') as file:
                config = yaml.safe_load(file)
            return config
        except Exception as e:
            print(f"Ошибка загрузки конфига: {e}")
            return {"motors_number": MOTOR_COUNT, "motors": {}}

    def _load_limits_from_config(self) -> List[Tuple[float, float]]:
        """Загрузить лимиты позиций из конфига"""
        limits = []
        motors_config = self.config.get('motors', {})
        
        for i in range(self.motor_count):
            motor_key = f"motor_{i+1}"
            motor_config = motors_config.get(motor_key, {})
            pos_limits = motor_config.get('position_limits', {})
            
            min_pos = pos_limits.get('min', -1.57)
            max_pos = pos_limits.get('max', 1.57)
            limits.append((float(min_pos), float(max_pos)))
        
        return limits

    def _build_motor_group(self, motor_index: int) -> QGroupBox:
        group = QGroupBox(f"Motor {motor_index}")
        group_layout = QVBoxLayout(group)

        # Блок команд (position, velocity, torque only - KP and KD removed)
        cmd_box = QGroupBox("Motor Commands")
        cmd_grid = QGridLayout(cmd_box)
        
        cmd_names = ["Position", "Velocity", "Torque"]  # Removed KP and KD
        default_values = [DEFAULT_POSITION, DEFAULT_VELOCITY, DEFAULT_TORQUE]
        cmd_spinboxes: List[QDoubleSpinBox] = []
        cmd_data_labels: List[QLabel] = []
        
        for i, name in enumerate(cmd_names):
            label = QLabel(f"{name}:")
            spinbox = QDoubleSpinBox()
            
            if name == "Position":
                lower, upper = self.pos_limits[motor_index]
                spinbox.setMinimum(lower)
                spinbox.setMaximum(upper)
                spinbox.setDecimals(3)
            elif name == "Velocity":
                spinbox.setMinimum(-50.0)
                spinbox.setMaximum(50.0)
                spinbox.setDecimals(3)
            elif name == "Torque":
                spinbox.setMinimum(-10.0)
                spinbox.setMaximum(10.0)
                spinbox.setDecimals(3)
                
            spinbox.setValue(default_values[i])  # Set to default value
            
            data_lbl = QLabel("current: 0.0")
            data_lbl.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
            
            cmd_grid.addWidget(label, i, 0)
            cmd_grid.addWidget(spinbox, i, 1)
            cmd_grid.addWidget(data_lbl, i, 2)
            cmd_spinboxes.append(spinbox)
            cmd_data_labels.append(data_lbl)

        # Кнопки отправки команд для одного мотора
        btn_layout = QHBoxLayout()
        send_single_btn = QPushButton("SEND")
        motor_default_btn = QPushButton("SET DEFAULT")
        
        send_single_btn.clicked.connect(lambda _, m=motor_index: self._send_single_motor_cmd(m))
        motor_default_btn.clicked.connect(lambda _, m=motor_index: self._individual_set_default(m))
        
        btn_layout.addWidget(send_single_btn)
        btn_layout.addWidget(motor_default_btn)
        
        cmd_grid.addLayout(btn_layout, len(cmd_names), 0, 1, 3)
        
        self.motor_cmd_spinboxes.append(cmd_spinboxes)
        self.motor_cmd_data_labels.append(cmd_data_labels)
        group_layout.addWidget(cmd_box)

        return group

    def _on_global_gains_changed(self):
        """When global KP/KD values change"""
        kp = self.global_kp_spinbox.value()
        kd = self.global_kd_spinbox.value()
        print(f"Global gains changed: KP={kp:.3f}, KD={kd:.3f}")

    def _apply_global_gains_to_all(self):
        """Apply global KP/KD values to all motors"""
        kp = self.global_kp_spinbox.value()
        kd = self.global_kd_spinbox.value()
        
        print(f"Applying global gains to all motors: KP={kp:.3f}, KD={kd:.3f}")
        
        # Send updated commands to all motors with current positions/velocities but new gains
        motor_commands = []
        
        for motor_index in range(self.motor_count):
            spinboxes = self.motor_cmd_spinboxes[motor_index]
            
            motor_cmd = MotorCmd()
            motor_cmd.position = spinboxes[0].value()  # Position
            motor_cmd.velocity = spinboxes[1].value()  # Velocity
            motor_cmd.torque = spinboxes[2].value()    # Torque
            motor_cmd.kp = kp                          # Global KP
            motor_cmd.kd = kd                          # Global KD
            
            motor_commands.append(motor_cmd)
        
        self.ros_node.publish_low_command(motor_commands)
        print(f"Applied global gains to all {self.motor_count} motors")

    def _send_control_cmd(self, cmd_value: int):
        """Send ControlCmd message"""
        motor_id = self.control_motor_id.value()
        self.ros_node.publish_control_command(motor_id, cmd_value)
        print(f"Sent ControlCmd: motor_id={motor_id}, cmd={cmd_value}")

    def _individual_set_default(self, motor_index: int):
        """Individual SET DEFAULT: Set specific motor to default values"""
        print(f"SET DEFAULT: Setting motor {motor_index} to default values...")
        
        # Set specific motor GUI controls to default values (position, velocity, torque)
        self._set_single_motor_to_default(motor_index)
        print(f"SET DEFAULT: Set motor {motor_index} GUI controls to default values")
        
        # Send default position command to specific motor with global KP/KD
        self._send_single_default_command(motor_index)
        print(f"SET DEFAULT: Sent default command to motor {motor_index}")

    def _set_single_motor_to_default(self, motor_index: int):
        """Set single motor GUI controls to default values"""
        if motor_index < len(self.motor_cmd_spinboxes):
            spinboxes = self.motor_cmd_spinboxes[motor_index]
            spinboxes[0].setValue(DEFAULT_POSITION)   # Position
            spinboxes[1].setValue(DEFAULT_VELOCITY)   # Velocity
            spinboxes[2].setValue(DEFAULT_TORQUE)     # Torque
    
    def _send_single_default_command(self, motor_index: int):
        """Send default position command for a single motor"""
        kp = self.global_kp_spinbox.value()
        kd = self.global_kd_spinbox.value()
        
        self.ros_node.publish_single_motor_command(
            motor_index, 
            DEFAULT_POSITION, 
            DEFAULT_VELOCITY, 
            DEFAULT_TORQUE, 
            kp, 
            kd
        )

    def _send_single_motor_cmd(self, motor_index: int):
        """Send OneMotorCmd for a single motor"""
        spinboxes = self.motor_cmd_spinboxes[motor_index]
        kp = self.global_kp_spinbox.value()
        kd = self.global_kd_spinbox.value()
        
        position = spinboxes[0].value()
        velocity = spinboxes[1].value()
        torque = spinboxes[2].value()
        
        self.ros_node.publish_single_motor_command(motor_index, position, velocity, torque, kp, kd)
        print(f"Sent OneMotorCmd for motor {motor_index}")

    def _send_all_motors_cmd(self):
        """Send LowCmd for all motors"""
        motor_commands = []
        kp = self.global_kp_spinbox.value()
        kd = self.global_kd_spinbox.value()
        
        for motor_index in range(self.motor_count):
            spinboxes = self.motor_cmd_spinboxes[motor_index]
            
            motor_cmd = MotorCmd()
            motor_cmd.position = spinboxes[0].value()  # Position
            motor_cmd.velocity = spinboxes[1].value()  # Velocity
            motor_cmd.torque = spinboxes[2].value()    # Torque
            motor_cmd.kp = kp                          # Global KP
            motor_cmd.kd = kd                          # Global KD
            
            motor_commands.append(motor_cmd)
        
        self.ros_node.publish_low_command(motor_commands)
        print(f"Sent LowCmd for all {self.motor_count} motors (KP={kp:.3f}, KD={kd:.3f})")

    def _emergency_stop(self):
        """Emergency stop - send disable commands to all motors and set to default"""
        print("EMERGENCY STOP: Sending DISABLE commands to all motors...")
        
        # Send DISABLE command to all motors
        for motor_id in range(self.motor_count):
            self.ros_node.publish_control_command(motor_id, 253)  # DISABLE command
        
        # Set all UI controls to default values
        for motor_index in range(self.motor_count):
            self._set_single_motor_to_default(motor_index)
        
        # Send default commands to all motors with current global gains
        kp = self.global_kp_spinbox.value()
        kd = self.global_kd_spinbox.value()
        
        for motor_index in range(self.motor_count):
            self.ros_node.publish_single_motor_command(
                motor_index, 
                DEFAULT_POSITION, 
                DEFAULT_VELOCITY, 
                DEFAULT_TORQUE, 
                kp, 
                kd
            )
        
        print(f"EMERGENCY STOP: All {self.motor_count} motors disabled and set to default values")

    def _on_timer(self):
        """Update GUI with current state data"""
        # Update motor state displays
        for motor_index in range(self.motor_count):
            if motor_index < len(self.ros_node.motor_states):
                motor_state = self.ros_node.motor_states[motor_index]
                labels = self.motor_cmd_data_labels[motor_index]
                
                if len(labels) >= 3:
                    labels[0].setText(f"current: {motor_state.position:.3f}")
                    labels[1].setText(f"current: {motor_state.velocity:.3f}")
                    labels[2].setText(f"current: {motor_state.torque:.3f}")
        
        # Update system status
        low_state = self.ros_node.current_low_state
        if hasattr(low_state, 'imu_state') and low_state.imu_state:
            imu = low_state.imu_state
            
            # Quaternion
            quat_str = "[-, -, -, -]"
            if len(imu.quaternion) >= 4:
                quat_str = f"[{imu.quaternion[0]:.3f}, {imu.quaternion[1]:.3f}, {imu.quaternion[2]:.3f}, {imu.quaternion[3]:.3f}]"
            
            # RPY
            rpy_str = "[-, -, -]°"
            if len(imu.rpy) >= 3:
                rpy_str = f"[{math.degrees(imu.rpy[0]):.1f}°, {math.degrees(imu.rpy[1]):.1f}°, {math.degrees(imu.rpy[2]):.1f}°]"
            
            self.imu_status_label.setText(f"IMU: Quaternion={quat_str} | RPY={rpy_str}")
        
        self.tick_label.setText(f"Tick: {low_state.tick}")
        
        if low_state.timestamp_state.sec > 0:
            timestamp = f"{low_state.timestamp_state.sec}.{low_state.timestamp_state.nanosec:09d}"
            self.timestamp_label.setText(f"Last Update: {timestamp}")


def main(args=None):
    rclpy.init(args=args)
    
    # Загружаем конфиг перед созданием нод
    try:
        pkg_share = get_package_share_directory("tinker_gui")
        config_path = os.path.join(pkg_share, "config", "gui_node.yaml")
        with open(config_path, 'r') as file:
            config = yaml.safe_load(file)
        motor_count = config.get('motors_number', MOTOR_COUNT)
    except Exception as e:
        print(f"Ошибка загрузки конфига: {e}")
        motor_count = MOTOR_COUNT
    
    ros_node = MotorSliderNode(motor_count)
    app = QApplication(sys.argv)
    window = SliderWindow(ros_node)
    window.show()

    def spin_ros():
        try:
            rclpy.spin_once(ros_node, timeout_sec=0)
        except Exception:
            # Игнорируем ошибки при завершении
            pass

    ros_timer = QTimer()
    ros_timer.timeout.connect(spin_ros)
    ros_timer.start(10)

    def signal_handler(sig, frame):
        print("Получен сигнал завершения, закрываем приложение...")
        ros_timer.stop()
        app.quit()
        ros_node.destroy_node()
        rclpy.shutdown()
        sys.exit(0)

    # Обработка сигналов завершения
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    try:
        exit_code = app.exec_()
    except KeyboardInterrupt:
        print("Прерывание с клавиатуры")
        exit_code = 0
    finally:
        ros_timer.stop()
        ros_node.destroy_node()
        rclpy.shutdown()
    
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
