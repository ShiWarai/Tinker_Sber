# Симуляция в MuJoCo

Модель робота и сцена взята из репозитория: https://github.com/Romlose/Tinker_sber_RL

В модель робота Tinker были добавлены ограничения на суставы:

`tinker_range.xml` - сгибает ноги как человек
`tinker_range_inv.xml` - сгибает ноги как кузнечик

Файл `sim_mujoco.py` запускает симуляцию в MuJoCo.

```bash
docker run -it --network host --ipc host \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v /usr/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu \
  --device /dev/dri \
  --group-add video \
  sim2sim:mujoco
```