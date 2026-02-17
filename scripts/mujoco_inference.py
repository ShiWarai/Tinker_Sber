# fixed_mujoco_inference.py
import mujoco
import numpy as np
import torch
import os
import time
import matplotlib.pyplot as plt
from typing import Dict, Any

def create_proper_h1_model():
    """Создание правильного XML файла модели H1"""
    
    proper_xml = """<mujoco model="h1">
  <compiler angle="radian" coordinate="local" inertiafromgeom="true"/>
  <option timestep="0.0025" integrator="RK4"/>
  
  <default>
    <joint armature="0.01" damping="0.1" limited="true"/>
    <geom conaffinity="0" condim="3" friction="1 0.005 0.0001" margin="0.001"/>
    <motor ctrllimited="true" ctrlrange="-1 1"/>
  </default>
  
  <worldbody>
    <light cutoff="100" diffuse="1 1 1" dir="-0 0 -1.3" directional="true" exponent="1" pos="0 0 1.3" specular=".1 .1 .1"/>
    <geom conaffinity="1" condim="3" name="floor" pos="0 0 0" rgba="0.8 0.9 0.8 1" size="40 40 40" type="plane"/>
    
    <!-- H1 Robot Body -->
    <body name="base" pos="0 0 0.75">
        <joint name="float_base" type="free"/>
        <inertial pos="0.00 0 0.036" mass="3" diaginertia="0.02538 0.03617 0.01306"/>  
        <geom type="box" size="0.2 0.1 0.05" rgba="0.3 0.3 0.8 1"/>
        <site name='imu' size='0.01' pos='0.0 0 0.0'/>

        <!-- Left Leg -->
        <body name="L0" pos="0.1 0.05 0">
            <joint name="L0_joint" type="hinge" axis="0 0 1" range="-0.7 0.7"/>
            <geom type="capsule" fromto="0 0 0 0.08 0 0" size="0.02" rgba="0.8 0.3 0.3 1"/>
            <body name="L1" pos="0.08 0 0">
                <joint name="L1_joint" type="hinge" axis="-1 0 0" range="-0.38 0.46"/>
                <geom type="capsule" fromto="0 0 0 0 0 -0.12" size="0.018" rgba="0.8 0.3 0.3 1"/>
                <body name="L2" pos="0 0 -0.12">
                    <joint name="L2_joint" type="hinge" axis="0 1 0" range="-1.57 1.57"/>
                    <geom type="capsule" fromto="0 0 0 0 0 -0.12" size="0.015" rgba="0.8 0.3 0.3 1"/>
                    <body name="L3" pos="0 0 -0.12">
                        <joint name="L3_joint" type="hinge" axis="0 1 0" range="-2.35 0"/>
                        <geom type="capsule" fromto="0 0 0 0 0 -0.08" size="0.012" rgba="0.8 0.3 0.3 1"/>
                        <body name="L4" pos="0 0 -0.08">
                            <joint name="L4_joint" type="hinge" axis="0 -1 0" range="-1.2 1.2"/>
                            <geom type="sphere" size="0.015" rgba="0.8 0.8 0.3 1"/>
                        </body>
                    </body>
                </body>
            </body>
        </body>
        
        <!-- Right Leg -->
        <body name="R0" pos="0.1 -0.05 0">
            <joint name="R0_joint" type="hinge" axis="0 0 1" range="-0.7 0.7"/>
            <geom type="capsule" fromto="0 0 0 0.08 0 0" size="0.02" rgba="0.3 0.8 0.3 1"/>
            <body name="R1" pos="0.08 0 0">
                <joint name="R1_joint" type="hinge" axis="-1 0 0" range="-0.38 0.47"/>
                <geom type="capsule" fromto="0 0 0 0 0 -0.12" size="0.018" rgba="0.3 0.8 0.3 1"/>
                <body name="R2" pos="0 0 -0.12">
                    <joint name="R2_joint" type="hinge" axis="0 -1 0" range="-1.57 1.57"/>
                    <geom type="capsule" fromto="0 0 0 0 0 -0.12" size="0.015" rgba="0.3 0.8 0.3 1"/>
                    <body name="R3" pos="0 0 -0.12">
                        <joint name="R3_joint" type="hinge" axis="0 -1 0" range="0 2.35"/>
                        <geom type="capsule" fromto="0 0 0 0 0 -0.08" size="0.012" rgba="0.3 0.8 0.3 1"/>
                        <body name="R4" pos="0 0 -0.08">
                            <joint name="R4_joint" type="hinge" axis="0 1 0" range="-1.2 1.2"/>
                            <geom type="sphere" size="0.015" rgba="0.8 0.8 0.3 1"/>
                        </body>
                    </body>
                </body>
            </body>
        </body>
    </body>
  </worldbody>
  
  <actuator>
    <!-- Left Leg Actuators -->
    <motor joint="L0_joint" gear="50"/>
    <motor joint="L1_joint" gear="80"/>
    <motor joint="L2_joint" gear="100"/>
    <motor joint="L3_joint" gear="80"/>
    <motor joint="L4_joint" gear="50"/>
    
    <!-- Right Leg Actuators -->
    <motor joint="R0_joint" gear="50"/>
    <motor joint="R1_joint" gear="80"/>
    <motor joint="R2_joint" gear="100"/>
    <motor joint="R3_joint" gear="80"/>
    <motor joint="R4_joint" gear="50"/>
  </actuator>
  
  <sensor>
    <!-- Joint Position Sensors -->
    <jointpos name="L0_pos" joint="L0_joint"/>
    <jointpos name="L1_pos" joint="L1_joint"/>
    <jointpos name="L2_pos" joint="L2_joint"/>
    <jointpos name="L3_pos" joint="L3_joint"/>
    <jointpos name="L4_pos" joint="L4_joint"/>
    
    <jointpos name="R0_pos" joint="R0_joint"/>
    <jointpos name="R1_pos" joint="R1_joint"/>
    <jointpos name="R2_pos" joint="R2_joint"/>
    <jointpos name="R3_pos" joint="R3_joint"/>
    <jointpos name="R4_pos" joint="R4_joint"/>
  </sensor>
</mujoco>"""
    
    with open("h1_proper.xml", "w") as f:
        f.write(proper_xml)
    
    print("✅ Создан правильный XML файл модели H1: h1_proper.xml")
    return "h1_proper.xml"

