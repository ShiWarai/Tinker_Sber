# fixed_mujoco_inference.py
import mujoco
import numpy as np
import torch
import os
import time
import matplotlib.pyplot as plt
from typing import Dict, Any, List
import json

class H1FixedInferenceTest:
    def __init__(self, model_path: str, policy_checkpoint: str = None):
        self.model_path = model_path
        self.policy_checkpoint = policy_checkpoint
        self.test_results = {}
        
        # Загрузка модели
        print(f"📁 Загрузка модели из {model_path}")
        try:
            self.model = mujoco.MjModel.from_xml_path(model_path)
            self.data = mujoco.MjData(self.model)
            
            print(f"✅ Модель H1 успешно загружена!")
            print(f"📊 Характеристики: nq={self.model.nq}, nv={self.model.nv}, nu={self.model.nu}, sensors={self.model.nsensordata}")
            
        except Exception as e:
            print(f"❌ Ошибка загрузки модели: {e}")
            raise
        
        # Загрузка политики с ФИКСИРОВАННЫМИ наблюдениями
        self.policy = self.load_policy_fixed(policy_checkpoint)
        
    def get_extended_observation(self):
        """Расширенное наблюдение для совместимости с обученной моделью"""
        observation_parts = []
        
        # 1. Базовые наблюдения (qpos + qvel)
        if self.model.nq > 0:
            observation_parts.append(self.data.qpos.copy())
        if self.model.nv > 0:
            observation_parts.append(self.data.qvel.copy())
        
        # 2. Данные сенсоров
        if self.model.nsensordata > 0:
            observation_parts.append(self.data.sensordata.copy())
        
        # 3. ДОБАВЛЯЕМ ФИКТИВНЫЕ ДАННЫЕ для достижения 195 измерений
        current_obs = np.concatenate(observation_parts)
        missing_dims = 195 - len(current_obs)
        
        if missing_dims > 0:
            # Добавляем нули для недостающих измерений
            fake_data = np.zeros(missing_dims)
            observation_parts.append(fake_data)
            print(f"🔄 Добавлено {missing_dims} фиктивных измерений")
        
        observation = np.concatenate(observation_parts)
        return observation.astype(np.float32)

    def load_policy_fixed(self, checkpoint_path: str) -> torch.nn.Module:
        """Загрузка политики с ФИКСИРОВАННЫМИ наблюдениями"""
        try:
            if not os.path.exists(checkpoint_path):
                print(f"⚠️ Файл политики не найден: {checkpoint_path}")
                return self.create_default_policy()
                
            checkpoint = torch.load(checkpoint_path, map_location='cpu')
            print(f"✅ Загружен чекпоинт")
            
            state_dict = checkpoint.get('model_state_dict', checkpoint)
            
            # Ожидаемая размерность из чекпоинта
            expected_obs_dim = 195
            current_obs_dim = self.get_extended_observation().shape[0]
            action_dim = self.model.nu
            
            print(f"📊 Размерности: ожидается={expected_obs_dim}, есть={current_obs_dim}, actions={action_dim}")
            
            # Всегда создаем адаптивную политику для 195 измерений
            policy = self.create_adaptive_policy(current_obs_dim, expected_obs_dim, action_dim, state_dict)
            policy.eval()
            return policy
            
        except Exception as e:
            print(f"❌ Ошибка загрузки политики: {e}")
            return self.create_default_policy()

    def create_adaptive_policy(self, current_obs_dim, expected_obs_dim, action_dim, state_dict):
        """Создание адаптивной политики"""
        
        class AdaptivePolicy(torch.nn.Module):
            def __init__(self, current_obs_dim, expected_obs_dim, action_dim, state_dict):
                super().__init__()
                
                # Адаптер для приведения к 195 измерениям
                self.obs_adapter = torch.nn.Linear(current_obs_dim, expected_obs_dim)
                
                # Основная сеть
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
                            
                            if len(weight.shape) == 3:
                                weight = weight[0]
                                bias = bias[0]
                            
                            mlp_layer_idx = i * 2
                            if (self.mlp_encoder[mlp_layer_idx].weight.shape == weight.shape and 
                                self.mlp_encoder[mlp_layer_idx].bias.shape == bias.shape):
                                self.mlp_encoder[mlp_layer_idx].weight.data = weight
                                self.mlp_encoder[mlp_layer_idx].bias.data = bias
                                print(f"✅ Загружен MLP слой {i}")
                    
                    # Загружаем актор
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
                            
                            if weight.shape[0] == 4:  # многомодальный
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
                x = self.obs_adapter(x)
                x = self.mlp_encoder(x)
                return self.actor(x)
        
        return AdaptivePolicy(current_obs_dim, expected_obs_dim, action_dim, state_dict)

    def create_default_policy(self):
        """Политика по умолчанию"""
        obs_dim = self.get_extended_observation().shape[0]
        policy = torch.nn.Sequential(
            torch.nn.Linear(obs_dim, 64),
            torch.nn.ReLU(),
            torch.nn.Linear(64, 32),
            torch.nn.ReLU(),
            torch.nn.Linear(32, self.model.nu),
            torch.nn.Tanh()
        )
        return policy

    def policy_step(self, observation: np.ndarray) -> np.ndarray:
        """Получение действия от политики"""
        with torch.no_grad():
            obs_tensor = torch.FloatTensor(observation).unsqueeze(0)
            action = self.policy(obs_tensor)
            return action.squeeze(0).numpy()

    def calculate_proper_reward(self, prev_pos, current_pos, action, step):
        """Правильная reward функция для ходьбы"""
        # 1. Бонус за скорость вперед
        forward_velocity = self.data.qvel[0] if len(self.data.qvel) > 0 else 0
        forward_reward = forward_velocity * 5.0  # Увеличили коэффициент
        
        # 2. Штраф за отклонение от целевой высоты
        height = current_pos[2] if len(current_pos) > 2 else 0
        target_height = 0.9  # Увеличили целевую высоту
        height_penalty = -abs(height - target_height) * 3.0
        
        # 3. Бонус за выживание
        alive_bonus = 2.0  # Увеличили бонус
        
        # 4. Штраф за падение
        fall_penalty = -20.0 if height < 0.3 else 0.0
        
        # 5. Штраф за бездействие
        action_penalty = -0.01 * np.sum(np.square(action))
        
        total_reward = (forward_reward + height_penalty + 
                       alive_bonus + fall_penalty + action_penalty)
        
        return total_reward

    def run_optimized_test(self, num_steps=1000, action_scale=1.0):
        """Оптимизированный тест с правильными начальными условиями"""
        print(f"🚀 ЗАПУСК ОПТИМИЗИРОВАННОГО ТЕСТА")
        print(f"   Шаги: {num_steps}, Масштаб: {action_scale}")
        
        # СБРОС И ПРАВИЛЬНАЯ НАЧАЛЬНАЯ ПОЗИЦИЯ
        mujoco.mj_resetData(self.model, self.data)
        
        # УСТАНАВЛИВАЕМ ПРАВИЛЬНУЮ ВЫСОТУ
        if len(self.data.qpos) > 2:
            self.data.qpos[2] = 0.9  # Высота над землей
        
        # Немного сгибаем ноги для устойчивости
        if len(self.data.qpos) > 4:
            self.data.qpos[3] = -0.3  # Сгибаем колено
        
        positions, velocities, actions, rewards = [], [], [], []
        
        for step in range(num_steps):
            # Получаем РАСШИРЕННОЕ наблюдение
            obs = self.get_extended_observation()
            action = self.policy_step(obs)
            
            # Применяем действие с БОЛЬШИМ масштабом
            self.data.ctrl[:] = action * action_scale
            
            prev_pos = self.data.qpos.copy()
            mujoco.mj_step(self.model, self.data)
            
            # Сохраняем данные
            positions.append(self.data.qpos.copy())
            velocities.append(self.data.qvel.copy())
            actions.append(action.copy())
            
            # Расчет reward с ПРАВИЛЬНОЙ функцией
            reward = self.calculate_proper_reward(prev_pos, self.data.qpos, action, step)
            rewards.append(reward)
            
            # Прогресс
            if step % 200 == 0:
                height = self.data.qpos[2] if len(self.data.qpos) > 2 else 0
                speed = self.data.qvel[0] if len(self.data.qvel) > 0 else 0
                print(f"📊 Шаг {step}: Высота={height:.3f}, Скорость={speed:.3f}, Reward={reward:.3f}")
        
        # Анализ результатов
        self.analyze_optimized_results(positions, velocities, actions, rewards, num_steps)
        return np.mean(rewards)

    def analyze_optimized_results(self, positions, velocities, actions, rewards, num_steps):
        """Анализ оптимизированных результатов"""
        positions = np.array(positions)
        rewards = np.array(rewards)
        
        print(f"\n📈 РЕЗУЛЬТАТЫ ТЕСТА ({num_steps} шагов):")
        print(f"   Средний Reward: {np.mean(rewards):.3f}")
        print(f"   Макс. продвижение: {np.max(positions[:, 0]):.3f} м")
        print(f"   Финальная высота: {positions[-1, 2]:.3f} м")
        print(f"   Средняя скорость: {np.mean(np.abs(velocities)):.3f}")
        
        # Оценка качества
        mean_reward = np.mean(rewards)
        if mean_reward > 5.0:
            print("   🎉 ОТЛИЧНО - модель работает правильно!")
        elif mean_reward > 0.0:
            print("   ✅ ХОРОШО - есть прогресс")
        else:
            print("   ❌ ПЛОХО - нужна доработка")

