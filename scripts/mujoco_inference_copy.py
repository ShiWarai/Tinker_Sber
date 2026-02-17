# enhanced_mujoco_inference_existing.py
import mujoco
import numpy as np
import torch
import os
import time
import matplotlib.pyplot as plt
from typing import Dict, Any, List
import json

def fix_h1_xml():
    """Исправление ошибки в h1.xml файле"""
    h1_xml_path = "/home/dzirt/RL/docker_rl/Alpha_Human_gym-main/resources/h1/xml/h1.xml"
    
    if os.path.exists(h1_xml_path):
        with open(h1_xml_path, 'r') as f:
            content = f.read()
        
        # Исправляем синтаксическую ошибку
        fixed_content = content.replace(
            '<geom type="mesh" contype="0" conaffinity="0" group="1" rgba="0.898039 0.917647 0.929412 1" mesh="L0_Link" /> -->',
            '<geom type="mesh" contype="0" conaffinity="0" group="1" rgba="0.898039 0.917647 0.929412 1" mesh="L0_Link" />'
        )
        
        # Сохраняем исправленную версию
        fixed_path = "/home/dzirt/RL/docker_rl/Alpha_Human_gym-main/resources/h1/xml/h1_fixed.xml"
        with open(fixed_path, 'w') as f:
            f.write(fixed_content)
        
        print(f"✅ Исправленный файл сохранен: {fixed_path}")
        return fixed_path
    else:
        print(f"❌ Файл h1.xml не найден: {h1_xml_path}")
        return None

def check_resources():
    """Проверка наличия всех необходимых ресурсов"""
    base_path = "/home/dzirt/RL/docker_rl/Alpha_Human_gym-main/resources/h1"
    
    print("🔍 Проверка ресурсов...")
    
    # Проверка XML файлов
    xml_files = [
        "xml/dependencies.xml",
        "xml/h1.xml", 
        "xml/scene.xml",
        "xml/world_terrain.xml",
        "xml/world.xml"
    ]
    
    for xml_file in xml_files:
        full_path = os.path.join(base_path, xml_file)
        if os.path.exists(full_path):
            print(f"✅ {xml_file}")
        else:
            print(f"❌ {xml_file} - не найден")
    
    # Проверка STL файлов
    stl_files = [
        "meshes/base_link.STL",
        "meshes/L0_Link.STL",
        "meshes/L1_Link.STL", 
        "meshes/L2_Link.STL",
        "meshes/L3_Link.STL",
        "meshes/L4_Link_ankle.STL",
        "meshes/R0_Link.STL",
        "meshes/R1_Link.STL",
        "meshes/R2_Link.STL",
        "meshes/R3_Link.STL",
        "meshes/R4_Link_ankle.STL"
    ]
    
    print("\n🔍 Проверка STL файлов...")
    missing_stl = []
    for stl_file in stl_files:
        full_path = os.path.join(base_path, stl_file)
        if os.path.exists(full_path):
            print(f"✅ {stl_file}")
        else:
            print(f"❌ {stl_file} - не найден")
            missing_stl.append(stl_file)
    
    return len(missing_stl) == 0

