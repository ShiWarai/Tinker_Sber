#!/home/dzirt/miniconda3/envs/Alpha_Human_gym/bin/python3
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy
from sensor_msgs.msg import JointState, Imu
from geometry_msgs.msg import Twist, Vector3, Quaternion
from nav_msgs.msg import Odometry
from std_msgs.msg import Header
import threading
import math
import numpy as np
import mujoco
import mujoco_viewer
from collections import deque
from scipy.spatial.transform import Rotation as R
import torch
import sys
import os
import time

# Добавляем путь к корню проекта
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../..'))

# Импорты из вашего проекта
try:
    from global_config import ROOT_DIR
    from configs.h1_constraint_him_trot import H1ConstraintHimRoughCfg
except ImportError:
    # Если импорт не работает, задаем значения по умолчанию
    ROOT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    
    # Минимальная конфигурация
    class H1ConstraintHimRoughCfg:
        class env:
            num_actions = 10
            n_proprio = 45
            history_len = 10
            n_priv_latent = 0
            n_scan = 0
            num_observations = 45 + 10 * 45
        
        class normalization:
            class obs_scales:
                ang_vel = 0.25
                quat = 1.0
                lin_vel = 2.0
                dof_pos = 1.0
                dof_vel = 0.05
            clip_observations = 100.0
            clip_actions = 100.0

# Константы
default_dof_pos = [0.0, 0.08, 0.56, -1.12, -0.57, 0.0, -0.08, -0.56, 1.12, 0.57]

class Command:
    def __init__(self):
        self.vx = 0.0
        self.vy = 0.0
        self.dyaw = 0.0

def quaternion_to_euler_array(quat):
    """Convert quaternion [x, y, z, w] to Euler angles [roll, pitch, yaw]"""
    x, y, z, w = quat
    t0 = +2.0 * (w * x + y * z)
    t1 = +1.0 - 2.0 * (x * x + y * y)
    roll_x = np.arctan2(t0, t1)
    
    t2 = +2.0 * (w * y - z * x)
    t2 = np.clip(t2, -1.0, 1.0)
    pitch_y = np.arcsin(t2)
    
    t3 = +2.0 * (w * z + x * y)
    t4 = +1.0 - 2.0 * (y * y + z * z)
    yaw_z = np.arctan2(t3, t4)
    
    return np.array([roll_x, pitch_y, yaw_z])

def get_obs(data):
    """Extract observation from MuJoCo data"""
    q = data.qpos.astype(np.double)
    dq = data.qvel.astype(np.double)
    
    # Get orientation (assuming sensor named 'orientation' exists)
    quat = data.sensor('orientation').data[[1, 2, 3, 0]].astype(np.double)
    
    r = R.from_quat(quat)
    v = r.apply(data.qvel[:3], inverse=True).astype(np.double)
    
    # Get angular velocity (assuming sensor named 'angular-velocity' exists)
    omega = data.sensor('angular-velocity').data.astype(np.double)
    
    gvec = r.apply(np.array([0., 0., -1.]), inverse=True).astype(np.double)
    
    return q, dq, quat, v, omega, gvec

def pd_control(target_q, q, kp, target_dq, dq, kd):
    """PD controller for joint positions"""
    return (target_q - q) * kp + (target_dq - dq) * kd

def low_pass_filter(actions, last_actions, filter_coef=0.1):
    """Low-pass filter for smooth actions"""
    return last_actions * filter_coef + actions * (1 - filter_coef)

