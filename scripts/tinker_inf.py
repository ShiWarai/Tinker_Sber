import mujoco
import numpy as np
from scipy.optimize import minimize

def ik_for_biped(xml_path):
    # Загрузка модели
    try:
        model = mujoco.MjModel.from_xml_path(xml_path)
        data = mujoco.MjData(model)
        print("✅ Модель успешно загружена из", xml_path)
    except Exception as e:
        print("❌ Ошибка загрузки модели:", e)
        return

    # Целевые позиции для стоп
    target_pos_left = np.array([0.1, 0.05, -0.3])  # Позиция левой стопы
    target_pos_right = np.array([0.1, -0.05, -0.3])  # Позиция правой стопы

    # Сайты для IK
    site_left = "left_ankle_site"
    site_right = "right_ankle_site"

    # Суставы для каждой ноги
    joint_names_left = ["joint_l_yaw", "joint_l_roll", "joint_l_pitch", "joint_l_knee", "joint_l_ankle"]
    joint_names_right = ["joint_r_yaw", "joint_r_roll", "joint_r_pitch", "joint_r_knee", "joint_r_ankle"]

    # Проверка наличия суставов и сайтов
    try:
        for joint in joint_names_left + joint_names_right:
            if model.joint_name2id(joint) < 0:
                print(f"❌ Ошибка: сустав {joint} не найден в модели")
                return
        for site in [site_left, site_right]:
            if model.site_name2id(site) < 0:
                print(f"❌ Ошибка: сайт {site} не найден в модели")
                return
    except Exception as e:
        print(f"❌ Ошибка проверки суставов/сайтов: {e}")
        return

    # Функция потерь для IK
    def ik_loss(qpos, site_id, target_pos):
        for i, joint_name in enumerate(joint_names_left if site_id == model.site_name2id("left_ankle_site") else joint_names_right):
            joint_id = model.joint_name2id(joint_name)
            data.qpos[joint_id] = qpos[i]
        mujoco.mj_forward(model, data)
        current_pos = data.site_xpos[site_id]
        return np.linalg.norm(current_pos - target_pos)

    # Начальные углы
    q0_left = [data.qpos[model.joint_name2id(j)] for j in joint_names_left]
    q0_right = [data.qpos[model.joint_name2id(j)] for j in joint_names_right]

    # Границы суставов (из tinker_range.xml)
    bounds_left = [(-0.66, 0.66), (-0.57, 0.57), (-1.57, 0.57), (0, 2.57), (-1.57, 0.57)]
    bounds_right = [(-0.66, 0.66), (-0.57, 0.57), (-1.57, 0.57), (0, 2.57), (-1.57, 0.57)]

    # Проверка достижимости целевых позиций
    def check_reachability(target_pos, site_id):
        # Оценка максимальной длины ноги
        leg_segments = [0.054965, 0.14738, 0.14049]  # Длины звеньев из XML
        max_reach = sum(leg_segments)
        base_pos = np.array([0.0025357, 0.053301 if site_id == model.site_name2id("left_ankle_site") else -0.053425, -0.15])
        distance = np.linalg.norm(target_pos - base_pos)
        if distance > max_reach:
            print(f"⚠️ Целевая позиция {target_pos} недостижима для сайта {site_id}. Максимальная длина: {max_reach}")
            return False
        return True

    if not check_reachability(target_pos_left, model.site_name2id("left_ankle_site")):
        print("❌ IK для левой ноги прерван из-за недостижимости")
        return
    if not check_reachability(target_pos_right, model.site_name2id("right_ankle_site")):
        print("❌ IK для правой ноги прерван из-за недостижимости")
        return

    # IK для левой ноги
    try:
        result_left = minimize(
            fun=lambda q: ik_loss(q, model.site_name2id("left_ankle_site"), target_pos_left),
            x0=q0_left,
            method="SLSQP",
            bounds=bounds_left,
            options={"maxiter": 100, "disp": False}
        )
        if result_left.success:
            for i, joint_name in enumerate(joint_names_left):
                joint_id = model.joint_name2id(joint_name)
                data.qpos[joint_id] = result_left.x[i]
            print("✅ Левая нога: Успех! qpos =", result_left.x.round(4), "Ошибка =", result_left.fun)
        else:
            print("⚠️ Левая нога: Не удалось найти решение IK")
    except Exception as e:
        print(f"❌ Ошибка IK для левой ноги: {e}")
        return

    mujoco.mj_forward(model, data)

    # IK для правой ноги
    try:
        result_right = minimize(
            fun=lambda q: ik_loss(q, model.site_name2id("right_ankle_site"), target_pos_right),
            x0=q0_right,
            method="SLSQP",
            bounds=bounds_right,
            options={"maxiter": 100, "disp": False}
        )
        if result_right.success:
            for i, joint_name in enumerate(joint_names_right):
                joint_id = model.joint_name2id(joint_name)
                data.qpos[joint_id] = result_right.x[i]
            print("✅ Правая нога: Успех! qpos =", result_right.x.round(4), "Ошибка =", result_right.fun)
        else:
            print("⚠️ Правая нога: Не удалось найти решение IK")
    except Exception as e:
        print(f"❌ Ошибка IK для правой ноги: {e}")
        return

    mujoco.mj_forward(model, data)

    # Визуализация
    try:
        renderer = mujoco.Renderer(model)
        renderer.update_scene(data)
        pixels = renderer.render()
        print("✅ Рендеринг сцены успешен")
        mujoco.viewer.launch(model, data)
    except Exception as e:
        print(f"❌ Ошибка визуализации: {e}")

if __name__ == "__main__":
    ik_for_biped("tinker_range.xml")