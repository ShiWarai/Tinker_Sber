# correct_policy_test.py
import mujoco
import numpy as np
import torch
import torch.nn as nn
import os
import time
import matplotlib.pyplot as plt

class CorrectH1Policy(nn.Module):
    def __init__(self, obs_dim=43, action_dim=10):
        super().__init__()
        
        # История наблюдений (из чекпоинта видно что есть history_encoder)
        self.history_encoder = nn.Sequential(
            nn.Linear(39, 30),  # из чекпоинта: [30, 39]
            nn.ReLU(),
        )
        
        # Conv layers для истории (если есть временные зависимости)
        self.conv_layers = nn.Sequential(
            nn.Conv1d(30, 20, kernel_size=4),  # из чекпоинта: [20, 30, 4]
            nn.ReLU(),
            nn.Conv1d(20, 10, kernel_size=2),  # из чекпоинта: [10, 20, 2]
            nn.ReLU(),
        )
        
        self.linear_output = nn.Sequential(
            nn.Linear(30, 16),  # из чекпоинта: [16, 30]
            nn.ReLU(),
        )
        
        # Основной MLP энкодер (из actor_teacher_backbone)
        self.mlp_encoder = nn.Sequential(
            nn.Linear(195, 512),  # из чекпоинта: [512, 195]
            nn.ReLU(),
            nn.Linear(512, 256),
            nn.ReLU(), 
            nn.Linear(256, 128),
            nn.ReLU(),
        )
        
        # Actor network
        self.actor = nn.Sequential(
            nn.Linear(128, 64),
            nn.ReLU(),
            nn.Linear(64, 32),
            nn.ReLU(),
            nn.Linear(32, action_dim),
            nn.Tanh()
        )
        
        # Batch normalization (из чекпоинта)
        self.bn = nn.BatchNorm1d(128)
        
        # Стандартное отклонение (из чекпоинта)
        self.std = nn.Parameter(torch.ones(action_dim) * 0.1)
        
    def forward(self, x):
        # Основной проход через MLP
        x = self.mlp_encoder(x)
        x = self.bn(x)
        actions = self.actor(x)
        return actions