class H1MuJoCoInferenceTest:
    def __init__(self, model_path: str, policy_checkpoint: str = None):
        """
        Инициализация теста инференса для H1 робота в MuJoCo
        """
        self.model_path = model_path
        
        # Загрузка модели MuJoCo
        print(f"📁 Загрузка модели из {model_path}")
        try:
            self.model = mujoco.MjModel.from_xml_path(model_path)
            self.data = mujoco.MjData(self.model)
            
            print(f"✅ Модель H1 успешно загружена!")
            print(f"📊 Характеристики:")
            print(f"   Суставы (nq): {self.model.nq}")
            print(f"   Скорости (nv): {self.model.nv}") 
            print(f"   Действия (nu): {self.model.nu}")
            print(f"   Тела: {self.model.nbody}")
            print(f"   Сенсоры: {self.model.nsensordata}")
            
        except Exception as e:
            print(f"❌ Ошибка загрузки модели: {e}")
            raise
        
        # Загрузка политики
        self.policy = self.load_policy(policy_checkpoint) if policy_checkpoint else self.create_default_policy()
        
        # Параметры управления
        self.episode_length = 1000
        
    def load_policy(self, checkpoint_path: str) -> torch.nn.Module:
        """Загрузка политики с адаптацией размерностей наблюдений"""
        try:
            if not os.path.exists(checkpoint_path):
                print(f"⚠️ Файл политики не найден: {checkpoint_path}")
                return self.create_default_policy()
                
            checkpoint = torch.load(checkpoint_path, map_location='cpu')
            print(f"✅ Загружен чекпоинт. Ключи: {list(checkpoint.keys())}")
            
            state_dict = checkpoint.get('model_state_dict', checkpoint)
            
            # Определяем ожидаемую размерность входа из чекпоинта
            expected_obs_dim = None
            for key in state_dict.keys():
                if 'mlp_encoder.0.weight' in key or 'actor_teacher_backbone.mlp_encoder.0.weight' in key:
                    expected_obs_dim = state_dict[key].shape[1]
                    print(f"🔍 Ожидаемая размерность входа: {expected_obs_dim}")
                    break
            
            if expected_obs_dim is None:
                print("❗ Не удалось определить ожидаемую размерность входа из чекпоинта")
                return self.create_default_policy()
            
            # Текущая размерность наблюдений
            current_obs_dim = self.get_observation().shape[0]
            action_dim = self.model.nu
            
            print(f"📊 Размерности: ожидается={expected_obs_dim}, есть={current_obs_dim}, actions={action_dim}")
            
            if current_obs_dim == expected_obs_dim:
                print("✅ Размерности наблюдений совпадают")
                # Создаем политику с правильной размерностью
                policy = self.create_policy(current_obs_dim, action_dim)
                # Загружаем веса
                policy = self.load_weights_compatible(policy, state_dict)
            else:
                print("🔄 Создаем адаптивную политику")
                policy = self.create_adaptive_policy(current_obs_dim, expected_obs_dim, action_dim, state_dict)
            
            policy.eval()
            return policy
            
        except Exception as e:
            print(f"❌ Ошибка загрузки политики: {e}")
            import traceback
            traceback.print_exc()
            return self.create_default_policy()

    def create_adaptive_policy(self, current_obs_dim, expected_obs_dim, action_dim, state_dict):
        """Создание политики с адаптацией размерностей"""
        
        class AdaptivePolicy(torch.nn.Module):
            def __init__(self, current_obs_dim, expected_obs_dim, action_dim, state_dict):
                super().__init__()
                
                # Адаптер для приведения размерностей
                self.obs_adapter = torch.nn.Linear(current_obs_dim, expected_obs_dim)
                
                # Основная сеть на основе архитектуры из чекпоинта
                self.mlp_encoder = torch.nn.Sequential(
                    torch.nn.Linear(expected_obs_dim, 512),
                    torch.nn.ReLU(),
                    torch.nn.Linear(512, 256),
                    torch.nn.ReLU(),
                    torch.nn.Linear(256, 128),
                    torch.nn.ReLU(),
                )
                
                self.actor = torch.nn.Sequential(
                    torch.nn.Linear(128, 64),
                    torch.nn.ReLU(),
                    torch.nn.Linear(64, 32),
                    torch.nn.ReLU(),
                    torch.nn.Linear(32, action_dim),
                    torch.nn.Tanh()
                )
                
                # Загружаем веса если размерности совпадают
                self.load_compatible_weights(state_dict)
                
            def load_compatible_weights(self, state_dict):
                """Загрузка совместимых весов"""
                try:
                    # Загружаем MLP энкодер
                    for i, (weight_key, bias_key) in enumerate([
                        ('actor_teacher_backbone.mlp_encoder.0.weight', 'actor_teacher_backbone.mlp_encoder.0.bias'),
                        ('actor_teacher_backbone.mlp_encoder.2.weight', 'actor_teacher_backbone.mlp_encoder.2.bias'),
                        ('actor_teacher_backbone.mlp_encoder.4.weight', 'actor_teacher_backbone.mlp_encoder.4.bias')
                    ]):
                        if weight_key in state_dict and bias_key in state_dict:
                            weight = state_dict[weight_key]
                            bias = state_dict[bias_key]
                            
                            # Адаптируем 3D веса в 2D
                            if len(weight.shape) == 3:
                                weight = weight[0]
                                bias = bias[0]
                            
                            mlp_layer_idx = i * 2
                            if (self.mlp_encoder[mlp_layer_idx].weight.shape == weight.shape and 
                                self.mlp_encoder[mlp_layer_idx].bias.shape == bias.shape):
                                self.mlp_encoder[mlp_layer_idx].weight.data = weight
                                self.mlp_encoder[mlp_layer_idx].bias.data = bias
                                print(f"✅ Загружен MLP слой {i}")
                    
                    # Загружаем актор если размерности совпадают
                    actor_mapping = [
                        ('actor_teacher_backbone.actor.w0', 'actor_teacher_backbone.actor.b0', 0),
                        ('actor_teacher_backbone.actor.w1', 'actor_teacher_backbone.actor.b1', 2),
                        ('actor_teacher_backbone.actor.w2', 'actor_teacher_backbone.actor.b2', 4)
                    ]
                    
                    for weight_key, bias_key, layer_idx in actor_mapping:
                        if weight_key in state_dict and bias_key in state_dict:
                            weight = state_dict[weight_key]
                            bias = state_dict[bias_key]
                            
                            if len(weight.shape) == 3:
                                weight = weight[0]
                                bias = bias[0]
                            
                            # Преобразуем размерности если нужно
                            if weight.shape[0] == 4:  # многомодальный выход
                                weight = weight.mean(dim=0)
                                bias = bias.mean(dim=0)
                            
                            if (self.actor[layer_idx].weight.shape == weight.shape and 
                                self.actor[layer_idx].bias.shape == bias.shape):
                                self.actor[layer_idx].weight.data = weight
                                self.actor[layer_idx].bias.data = bias
                                print(f"✅ Загружен актор слой {layer_idx}")
                                
                except Exception as e:
                    print(f"⚠️ Частичная загрузка весов: {e}")
            
            def forward(self, x):
                # Адаптируем наблюдения
                x = self.obs_adapter(x)
                # Пропускаем через основную сеть
                x = self.mlp_encoder(x)
                return self.actor(x)
        
        return AdaptivePolicy(current_obs_dim, expected_obs_dim, action_dim, state_dict)

    def create_default_policy(self) -> torch.nn.Module:
        """Политика по умолчанию для H1"""
        obs_dim = self.model.nq + self.model.nv
        if self.model.nsensordata > 0:
            obs_dim += self.model.nsensordata
            
        action_dim = self.model.nu
        
        print(f"🔧 Создание политики по умолчанию: {obs_dim} -> {action_dim}")
        
        policy = torch.nn.Sequential(
            torch.nn.Linear(obs_dim, 64),
            torch.nn.ReLU(),
            torch.nn.Linear(64, 32),
            torch.nn.ReLU(),
            torch.nn.Linear(32, action_dim),
            torch.nn.Tanh()
        )
        print("✅ Создана политика по умолчанию для H1")
        return policy
    
    def get_observation(self) -> np.ndarray:
        """Получение наблюдения для H1"""
        observation_parts = []
        
        # Позиции суставов
        if self.model.nq > 0:
            observation_parts.append(self.data.qpos.copy())
        
        # Скорости суставов
        if self.model.nv > 0:
            observation_parts.append(self.data.qvel.copy())
        
        # Данные сенсоров
        if self.model.nsensordata > 0:
            observation_parts.append(self.data.sensordata.copy())
        
        observation = np.concatenate(observation_parts)
        return observation.astype(np.float32)
    
    def policy_step(self, observation: np.ndarray) -> np.ndarray:
        """Получение действия от политики"""
        if self.policy is None:
            return np.zeros(self.model.nu)
        
        with torch.no_grad():
            obs_tensor = torch.FloatTensor(observation).unsqueeze(0)
            action = self.policy(obs_tensor)
            return action.squeeze(0).numpy()
    
    def run_episode(self, num_steps=500, action_scale=0.1):
        """Запуск эпизода тестирования для H1"""
        print(f"🚀 Запуск эпизода H1 на {num_steps} шагов (масштаб: {action_scale})...")
        
        # Сброс симуляции
        mujoco.mj_resetData(self.model, self.data)
        
        # Сохранение данных для анализа
        positions = []
        velocities = []
        actions = []
        rewards = []
        
        for step in range(num_steps):
            # Получение наблюдения
            obs = self.get_observation()
            
            # Получение действия от политики
            action = self.policy_step(obs)
            
            # Применение действия (с масштабированием для безопасности)
            self.data.ctrl[:] = action * action_scale
            
            # Сохранение состояния до шага
            prev_pos = self.data.qpos.copy()
            
            # Шаг симуляции
            mujoco.mj_step(self.model, self.data)
            
            # Сохранение данных
            positions.append(self.data.qpos.copy())
            velocities.append(self.data.qvel.copy())
            actions.append(action.copy())
            
            # Расчет reward
            reward = self.calculate_h1_reward(prev_pos, self.data.qpos)
            rewards.append(reward)
            
            # Периодический вывод
            if step % 100 == 0:
                print(f"📊 Шаг {step}:")
                print(f"   Позиции: {self.data.qpos[:3]}")
                print(f"   Скорости: {self.data.qvel[:3]}")
                print(f"   Действия: {action[:3]}")
                print(f"   Reward: {reward:.3f}")
        
        # Анализ результатов
        self.analyze_h1_results(positions, velocities, actions, rewards)
        
        return np.mean(rewards)
    
    def calculate_h1_reward(self, prev_pos, current_pos):
        """Расчет reward для H1"""
        # Простой reward для тестирования
        if len(current_pos) > 1:
            forward_movement = current_pos[0] - prev_pos[0]
            height_penalty = -abs(current_pos[2] - 0.7) if len(current_pos) > 2 else 0
        else:
            forward_movement = 0
            height_penalty = 0
            
        alive_bonus = 1.0
        return forward_movement * 10 + height_penalty + alive_bonus
    
    def analyze_h1_results(self, positions, velocities, actions, rewards):
        """Анализ результатов для H1"""
        positions = np.array(positions)
        velocities = np.array(velocities)
        actions = np.array(actions)
        rewards = np.array(rewards)
        
        print("\n📈 АНАЛИЗ РЕЗУЛЬТАТОВ H1:")
        print(f"   Средний reward: {np.mean(rewards):.3f}")
        if positions.shape[1] > 0:
            print(f"   Макс. позиция X: {np.max(positions[:, 0]):.3f}")
        print(f"   Средняя скорость: {np.mean(np.abs(velocities)):.3f}")
        print(f"   Среднее действие: {np.mean(np.abs(actions)):.3f}")
        
        # Создание графиков
        self.create_h1_plots(positions, velocities, actions, rewards)
    
    def create_h1_plots(self, positions, velocities, actions, rewards):
        """Создание графиков для H1"""
        fig, axes = plt.subplots(2, 2, figsize=(12, 10))
        
        # График позиций суставов
        for i in range(min(5, positions.shape[1])):
            axes[0, 0].plot(positions[:, i], label=f'Joint {i}')
        axes[0, 0].set_title('Joint Positions')
        axes[0, 0].legend()
        axes[0, 0].grid(True)
        
        # График скоростей
        for i in range(min(5, velocities.shape[1])):
            axes[0, 1].plot(velocities[:, i], label=f'Vel {i}')
        axes[0, 1].set_title('Joint Velocities')
        axes[0, 1].legend()
        axes[0, 1].grid(True)
        
        # График действий
        for i in range(min(5, actions.shape[1])):
            axes[1, 0].plot(actions[:, i], label=f'Action {i}')
        axes[1, 0].set_title('Actions')
        axes[1, 0].legend()
        axes[1, 0].grid(True)
        
        # График rewards
        axes[1, 1].plot(rewards)
        axes[1, 1].set_title('Rewards')
        axes[1, 1].set_ylabel('Reward')
        axes[1, 1].grid(True)
        
        plt.tight_layout()
        plt.savefig('h1_inference_results.png', dpi=150, bbox_inches='tight')
        print("✅ Графики сохранены в h1_inference_results.png")
        
        # Показать графики
        plt.show()



