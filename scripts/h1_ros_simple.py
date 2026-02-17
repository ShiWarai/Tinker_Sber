#!/usr/bin/env python3

import math
import numpy as np
import mujoco
import mujoco_viewer
import torch
from collections import deque
from scipy.spatial.transform import Rotation as R
import time
import sys
import os

# Добавляем пути
sys.path.append('..')
from global_config import ROOT_DIR
from configs.h1_constraint_him_trot import H1ConstraintHimRoughCfg

default_dof_pos = [0.0, 0.08, 0.56, -1.12, -0.57, 0.0, -0.08, -0.56, 1.12, 0.57]

class H1FixedController:
    def __init__(self):
        self.cmd_velocity = [0.5, 0.0, 0.2]  # vx, vy, dyaw
        print("=== H1 Mujoco Fixed Controller ===")
        self.setup()
        
    def setup(self):
        """Настройка модели и политики"""
        # Конфигурация
        class Sim2simCfg(H1ConstraintHimRoughCfg):
            class sim_config:
                mujoco_model_path = f'{ROOT_DIR}/resources/h1/xml/world.xml'
                sim_duration = 60.0
                dt = 0.001
                decimation = 20

            class robot_config:
                kps = np.array([13, 15, 15, 15, 13, 13, 15, 15, 15, 13], dtype=np.double)
                kds = np.array([0.3, 0.65, 0.65, 0.65, 0.3, 0.3, 0.65, 0.65, 0.65, 0.3], dtype=np.double)
                tau_limit = 20. * np.ones(10, dtype=np.double)

        self.cfg = Sim2simCfg()
        
        # Загрузка модели Mujoco
        print(f"Loading Mujoco model from: {self.cfg.sim_config.mujoco_model_path}")
        try:
            self.model = mujoco.MjModel.from_xml_path(self.cfg.sim_config.mujoco_model_path)
            self.model.opt.timestep = self.cfg.sim_config.dt
            self.data = mujoco.MjData(self.model)
            mujoco.mj_step(self.model, self.data)
            
            # Инициализация viewer
            self.viewer = mujoco_viewer.MujocoViewer(self.model, self.data)
            self.viewer.cam.distance = 3.0
            self.viewer.cam.azimuth = 180
            self.viewer.cam.elevation = -20
            
            print("✓ Mujoco simulation loaded")
        except Exception as e:
            print(f"✗ Failed to load Mujoco: {e}")
            raise

        # Загрузка политики с исправлением
        policy_path = os.path.join(ROOT_DIR, 'scripts/model/trot.pt')
        self.policy = None
        
        if os.path.exists(policy_path):
            print(f"Loading policy from: {policy_path}")
            self.policy = self.load_policy_safely(policy_path)
        else:
            print(f"Policy file not found: {policy_path}")
            print("Running in default pose mode")

        # Инициализация переменных
        self.hist_obs = deque()
        for _ in range(self.cfg.env.history_len):
            self.hist_obs.append(np.zeros([1, self.cfg.env.n_proprio], dtype=np.double))
        
        self.last_actions = np.zeros((self.cfg.env.num_actions), dtype=np.double)
        self.count_lowlevel = 0
        
        print("✓ Controller initialized")

    def load_policy_safely(self, policy_path):
        """Безопасная загрузка модели PyTorch"""
        try:
            # Способ 1: Простая загрузка с weights_only=False
            policy = torch.load(policy_path, map_location='cpu', weights_only=False)
            print("✓ Policy loaded with weights_only=False")
            return policy
            
        except Exception as e:
            print(f"Method 1 failed: {e}")
            
            try:
                # Способ 2: Загрузка как state_dict
                checkpoint = torch.load(policy_path, map_location='cpu', weights_only=False)
                
                # Проверяем различные форматы моделей
                if hasattr(checkpoint, 'state_dict'):
                    policy = checkpoint
                elif 'model_state_dict' in checkpoint:
                    policy = checkpoint['model_state_dict']
                elif 'state_dict' in checkpoint:
                    policy = checkpoint['state_dict']
                elif 'actor' in checkpoint:
                    policy = checkpoint['actor']
                else:
                    policy = checkpoint
                    
                print("✓ Policy loaded as state_dict")
                return policy
                
            except Exception as e2:
                print(f"Method 2 failed: {e2}")
                
                try:
                    # Способ 3: Используем pickle напрямую
                    import pickle
                    with open(policy_path, 'rb') as f:
                        policy = pickle.load(f)
                    print("✓ Policy loaded with pickle")
                    return policy
                    
                except Exception as e3:
                    print(f"Method 3 failed: {e3}")
                    
                    try:
                        # Способ 4: Игнорируем ошибки и загружаем то, что можем
                        checkpoint = torch.load(policy_path, map_location='cpu', weights_only=False, pickle_module=__import__('pickle'))
                        print("✓ Policy loaded with custom pickle module")
                        return checkpoint
                        
                    except Exception as e4:
                        print(f"All loading methods failed: {e4}")
                        print("❌ Running without policy - using default pose")
                        return None

    def quaternion_to_euler_array(self, quat):
        """Конвертация кватерниона в углы Эйлера"""
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

    def get_obs(self, data):
        """Получение наблюдений из Mujoco"""
        q = data.qpos.astype(np.double)
        dq = data.qvel.astype(np.double)
        
        quat = np.array([1., 0., 0., 0.])
        omega = np.array([0., 0., 0.])
        
        try:
            if hasattr(data.sensor('orientation'), 'data'):
                quat = data.sensor('orientation').data[[1, 2, 3, 0]].astype(np.double)
            if hasattr(data.sensor('angular-velocity'), 'data'):
                omega = data.sensor('angular-velocity').data.astype(np.double)
        except:
            pass
        
        return q, dq, quat, omega

    def pd_control(self, target_q, q, kp, target_dq, dq, kd):
        """PD контроллер"""
        return (target_q - q) * kp + (target_dq - dq) * kd

    def apply_control(self):
        """Применение управления"""
        q = self.data.qpos[-self.cfg.env.num_actions:] if len(self.data.qpos) >= self.cfg.env.num_actions else self.data.qpos
        dq = self.data.qvel[-self.cfg.env.num_actions:] if len(self.data.qvel) >= self.cfg.env.num_actions else self.data.qvel
        
        if self.policy is not None:
            # Режим с политикой RL
            try:
                q_full, dq_full, quat, omega = self.get_obs(self.data)
                q_joints = q_full[-self.cfg.env.num_actions:] if len(q_full) >= self.cfg.env.num_actions else q_full
                dq_joints = dq_full[-self.cfg.env.num_actions:] if len(dq_full) >= self.cfg.env.num_actions else dq_full
                
                # Подготовка наблюдений (упрощенная версия)
                obs = np.zeros([1, self.cfg.env.n_proprio], dtype=np.float32)
                eu_ang = self.quaternion_to_euler_array(quat)
                eu_ang[eu_ang > math.pi] -= 2 * math.pi
                
                cmd_vx, cmd_vy, cmd_dyaw = self.cmd_velocity
                
                # Заполнение наблюдений
                obs[0, 0] = omega[0] * self.cfg.normalization.obs_scales.ang_vel
                obs[0, 1] = omega[1] * self.cfg.normalization.obs_scales.ang_vel
                obs[0, 2] = omega[2] * self.cfg.normalization.obs_scales.ang_vel
                obs[0, 3] = eu_ang[0] * self.cfg.normalization.obs_scales.quat
                obs[0, 4] = eu_ang[1] * self.cfg.normalization.obs_scales.quat
                obs[0, 5] = eu_ang[2] * self.cfg.normalization.obs_scales.quat
                obs[0, 6] = cmd_vx * self.cfg.normalization.obs_scales.lin_vel
                obs[0, 7] = cmd_vy * self.cfg.normalization.obs_scales.lin_vel
                obs[0, 8] = cmd_dyaw * self.cfg.normalization.obs_scales.ang_vel
                
                obs[0, 9:19] = (q_joints - default_dof_pos) * self.cfg.normalization.obs_scales.dof_pos
                obs[0, 19:29] = dq_joints * self.cfg.normalization.obs_scales.dof_vel
                obs[0, 29:39] = self.last_actions
                
                # Применение политики
                with torch.no_grad():
                    if hasattr(self.policy, 'act_teacher'):
                        action = self.policy.act_teacher(torch.tensor(obs).half())[0].detach().numpy()
                    elif hasattr(self.policy, '__call__'):
                        action = self.policy(torch.tensor(obs).half())[0].detach().numpy()
                    else:
                        # Если политика - это просто тензор
                        action = np.random.uniform(-0.1, 0.1, self.cfg.env.num_actions)
                
                action = np.clip(action, -self.cfg.normalization.clip_actions, self.cfg.normalization.clip_actions)
                self.last_actions = action
                
                target_q = action * 0.25 + default_dof_pos
                
            except Exception as e:
                print(f"Policy execution failed: {e}")
                target_q = np.array(default_dof_pos)
        else:
            # Режим без политики - стоячая поза
            target_q = np.array(default_dof_pos)
        
        target_dq = np.zeros(self.cfg.env.num_actions)
        tau = self.pd_control(target_q, q, self.cfg.robot_config.kps, target_dq, dq, self.cfg.robot_config.kds)
        tau = np.clip(tau, -self.cfg.robot_config.tau_limit, self.cfg.robot_config.tau_limit)
        self.data.ctrl = tau

    def run(self):
        """Основной цикл"""
        print("Starting simulation...")
        print("Press ESC to exit")
        
        try:
            step_count = 0
            start_time = time.time()
            
            while self.viewer.is_alive:
                # Шаг симуляции
                mujoco.mj_step(self.model, self.data)
                self.viewer.render()
                
                # Управление на 50Hz
                if self.count_lowlevel % self.cfg.sim_config.decimation == 0:
                    self.apply_control()
                
                self.count_lowlevel += 1
                step_count += 1
                
                # Вывод статуса каждые 1000 шагов
                if step_count % 1000 == 0:
                    elapsed = time.time() - start_time
                    print(f"Step {step_count}, Time: {elapsed:.2f}s")
                
            print("Viewer closed")
                
        except KeyboardInterrupt:
            print("\nSimulation stopped by user")
        except Exception as e:
            print(f"Simulation error: {e}")
        finally:
            self.viewer.close()
            print("Simulation finished")

if __name__ == '__main__':
    try:
        controller = H1FixedController()
        controller.run()
    except Exception as e:
        print(f"Failed to start controller: {e}")