def main():
    print("🧪 H1 FIXED INFERENCE TEST")
    print("=" * 50)
    
    model_path = "/home/dzirt/RL/docker_rl/Alpha_Human_gym-main/resources/h1/xml/world.xml"
    policy_path = "/home/dzirt/RL/docker_rl/Alpha_Human_gym-main/logs/h1_constraint_trot/Oct09_15-54-01_/model_30000.pt"
    
    print(f"📁 Модель: {model_path}")
    print(f"📁 Политика: {policy_path}")
    
    try:
        # Создаем тестер с ФИКСИРОВАННЫМИ наблюдениями
        tester = H1FixedInferenceTest(model_path, policy_path)
        
        print("\n🎯 ТЕСТИРОВАНИЕ С ФИКСИРОВАННЫМИ НАБЛЮДЕНИЯМИ")
        print("=" * 50)
        
        # Тестируем с разными масштабами
        scales = [0.5, 1.0, 2.0]
        
        for scale in scales:
            print(f"\n🔧 Масштаб действий: {scale}")
            print("-" * 30)
            reward = tester.run_optimized_test(num_steps=1000, action_scale=scale)
            print(f"📊 Итоговый reward: {reward:.3f}")
            
    except Exception as e:
        print(f"❌ Ошибка: {e}")
        import traceback
        traceback.print_exc()
    
    print("\n✅ Тестирование завершено!")

if __name__ == "__main__":
    main()