def main():
    print("🧪 H1 MUJOCO INFERENCE TEST")
    print("=" * 50)
    
    # Сначала создадим правильную модель
    model_path = create_proper_h1_model()
    
    # Поиск политики - ДОБАВЛЕН ПРАВИЛЬНЫЙ ПУТЬ
    policy_path = None
    possible_policy_paths = [
        # ПРЯМОЙ ПУТЬ К ВАШЕЙ ПОЛИТИКЕ
        "/home/dzirt/RL/docker_rl/Alpha_Human_gym-main/logs/h1_constraint_trot/Oct09_15-54-01_/model_30000.pt",
        # Относительные пути на случай если запускаете из других мест
        "../logs/h1_constraint_trot/Oct09_15-54-01_/model_30000.pt",
        "../../logs/h1_constraint_trot/Oct09_15-54-01_/model_30000.pt",
        "logs/h1_constraint_trot/Oct09_15-54-01_/model_30000.pt",
        # Другие возможные пути
        "model_30000.pt",
        "policy.pth", 
        "../logs/model.pth",
        "../model.pth",
        "trained_model.pth"
    ]
    
    for path in possible_policy_paths:
        if os.path.exists(path):
            policy_path = path
            print(f"✅ Найдена политика: {path}")
            break
        else:
            print(f"❌ Не найдено: {path}")
    
    if policy_path is None:
        print("❌ Политика не найдена! Проверьте пути:")
        print("   Ожидаемый путь: /home/dzirt/RL/docker_rl/Alpha_Human_gym-main/logs/h1_constraint_trot/Oct09_15-54-01_/model_30000.pt")
        
        # Проверим существование директории
        expected_dir = "/home/dzirt/RL/docker_rl/Alpha_Human_gym-main/logs/h1_constraint_trot/Oct09_15-54-01_"
        if os.path.exists(expected_dir):
            print(f"✅ Директория существует: {expected_dir}")
            files = os.listdir(expected_dir)
            print(f"📁 Файлы в директории: {files}")
        else:
            print(f"❌ Директория не существует: {expected_dir}")
    
    print(f"📁 Модель H1: {model_path}")
    print(f"📁 Политика: {policy_path if policy_path else 'Не найдена, используется случайная'}")
    
    try:
        # Создание тестера
        tester = H1MuJoCoInferenceTest(model_path, policy_path)
        
        # Основной тест
        print("\n" + "="*50)
        print("ТЕСТИРОВАНИЕ H1 В MUJOCO")
        print("="*50)
        
        # Тестирование с разными масштабами действий
        scales = [0.01, 0.05, 0.1]
        results = {}
        
        for scale in scales:
            print(f"\n🎯 Тестирование с масштабом действий: {scale}")
            print("-" * 40)
            reward = tester.run_episode(num_steps=300, action_scale=scale)
            results[scale] = reward
            print(f"📊 Результат: средний reward = {reward:.3f}")
        
        print("\n📋 ИТОГИ ТЕСТИРОВАНИЯ:")
        for scale, reward in results.items():
            print(f"   Масштаб {scale}: reward = {reward:.3f}")
            
    except Exception as e:
        print(f"❌ Ошибка во время тестирования: {e}")
        import traceback
        traceback.print_exc()
    
    print("\n✅ Тестирование H1 завершено!")

if __name__ == "__main__":
    main()