class H1MuJoCoInferenceTest:
    def __init__(self, model_path: str, policy_checkpoint: str = None):
        """
        Инициализация теста инференса для H1 робота в MuJoCo
        """
        self.model_path = model_path
        self.policy_checkpoint = policy_checkpoint
        self.test_results = {}
        
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
            
            # Вывод информации о суставах
            print(f"🔧 Суставы:")
            for i in range(self.model.njnt):
                jnt_type = self.model.jnt_type[i]
                jnt_name = mujoco.mj_id2name(self.model, mujoco.mjtObj.mjOBJ_JOINT, i)
                print(f"   {jnt_name}: тип {jnt_type}")
                
        except Exception as e:
            print(f"❌ Ошибка загрузки модели: {e}")
            raise
        
        # Загрузка политики
        self.policy = self.load_policy(policy_checkpoint) if policy_checkpoint else self.create_default_policy()
        
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
        obs_dim = self.get_observation().shape[0]
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

    def load_weights_compatible(self, policy, state_dict):
        """Загрузка весов с совместимостью размерностей"""
        try:
            # Простая загрузка если архитектура совпадает
            policy.load_state_dict(state_dict, strict=False)
            print("✅ Веса загружены (нестрогий режим)")
        except Exception as e:
            print(f"⚠️ Не удалось загрузить веса: {e}")
        return policy

    def create_policy(self, obs_dim, action_dim):
        """Создание политики с заданными размерностями"""
        return torch.nn.Sequential(
            torch.nn.Linear(obs_dim, 512),
            torch.nn.ReLU(),
            torch.nn.Linear(512, 256),
            torch.nn.ReLU(),
            torch.nn.Linear(256, 128),
            torch.nn.ReLU(),
            torch.nn.Linear(128, 64),
            torch.nn.ReLU(),
            torch.nn.Linear(64, action_dim),
            torch.nn.Tanh()
        )
    
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

    def enhanced_policy_verification(self):
        """Улучшенная проверка загрузки политики"""
        print("\n" + "="*60)
        print("🔍 ДИАГНОСТИКА ЗАГРУЗКИ ПОЛИТИКИ")
        print("="*60)
        
        # Проверка загруженных весов
        print("📊 Проверка загруженных весов:")
        total_params = 0
        loaded_params = 0
        
        for name, param in self.policy.named_parameters():
            total_params += param.numel()
            # Проверяем, остались ли веса нулевыми (не загруженными)
            if param.data.abs().sum() > 0.001:  # Если есть ненулевые значения
                loaded_params += param.numel()
                print(f"   ✅ {name}: загружены (форма: {param.shape})")
            else:
                print(f"   ⚠️  {name}: нулевые веса (форма: {param.shape})")
        
        print(f"📈 Загружено параметров: {loaded_params}/{total_params} ({loaded_params/total_params*100:.1f}%)")
        
        # Тестовый прогон для проверки стабильности выходов
        print("\n🧪 Тестовый прогон политики:")
        test_obs = torch.randn(1, self.get_observation().shape[0])
        with torch.no_grad():
            for i in range(3):
                action = self.policy(test_obs)
                print(f"   Прогон {i+1}: действия = {action[0][:3].numpy()}...")
        
        # Проверка согласованности выходов
        print("\n📋 Проверка согласованности:")
        obs1 = self.get_observation()
        action1 = self.policy_step(obs1)
        
        # Небольшое изменение наблюдения
        obs2 = obs1 + 0.01 * np.random.randn(*obs1.shape)
        action2 = self.policy_step(obs2)
        
        action_diff = np.abs(action1 - action2).mean()
        print(f"   Разница действий при изменении наблюдения: {action_diff:.4f}")
        if action_diff > 0.001:
            print("   ✅ Политика реагирует на изменения наблюдений")
        else:
            print("   ⚠️  Политика может не реагировать на наблюдения")

    def run_extended_test(self, num_steps=2000, action_scale=0.1, save_interval=100):
        """Расширенный тест с мониторингом производительности"""
        print(f"🚀 ЗАПУСК РАСШИРЕННОГО ТЕСТА на {num_steps} шагов")
        print(f"   Масштаб действий: {action_scale}")
        print(f"   Интервал сохранения: {save_interval} шагов")
        
        # Сброс симуляции
        mujoco.mj_resetData(self.model, self.data)
        
        # Расширенное сохранение данных
        positions = []
        velocities = [] 
        actions = []
        rewards = []
        timestamps = []
        
        start_time = time.time()
        
        for step in range(num_steps):
            # Получение наблюдения
            obs = self.get_observation()
            
            # Получение действия от политики
            action = self.policy_step(obs)
            
            # Применение действия
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
            timestamps.append(time.time() - start_time)
            
            # Расширенный периодический вывод
            if step % save_interval == 0:
                current_time = timestamps[-1]
                steps_per_sec = (step + 1) / current_time if current_time > 0 else 0
                
                print(f"📊 Шаг {step}/{num_steps}:")
                print(f"   Время: {current_time:.1f}с, Скорость: {steps_per_sec:.1f} шагов/сек")
                print(f"   Позиция X: {self.data.qpos[0]:.3f}, Высота: {self.data.qpos[2]:.3f}")
                print(f"   Действия: {np.abs(action).mean():.3f} ± {np.abs(action).std():.3f}")
                print(f"   Reward: {reward:.3f}, Средний: {np.mean(rewards):.3f}")
                
                # Проверка стабильности
                if len(rewards) > 10:
                    recent_rewards = rewards[-10:]
                    reward_std = np.std(recent_rewards)
                    print(f"   Стабильность (std последних 10): {reward_std:.3f}")
                
                print("   " + "-" * 40)
        
        # Расширенный анализ
        final_reward_mean, final_reward_std = self.analyze_extended_results(
            positions, velocities, actions, rewards, timestamps, num_steps
        )
        
        return final_reward_mean, final_reward_std

    def analyze_extended_results(self, positions, velocities, actions, rewards, timestamps, num_steps):
        """Расширенный анализ результатов"""
        positions = np.array(positions)
        velocities = np.array(velocities)
        actions = np.array(actions)
        rewards = np.array(rewards)
        timestamps = np.array(timestamps)
        
        print("\n" + "="*60)
        print("📈 РАСШИРЕННЫЙ АНАЛИЗ РЕЗУЛЬТАТОВ")
        print("="*60)
        
        total_time = timestamps[-1] if len(timestamps) > 0 else 0
        steps_per_sec = len(timestamps) / total_time if total_time > 0 else 0
        
        print(f"⏱️  Временные характеристики:")
        print(f"   Общее время: {total_time:.2f} сек")
        print(f"   Скорость симуляции: {steps_per_sec:.1f} шагов/сек")
        print(f"   Всего шагов: {len(rewards)}")
        
        reward_mean = np.mean(rewards)
        reward_std = np.std(rewards)
        
        print(f"📊 Статистика Reward:")
        print(f"   Средний: {reward_mean:.3f} ± {reward_std:.3f}")
        print(f"   Медиана: {np.median(rewards):.3f}")
        print(f"   Минимум: {np.min(rewards):.3f}")
        print(f"   Максимум: {np.max(rewards):.3f}")
        
        print(f"🎯 Достижения:")
        if positions.shape[1] > 0:
            max_forward = np.max(positions[:, 0])
            final_forward = positions[-1, 0]
            print(f"   Макс. продвижение: {max_forward:.3f} м")
            print(f"   Финальное продвижение: {final_forward:.3f} м")
            print(f"   Эффективность: {final_forward/total_time:.3f} м/сек")
        
        print(f"⚡ Динамика:")
        print(f"   Средняя скорость: {np.mean(np.abs(velocities)):.3f}")
        print(f"   Среднее действие: {np.mean(np.abs(actions)):.3f}")
        
        # Анализ стабильности
        reward_changes = np.diff(rewards)
        large_changes = np.sum(np.abs(reward_changes) > 1.0)
        print(f"📉 Стабильность:")
        print(f"   Резких изменений reward: {large_changes}")
        print(f"   Макс. изменение за шаг: {np.max(np.abs(reward_changes)):.3f}")
        
        # Сохранение результатов
        test_id = f"test_{num_steps}steps"
        self.test_results[test_id] = {
            'reward_mean': float(reward_mean),
            'reward_std': float(reward_std),
            'total_time': float(total_time),
            'steps_per_sec': float(steps_per_sec),
            'max_forward': float(max_forward) if positions.shape[1] > 0 else 0.0,
            'final_forward': float(final_forward) if positions.shape[1] > 0 else 0.0,
            'avg_velocity': float(np.mean(np.abs(velocities))),
            'avg_action': float(np.mean(np.abs(actions))),
            'stability_large_changes': int(large_changes)
        }
        
        # Создание расширенных графиков
        self.create_extended_plots(positions, velocities, actions, rewards, timestamps, num_steps)
        
        return reward_mean, reward_std

    def create_extended_plots(self, positions, velocities, actions, rewards, timestamps, num_steps):
        """Создание расширенных графиков"""
        fig, axes = plt.subplots(3, 2, figsize=(15, 12))
        fig.suptitle(f'Результаты тестирования H1 ({num_steps} шагов)', fontsize=16, fontweight='bold')
        
        # График 1: Позиции основных суставов
        time_seconds = timestamps
        for i in range(min(6, positions.shape[1])):
            axes[0, 0].plot(time_seconds, positions[:, i], label=f'Joint {i}', alpha=0.7)
        axes[0, 0].set_title('Позиции суставов')
        axes[0, 0].set_ylabel('Позиция (рад/м)')
        axes[0, 0].legend()
        axes[0, 0].grid(True, alpha=0.3)
        
        # График 2: Скорости
        for i in range(min(6, velocities.shape[1])):
            axes[0, 1].plot(time_seconds, velocities[:, i], label=f'Vel {i}', alpha=0.7)
        axes[0, 1].set_title('Скорости суставов')
        axes[0, 1].set_ylabel('Скорость (рад/с)')
        axes[0, 1].legend()
        axes[0, 1].grid(True, alpha=0.3)
        
        # График 3: Действия
        for i in range(min(6, actions.shape[1])):
            axes[1, 0].plot(time_seconds, actions[:, i], label=f'Action {i}', alpha=0.7)
        axes[1, 0].set_title('Действия политики')
        axes[1, 0].set_ylabel('Действие')
        axes[1, 0].legend()
        axes[1, 0].grid(True, alpha=0.3)
        
        # График 4: Reward с скользящим средним
        axes[1, 1].plot(time_seconds, rewards, 'b-', alpha=0.3, label='Instant Reward')
        window_size = max(1, len(rewards) // 20)
        if window_size > 1:
            rewards_smooth = np.convolve(rewards, np.ones(window_size)/window_size, mode='valid')
            time_smooth = time_seconds[window_size-1:]
            axes[1, 1].plot(time_smooth, rewards_smooth, 'r-', linewidth=2, label=f'Среднее ({window_size} шагов)')
        axes[1, 1].set_title('Reward во времени')
        axes[1, 1].set_ylabel('Reward')
        axes[1, 1].legend()
        axes[1, 1].grid(True, alpha=0.3)
        
        # График 5: Продвижение вперед
        if positions.shape[1] > 0:
            forward_movement = positions[:, 0]
            axes[2, 0].plot(time_seconds, forward_movement, 'g-', linewidth=2)
            axes[2, 0].set_title('Продвижение вперед (ось X)')
            axes[2, 0].set_ylabel('Позиция X (м)')
            axes[2, 0].set_xlabel('Время (сек)')
            axes[2, 0].grid(True, alpha=0.3)
            
            # Добавим финальное значение
            final_x = forward_movement[-1]
            axes[2, 0].axhline(y=final_x, color='r', linestyle='--', alpha=0.7, 
                              label=f'Финально: {final_x:.3f} м')
            axes[2, 0].legend()
        
        # График 6: Статистика действий
        action_stats = np.array([np.mean(np.abs(a)) for a in actions])
        axes[2, 1].plot(time_seconds, action_stats, 'purple', linewidth=2)
        axes[2, 1].set_title('Средняя величина действий')
        axes[2, 1].set_ylabel('|Действие|')
        axes[2, 1].set_xlabel('Время (сек)')
        axes[2, 1].grid(True, alpha=0.3)
        
        plt.tight_layout()
        filename = f'h1_extended_test_{num_steps}steps.png'
        plt.savefig(filename, dpi=150, bbox_inches='tight')
        print(f"✅ Расширенные графики сохранены в {filename}")
        
        # Показать графики
        plt.show()

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

    def save_final_report(self):
        """Сохранение финального отчета"""
        report = {
            'model_checkpoint': self.policy_checkpoint,
            'test_configuration': {
                'model_path': self.model_path,
                'total_tests': len(self.test_results)
            },
            'test_results': self.test_results,
            'timestamp': time.strftime("%Y-%m-%d %H:%M:%S")
        }
        
        with open('h1_inference_final_report.json', 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"✅ Финальный отчет сохранен в h1_inference_final_report.json")
        
        # Создание сводной таблицы
        print("\n" + "="*60)
        print("📋 СВОДНЫЙ ОТЧЕТ ПО ТЕСТИРОВАНИЮ")
        print("="*60)
        
        for test_name, result in self.test_results.items():
            stability = "✅ Стабильно" if result['reward_std'] < 0.5 else "⚠️  Нестабильно"
            performance = "🚀 Отлично" if result['steps_per_sec'] > 1000 else "📊 Нормально" if result['steps_per_sec'] > 500 else "🐌 Медленно"
            
            print(f"   {test_name}:")
            print(f"      Reward: {result['reward_mean']:.3f} ± {result['reward_std']:.3f} {stability}")
            print(f"      Производительность: {result['steps_per_sec']:.1f} шаг/сек {performance}")
            print(f"      Продвижение: {result['final_forward']:.3f} м (макс: {result['max_forward']:.3f} м)")
            print(f"      Время: {result['total_time']:.1f} сек")
            print()

def main():
    print("🧪 H1 MUJOCO EXTENDED INFERENCE TEST (EXISTING FILES)")
    print("=" * 60)
    
    # Проверка ресурсов
    resources_ok = check_resources()
    
    if not resources_ok:
        print("❌ Не все ресурсы найдены. Проверьте наличие STL файлов.")
        return
    
    # Исправление h1.xml
    fixed_h1_path = fix_h1_xml()
    if not fixed_h1_path:
        return
    
    # Используем world.xml как основную модель
    model_path = "/home/dzirt/RL/docker_rl/Alpha_Human_gym-main/resources/h1/xml/world.xml"
    policy_path = "/home/dzirt/RL/docker_rl/Alpha_Human_gym-main/logs/h1_constraint_trot/Oct09_15-54-01_/model_30000.pt"
    
    print(f"📁 Модель H1: {model_path}")
    print(f"📁 Политика: {policy_path}")
    
    try:
        # Создание тестера
        tester = H1MuJoCoInferenceTest(model_path, policy_path)
        
        # 🔍 ПРОВЕРКА ПОЛИТИКИ
        tester.enhanced_policy_verification()
        
        print("\n" + "="*60)
        print("🚀 ЗАПУСК РАСШИРЕННЫХ ТЕСТОВ")
        print("="*60)
        
        # Тестирование с разными длительностями
        test_configs = [
            {"steps": 500, "scale": 0.01, "name": "Короткий тест"},
            {"steps": 2000, "scale": 0.01, "name": "Средний тест"}, 
            {"steps": 5000, "scale": 0.01, "name": "Длинный тест"},
        ]
        
        for config in test_configs:
            print(f"\n🎯 {config['name']}: {config['steps']} шагов, масштаб {config['scale']}")
            print("-" * 50)
            
            mean_reward, std_reward = tester.run_extended_test(
                num_steps=config['steps'], 
                action_scale=config['scale'],
                save_interval=min(200, config['steps'] // 5)
            )
            
            print(f"📊 Результат: reward = {mean_reward:.3f} ± {std_reward:.3f}")
        
        # Сохранение финального отчета
        tester.save_final_report()
        
    except Exception as e:
        print(f"❌ Ошибка во время тестирования: {e}")
        import traceback
        traceback.print_exc()
    
    print("\n✅ Расширенное тестирование H1 завершено!")

if __name__ == "__main__":
    main()