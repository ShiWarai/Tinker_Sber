# Симуляция в MuJoCo

Модель робота и сцена взята из репозитория: https://github.com/Romlose/Tinker_sber_RL

В модель робота Tinker были добавлены ограничения на суставы:

`tinker_range.xml` - сгибает ноги как человек
`tinker_range_inv.xml` - сгибает ноги как кузнечик

Файл `sim_mujoco.py` запускает симуляцию в MuJoCo.
