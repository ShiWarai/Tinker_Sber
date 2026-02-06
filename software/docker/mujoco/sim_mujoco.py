import mujoco
import mujoco.viewer
import os
import time
import numpy as np

# Получаем абсолютный путь к текущей директории
current_dir = os.path.dirname(os.path.abspath(__file__))
xml_path = os.path.join(current_dir, "xml", "world.xml")

print(f"Loading model from: {xml_path}")
model = mujoco.MjModel.from_xml_path(xml_path)
data = mujoco.MjData(model)

print("Starting viewer. Управление в окне MuJoCo: Space — пауза/старт, ESC — выход.")

with mujoco.viewer.launch(model, data) as viewer:
    # Настройка камеры
    viewer.cam.lookat[:] = [0, 0, 0.5]
    viewer.cam.distance = 2.0
    viewer.cam.azimuth = 45

    while viewer.is_running():
        time.sleep(0.01)

print("Viewer closed.")