class CorrectPolicyTest:
    def __init__(self, model_path: str, policy_path: str):
        self.model_path = model_path
        self.policy_path = policy_path
        
        # Загрузка модели MuJoCo
        print(f"📁 Загрузка модели из {model_path}")
        self.model = mujoco.MjModel.from_xml_path(model_path)
        self.data = mujoco.MjData(self.model)
        
        print(f"✅ Модель H1 загружена:")
        print(f"   Суставы (nq): {self.model.nq}")
        print(f"   Скорости (nv): {self.model.nv}") 
        print(f"   Действия (nu): {self.model.nu}")
        
        # Загрузка политики с правильной архитектурой
        self.policy = self.load_correct_policy(policy_path)
        
    def load_correct_policy(self, policy_path: str) -> nn.Module:
        """Загрузка политики с правильной архитектурой"""
        print("🎯 Создание корректной архитектуры политики...")
        
        # Определяем размерности
        obs_dim = self.model.nq + self.model.nv
        if self.model.nsensordata > 0:
            obs_dim += self.model.nsensordata
            
        action_dim = self.model.nu
        
        print(f"🔧 Создание политики: {obs_dim} -> {action_dim}")
        
        # Создаем политику
        policy = CorrectH1Policy(obs_dim, action_dim)
        
        # Загружаем чекпоинт
        checkpoint = torch.load(policy_path, map_location='cpu')
        
        print("🔄 Загрузка весов в корректную архитектуру...")
        
        # Создаем новый state_dict с правильными именами
        new_state_dict = {}
        
        # Переносим веса, которые можем
        for key, value in checkpoint['model_state_dict'].items():
            # Пропускаем сложные компоненты, берем только основные веса
            if 'actor_teacher_backbone.mlp_encoder' in key:
                # Преобразуем имена
                new_key = key.replace('actor_teacher_backbone.mlp_encoder', 'mlp_encoder')
                new_state_dict[new_key] = value
            elif 'actor_teacher_backbone.actor.w' in key:
                # Преобразуем веса актора
                layer_num = key.split('.')[-1][1:]  # извлекаем номер слоя
                if layer_num == '0':
                    new_key = 'actor.0.weight'
                elif layer_num == '1': 
                    new_key = 'actor.2.weight'
                elif layer_num == '2':
                    new_key = 'actor.4.weight'
                new_state_dict[new_key] = value
            elif 'actor_teacher_backbone.actor.b' in key:
                # Преобразуем bias актора
                layer_num = key.split('.')[-1][1:]
                if layer_num == '0':
                    new_key = 'actor.0.bias'
                elif layer_num == '1':
                    new_key = 'actor.2.bias' 
                elif layer_num == '2':
                    new_key = 'actor.4.bias'
                new_state_dict[new_key] = value
            elif 'std' in key:
                new_state_dict[key] = value
        
        # Загружаем доступные веса
        policy.load_state_dict(new_state_dict, strict=False)
        policy.eval()
        
        print("✅ Политика загружена (частично)")
        return policy
    
    def get_observation(self) -> np.ndarray:
        """Получение наблюдения"""
        observation_parts = []
        
        if self.model.nq > 0:
            observation_parts.append(self.data.qpos.copy())
        if self.model.nv > 0:
            observation_parts.append(self.data.qvel.copy())
        if self.model.nsensordata > 0:
            observation_parts.append(self.data.sensordata.copy())
        
        return np.concatenate(observation_parts).astype(np.float32)
    
    def policy_step(self, observation: np.ndarray) -> np.ndarray:
        """Получение действия от политики"""
        with torch.no_grad():
            obs_tensor = torch.FloatTensor(observation).unsqueeze(0)
            action = self.policy(obs_tensor)
            return action.squeeze(0).numpy()
    
    def run_comprehensive_test(self, num_steps=2000):
        """Комплексный тест обученной политики"""
        print("\n🎯 КОМПЛЕКСНЫЙ ТЕСТ ОБУЧЕННОЙ ПОЛИТИКИ")
        print("=" * 50)
        
        mujoco.mj_resetData(self.model, self.data)
        self.data.qpos[2] = 0.75  # начальная высота
        
        # Статистика
        stats = {
            'height_history': [],
            'stability_streaks': [],
            'actions_std': [],
            'current_streak': 0,
            'max_streak': 0,
            'total_stable_time': 0
        }
        
        for step in range(num_steps):
            obs = self.get_observation()
            action = self.policy_step(obs)
            
            # Применяем действие
            self.data.ctrl[:] = action * 0.1
            
            # Сохраняем состояние до шага
            prev_height = self.data.qpos[2]
            
            # Шаг симуляции
            mujoco.mj_step(self.model, self.data)
            
            current_height = self.data.qpos[2]
            stats['height_history'].append(current_height)
            stats['actions_std'].append(np.std(action))
            
            # Отслеживаем стабильность
            if current_height > 0.5:  # робот стоит
                stats['current_streak'] += 1
                stats['total_stable_time'] += 1
                stats['max_streak'] = max(stats['max_streak'], stats['current_streak'])
            else:
                stats['stability_streaks'].append(stats['current_streak'])
                stats['current_streak'] = 0
            
            # Детальный вывод каждые 200 шагов
            if step % 200 == 0:
                height_change = current_height - prev_height
                action_magnitude = np.linalg.norm(action)
                
                print(f"📊 Шаг {step}:")
                print(f"   Высота: {current_height:.3f} (Δ: {height_change:+.3f})")
                print(f"   Стабильность: {stats['current_streak']} шагов")
                print(f"   Мощность действий: {action_magnitude:.3f}")
                print(f"   Стандартное отклонение: {np.std(action):.3f}")
            
            # Автоматический перезапуск при падении
            if current_height < 0.2:
                if step > 100:  # Не перезапускаем сразу
                    print(f"💥 Падение на шаге {step}. Стабильность: {stats['current_streak']} шагов")
                
                mujoco.mj_resetData(self.model, self.data)
                self.data.qpos[2] = 0.75
                stats['stability_streaks'].append(stats['current_streak'])
                stats['current_streak'] = 0
        
        # Финальная статистика
        stats['stability_streaks'].append(stats['current_streak'])
        
        # Анализ результатов
        self.analyze_comprehensive_results(stats, num_steps)
    
    def analyze_comprehensive_results(self, stats, total_steps):
        """Анализ комплексных результатов"""
        heights = np.array(stats['height_history'])
        stability_streaks = [s for s in stats['stability_streaks'] if s > 0]  # только ненулевые
        
        print("\n📈 ДЕТАЛЬНЫЙ АНАЛИЗ РЕЗУЛЬТАТОВ:")
        print("=" * 50)
        print(f"🎯 ОБЩАЯ СТАТИСТИКА:")
        print(f"   Всего шагов: {total_steps}")
        print(f"   Стабильных шагов: {stats['total_stable_time']} ({stats['total_stable_time']/total_steps*100:.1f}%)")
        print(f"   Макс. непрерывная стабильность: {stats['max_streak']} шагов")
        
        print(f"\n📊 СТАТИСТИКА ВЫСОТЫ:")
        print(f"   Средняя высота: {np.mean(heights):.3f} м")
        print(f"   Медианная высота: {np.median(heights):.3f} м") 
        print(f"   Стандартное отклонение: {np.std(heights):.3f} м")
        print(f"   Минимальная высота: {np.min(heights):.3f} м")
        print(f"   Максимальная высота: {np.max(heights):.3f} м")
        
        print(f"\n🎭 СТАТИСТИКА СТАБИЛЬНОСТИ:")
        if stability_streaks:
            print(f"   Средняя длина стабильности: {np.mean(stability_streaks):.1f} шагов")
            print(f"   Медианная длина стабильности: {np.median(stability_streaks):.1f} шагов")
            print(f"   Количество периодов стабильности: {len(stability_streaks)}")
        else:
            print(f"   Периоды стабильности не обнаружены")
        
        print(f"\n⚡ СТАТИСТИКА ДЕЙСТВИЙ:")
        print(f"   Среднее std действий: {np.mean(stats['actions_std']):.3f}")
        
        # Оценка качества
        stability_score = stats['total_stable_time'] / total_steps
        if stability_score > 0.8:
            rating = "ОТЛИЧНО"
        elif stability_score > 0.6:
            rating = "ХОРОШО" 
        elif stability_score > 0.4:
            rating = "УДОВЛЕТВОРИТЕЛЬНО"
        else:
            rating = "НУЖДАЕТСЯ В ДООБУЧЕНИИ"
        
        print(f"\n🏆 ОЦЕНКА КАЧЕСТВА: {rating}")
        print(f"   Балл стабильности: {stability_score:.3f}")
        
        # Визуализация
        self.plot_comprehensive_results(stats, total_steps)
    
    def plot_comprehensive_results(self, stats, total_steps):
        """Визуализация комплексных результатов"""
        fig, axes = plt.subplots(2, 2, figsize=(15, 12))
        
        heights = np.array(stats['height_history'])
        
        # График высоты с временем
        axes[0, 0].plot(heights, alpha=0.7)
        axes[0, 0].axhline(y=0.5, color='red', linestyle='--', label='Граница стабильности')
        axes[0, 0].axhline(y=0.75, color='green', linestyle='--', label='Целевая высота')
        axes[0, 0].set_title('Динамика высоты робота')
        axes[0, 0].set_xlabel('Шаг')
        axes[0, 0].set_ylabel('Высота (м)')
        axes[0, 0].legend()
        axes[0, 0].grid(True, alpha=0.3)
        
        # Гистограмма высот
        axes[0, 1].hist(heights, bins=50, alpha=0.7, color='skyblue')
        axes[0, 1].axvline(x=0.5, color='red', linestyle='--', label='Граница стабильности')
        axes[0, 1].axvline(x=np.mean(heights), color='orange', linestyle='-', label=f'Среднее: {np.mean(heights):.3f}')
        axes[0, 1].set_title('Распределение высот')
        axes[0, 1].set_xlabel('Высота (м)')
        axes[0, 1].set_ylabel('Частота')
        axes[0, 1].legend()
        axes[0, 1].grid(True, alpha=0.3)
        
        # График стабильности
        stability_streaks = [s for s in stats['stability_streaks'] if s > 0]
        if stability_streaks:
            axes[1, 0].plot(stability_streaks, 'o-', alpha=0.7)
            axes[1, 0].set_title('Длительности периодов стабильности')
            axes[1, 0].set_xlabel('Номер периода')
            axes[1, 0].set_ylabel('Длительность (шаги)')
            axes[1, 0].grid(True, alpha=0.3)
        
        # Статистика
        axes[1, 1].axis('off')
        stability_score = stats['total_stable_time'] / total_steps
        
        text_stats = f"""СТАТИСТИКА ТЕСТА:
        
Общее время: {total_steps} шагов
Стабильное время: {stats['total_stable_time']} шагов
Стабильность: {stability_score:.1%}

Высота:
  Средняя: {np.mean(heights):.3f} м
  Медиана: {np.median(heights):.3f} м  
  Отклонение: {np.std(heights):.3f} м

Стабильность:
  Макс. период: {stats['max_streak']} шагов
  Периодов: {len(stability_streaks)}
  Ср. период: {np.mean(stability_streaks):.1f} шагов

ОЦЕНКА: {'ОТЛИЧНО' if stability_score > 0.8 else 'ХОРОШО' if stability_score > 0.6 else 'УДОВЛЕТВОРИТЕЛЬНО' if stability_score > 0.4 else 'НУЖДАЕТСЯ В ДООБУЧЕНИИ'}"""
        
        axes[1, 1].text(0.1, 0.9, text_stats, fontsize=10, verticalalignment='top', fontfamily='monospace')
        
        plt.tight_layout()
        plt.savefig('h1_comprehensive_test.png', dpi=150, bbox_inches='tight')
        print("✅ Детальные графики сохранены в h1_comprehensive_test.png")
        plt.show()

def main():
    MODEL_PATH = "h1_proper.xml"
    POLICY_PATH = "/home/dzirt/RL/docker_rl/Alpha_Human_gym-main/logs/h1_constraint_trot/Oct09_15-54-01_/model_30000.pt"
    
    print("🧪 КОРРЕКТНЫЙ ТЕСТ ОБУЧЕННОЙ ПОЛИТИКИ H1")
    print("=" * 60)
    print(f"📁 Модель: {MODEL_PATH}")
    print(f"📁 Политика: {POLICY_PATH}")
    print("=" * 60)
    
    try:
        tester = CorrectPolicyTest(MODEL_PATH, POLICY_PATH)
        tester.run_comprehensive_test(num_steps=2000)
        
    except Exception as e:
        print(f"❌ Ошибка: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()