class H1MuJoCoNode(Node):
    def __init__(self):
        super().__init__('h1_mujoco_node')
        
        # Parameters
        self.declare_parameter('model_path', 
                              os.path.join(ROOT_DIR, 'scripts/model/trot.pt'))
        self.declare_parameter('mujoco_xml', 
                              os.path.join(ROOT_DIR, 'resources/h1/xml/world.xml'))
        self.declare_parameter('sim_duration', 60.0)
        self.declare_parameter('control_rate', 100.0)
        
        # Command storage
        self.command = Command()
        self.last_cmd_time = self.get_clock().now()
        
        # QoS settings for real-time performance
        qos_profile = QoSProfile(
            depth=10,
            reliability=ReliabilityPolicy.BEST_EFFORT,
            durability=DurabilityPolicy.VOLATILE
        )
        
        # Publishers
        self.joint_pub = self.create_publisher(JointState, 'joint_states', qos_profile)
        self.odom_pub = self.create_publisher(Odometry, 'odom', qos_profile)
        self.imu_pub = self.create_publisher(Imu, 'imu/data', qos_profile)
        self.h1_joint_pub = self.create_publisher(JointState, 'h1/joint_states', qos_profile)
        
        # Subscribers
        self.cmd_sub = self.create_subscription(
            Twist, 'cmd_vel', self.cmd_vel_callback, 10)
        
        # Control variables
        self.sim_running = True
        self.sim_thread = None
        
        self.get_logger().info('H1 MuJoCo ROS 2 Node initialized')
        
        # Start simulation in separate thread
        self.start_simulation()
        
    def cmd_vel_callback(self, msg):
        """Handle velocity commands"""
        self.command.vx = msg.linear.x
        self.command.vy = msg.linear.y
        self.command.dyaw = msg.angular.z
        self.last_cmd_time = self.get_clock().now()
        
        self.get_logger().info(f'Command received: vx={msg.linear.x:.2f}, vy={msg.linear.y:.2f}, dyaw={msg.angular.z:.2f}')
    
    def publish_robot_data(self, q, dq, quat, v, omega):
        """Publish robot state to ROS 2 topics"""
        current_time = self.get_clock().now().to_msg()
        
        # Joint States
        joint_msg = JointState()
        joint_msg.header.stamp = current_time
        joint_msg.name = [
            'FL_hip_joint', 'FL_thigh_joint', 'FL_calf_joint',
            'FR_hip_joint', 'FR_thigh_joint', 'FR_calf_joint', 
            'RL_hip_joint', 'RL_thigh_joint', 'RL_calf_joint',
            'RR_hip_joint', 'RR_thigh_joint', 'RR_calf_joint'
        ]
        
        # Use first 10 positions for H1 robot
        positions = q[:10].tolist() if len(q) >= 10 else [0.0] * 10
        velocities = dq[:10].tolist() if len(dq) >= 10 else [0.0] * 10
        
        # Pad to 12 joints if needed
        joint_msg.position = positions + [0.0] * (12 - len(positions))
        joint_msg.velocity = velocities + [0.0] * (12 - len(velocities))
        joint_msg.effort = [0.0] * 12
        
        self.joint_pub.publish(joint_msg)
        self.h1_joint_pub.publish(joint_msg)
        
        # IMU Data
        imu_msg = Imu()
        imu_msg.header.stamp = current_time
        imu_msg.header.frame_id = 'base_link'
        imu_msg.angular_velocity = Vector3(x=float(omega[0]), y=float(omega[1]), z=float(omega[2]))
        imu_msg.orientation = Quaternion(x=float(quat[0]), y=float(quat[1]), 
                                       z=float(quat[2]), w=float(quat[3]))
        
        # Odometry
        odom_msg = Odometry()
        odom_msg.header.stamp = current_time
        odom_msg.header.frame_id = 'odom'
        odom_msg.child_frame_id = 'base_link'
        odom_msg.pose.pose.orientation = imu_msg.orientation
        odom_msg.twist.twist.linear = Vector3(x=float(v[0]), y=float(v[1]), z=float(v[2]))
        odom_msg.twist.twist.angular = imu_msg.angular_velocity
        
        self.imu_pub.publish(imu_msg)
        self.odom_pub.publish(odom_msg)
    
    def load_policy_model(self, model_path):
        """Load the trained policy model"""
        try:
            if not os.path.exists(model_path):
                self.get_logger().error(f"Model file not found: {model_path}")
                return None
            
            self.get_logger().info(f"Loading model from: {model_path}")
            policy = torch.load(model_path, map_location='cpu')
            policy.eval()
            return policy
            
        except Exception as e:
            self.get_logger().error(f"Error loading model: {str(e)}")
            return None
    
    def run_simulation_loop(self):
        """Main simulation loop"""
        try:
            # Load parameters
            model_path = self.get_parameter('model_path').value
            mujoco_xml = self.get_parameter('mujoco_xml').value
            sim_duration = self.get_parameter('sim_duration').value
            
            # Load policy
            policy = self.load_policy_model(model_path)
            if policy is None:
                self.get_logger().error("Failed to load policy model")
                return
            
            # Simulation configuration
            class SimulationConfig:
                class sim_config:
                    mujoco_model_path = mujoco_xml
                    sim_duration = sim_duration
                    dt = 0.001
                    decimation = 20
                
                class robot_config:
                    kps = np.array([13, 15, 15, 15, 13, 13, 15, 15, 15, 13], dtype=np.double)
                    kds = np.array([0.3, 0.65, 0.65, 0.65, 0.3, 0.3, 0.65, 0.65, 0.65, 0.3], dtype=np.double)
                    tau_limit = 20.0 * np.ones(10, dtype=np.double)
                
                class env:
                    num_actions = 10
                    n_proprio = 45
                    history_len = 10
                    n_priv_latent = 0
                    n_scan = 0
                    num_observations = 495  # 45 + 10*45
                
                class normalization:
                    class obs_scales:
                        ang_vel = 0.25
                        quat = 1.0
                        lin_vel = 2.0
                        dof_pos = 1.0
                        dof_vel = 0.05
                    clip_observations = 100.0
                    clip_actions = 100.0
            
            cfg = SimulationConfig()
            
            # Initialize MuJoCo
            self.get_logger().info(f"Loading MuJoCo model from: {cfg.sim_config.mujoco_model_path}")
            model = mujoco.MjModel.from_xml_path(cfg.sim_config.mujoco_model_path)
            model.opt.timestep = cfg.sim_config.dt
            data = mujoco.MjData(model)
            
            # Initialize viewer
            viewer = mujoco_viewer.MujocoViewer(model, data)
            viewer.cam.distance = 3.0
            viewer.cam.azimuth = 180.0
            viewer.cam.elevation = -20.0
            
            # Control variables
            target_q = np.zeros(cfg.env.num_actions, dtype=np.double)
            action = np.zeros(cfg.env.num_actions, dtype=np.double)
            action_flt = np.zeros(cfg.env.num_actions, dtype=np.double)
            last_actions = np.zeros(cfg.env.num_actions, dtype=np.double)
            
            # Observation history
            hist_obs = deque(maxlen=cfg.env.history_len)
            for _ in range(cfg.env.history_len):
                hist_obs.append(np.zeros(cfg.env.n_proprio, dtype=np.float32))
            
            step_count = 0
            total_steps = int(cfg.sim_config.sim_duration / cfg.sim_config.dt)
            
            self.get_logger().info("Starting simulation...")
            
            # Main simulation loop
            while self.sim_running and step_count < total_steps and viewer.is_alive:
                # Step simulation
                mujoco.mj_step(model, data)
                
                # Get observations
                q, dq, quat, v, omega, gvec = get_obs(data)
                q_actions = q[-cfg.env.num_actions:]
                dq_actions = dq[-cfg.env.num_actions:]
                
                # Publish to ROS 2
                self.publish_robot_data(q, dq, quat, v, omega)
                
                # RL control at lower frequency
                if step_count % cfg.sim_config.decimation == 0:
                    # Prepare observation
                    obs = np.zeros(cfg.env.n_proprio, dtype=np.float32)
                    eu_ang = quaternion_to_euler_array(quat)
                    eu_ang[eu_ang > math.pi] -= 2 * math.pi
                    
                    # Fill observation
                    obs[0] = omega[0] * cfg.normalization.obs_scales.ang_vel
                    obs[1] = omega[1] * cfg.normalization.obs_scales.ang_vel
                    obs[2] = omega[2] * cfg.normalization.obs_scales.ang_vel
                    obs[3] = eu_ang[0] * cfg.normalization.obs_scales.quat
                    obs[4] = eu_ang[1] * cfg.normalization.obs_scales.quat
                    obs[5] = eu_ang[2] * cfg.normalization.obs_scales.quat
                    obs[6] = self.command.vx * cfg.normalization.obs_scales.lin_vel
                    obs[7] = self.command.vy * cfg.normalization.obs_scales.lin_vel
                    obs[8] = self.command.dyaw * cfg.normalization.obs_scales.ang_vel
                    obs[9:19] = (q_actions - default_dof_pos) * cfg.normalization.obs_scales.dof_pos
                    obs[19:29] = dq_actions * cfg.normalization.obs_scales.dof_vel
                    obs[29:39] = last_actions
                    
                    obs = np.clip(obs, -cfg.normalization.clip_observations, cfg.normalization.clip_observations)
                    
                    # Update history
                    hist_obs.append(obs)
                    
                    # Prepare policy input
                    policy_input = np.zeros(cfg.env.num_observations, dtype=np.float32)
                    policy_input[:cfg.env.n_proprio] = obs
                    
                    # Add history (flattened)
                    hist_start = cfg.env.n_proprio + cfg.env.n_priv_latent + cfg.env.n_scan
                    for i, hist_obs_item in enumerate(hist_obs):
                        start_idx = hist_start + i * cfg.env.n_proprio
                        end_idx = start_idx + cfg.env.n_proprio
                        policy_input[start_idx:end_idx] = hist_obs_item
                    
                    # Get action from policy
                    with torch.no_grad():
                        policy_input_tensor = torch.tensor(policy_input).unsqueeze(0).half()
                        action_tensor = policy.act_teacher(policy_input_tensor)[0]
                        action = action_tensor.cpu().numpy()
                    
                    action = np.clip(action, -cfg.normalization.clip_actions, cfg.normalization.clip_actions)
                    action_flt = low_pass_filter(action, last_actions)
                    last_actions = action_flt.copy()
                    
                    target_q = action_flt * 0.25 + default_dof_pos
                
                # PD control
                target_dq = np.zeros(cfg.env.num_actions, dtype=np.double)
                tau = pd_control(target_q, q_actions, cfg.robot_config.kps,
                               target_dq, dq_actions, cfg.robot_config.kds)
                tau = np.clip(tau, -cfg.robot_config.tau_limit, cfg.robot_config.tau_limit)
                
                # Apply control
                data.ctrl = tau
                
                # Render
                viewer.render()
                step_count += 1
                
                # Small delay to prevent excessive CPU usage
                time.sleep(0.001)
            
            viewer.close()
            self.get_logger().info("Simulation finished")
            
        except Exception as e:
            self.get_logger().error(f"Simulation error: {str(e)}")
            import traceback
            self.get_logger().error(traceback.format_exc())
    
    def start_simulation(self):
        """Start simulation in a separate thread"""
        self.sim_thread = threading.Thread(target=self.run_simulation_loop, daemon=True)
        self.sim_thread.start()
    
    def stop_simulation(self):
        """Stop the simulation"""
        self.sim_running = False
        if self.sim_thread and self.sim_thread.is_alive():
            self.sim_thread.join(timeout=5.0)

def main(args=None):
    rclpy.init(args=args)
    
    try:
        node = H1MuJoCoNode()
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info('Keyboard interrupt received')
    finally:
        node.stop_simulation()
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()