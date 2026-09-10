# Legacy: dev-inf (docker sim2sim + inference)

Снимок ветки **`dev-inf`** (tip `a6c179f`, 2026-02-20): inference-контроллер и MuJoCo sim2sim в Docker.

Перенесено в `software/sim/legacy/dev-inf/` с сохранением истории коммитов ветки (merge `-s ours` + checkout).

## Содержимое

| Путь | Назначение |
|---|---|
| `controller/` | RL inference controller в Docker, своя копия `tinker_msgs`, ONNX/PT модели |
| `mujoco/` | MuJoCo twin (`sim_mujoco.py`), Dockerfile |
| `jetson/` | Docker для Jetson (копия `software/docker/jetson/`) |

## Актуальный стек

Для работы с unified API используйте **`software/sim/mujoco/export_inference/`** и `software/ros2/src/tinker_msgs/`.

Эта папка — архив docker-окружения Kirill (feb 2026), на случай если понадобятся Dockerfile или старые веса.
