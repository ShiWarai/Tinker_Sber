# Tinker

<div align="center">
<img src="https://github.com/Yuexuan9/Tinker/raw/main/docs/images/t01.JPG" height="300" />
</div>

## 🔍 Навигация

- [О проекте](#о-проекте)
- [Работа с репозиторием](#работа-с-репозиторием)
- [Параметры робота](#параметры-робота)
- [Обновления](#обновления)
- [Видеоматериалы](#видео)

---

## О проекте

Данный проект является адаптацией и продолжением исходной [работы](https://github.com/Yuexuan9/Tinker/tree/main).

`Tinker` — _open-source_ двуногий мини-робот, созданный для энтузиастов и разработчиков-робототехников. Это значит, что можно не просто самостоятельно собрать робота, но еще и модифицировать его под свои задачи и задумки, проявляя волю своего инженерного воображения. _Ведь лучший учитель - это практика!_

Данный репозиторий предоставляет полный список необходимых аппаратных компонентов, программное обеспечение и пошаговые инструкции по сборке, чтобы каждый мог легко воспроизвести всю работу и собрать своего робота.

---

## Работа с репозиторием

### Устройство проекта

```
Tinker/           # Директория проекта
   ├── assets/     # изображения
   ├── assemble/     # сборка
   │   ├── 3d_models/   # 3d-модели, файлы для печати
   │   ├── electronics/ # электрические схемы и схемы подключения
   │   └── images/      # хранилище фото для инструкции
   │   └── README.md/      # инструкция по сборке
   ├── software/     # Программное обеспечение
   │   ├── docker/      # программы для работы с ПК
   │   ├── ros2/      # основной контроллер низкого уровня
   │   ├── sim/      # симуляция
   │   └── firmware/         # контроллер двигателей
   └── README.md     # Основная документация
```

### Работа с ветками

   Иерархия веток проекта:

- `main`
  - `dev`
    - `dev-assemble`
    - `dev-software-docker`
    - `dev-software-ros2`
      - `dev-software-ros2-gui`
      - `dev-software-ros2-node`
    - `dev-software-sim2sim`
    - `dev-software-mujoco`
    - `dev-software-firmware`

  > Ветка `dev-software` архивирована в тег `archive/dev-software-2026-02` и больше не используется как промежуточный узел. Рабочие ветки sim2sim и mujoco сливаются напрямую в `dev`.

- **`main`** - содержит последнюю стабильную сборку проекта

  Слияние в ветку `main` в общем случае возможно **только** из ветки `dev`

  Коммиты возможны **только** для изменений в общей структуре репозитория

  **Не может** служить корнем для любых других веток, кроме `dev`
- **`dev`** - текущая рабочая ветка

  Является сборкой из всех **работоспособных** нижестоящих по иерархии веток.

  Необходима для общего контроля и апробации совместной работы модулей.

- Ветки **`dev-*`** - ветки для контроля одноименных директорий

  **Не являются** рабочими ветками, как и все вышестоящие -
  в общем случае исключены любые прямые коммиты изменений в модулях.

  Служат узлами для нижестоящих **рабочих** веток.

  > Например, ветка `dev-assemble-` не должна содержать изменений в любых директориях `sim/` или `firmware/`

- Ветки **`dev-*-taskname`** - основные рабочие ветки.

  В рамках данных веток нужно работать строго в соответствущей папке, если конечно это не относится к коренному изменению структуры системы. Именно в таких ветках и требуется вести разработку.

Описанные выше правила необходимы для успешной организации совместной работы в общем репозитории.

Соблюдение правил создания и слияние веток обязательно!
Это **важно** для возможности проследить за историей изменений репозитория, а также для оперативного решения конфликтов слияния

> Запросы на слияние и внесение изменений в вышестоящие по иерархии ветки не будут одобряться при потенциальном нарушении правил работы - `это может привести к серьезным конфликтам при сборке проекта!` Просьба отнестись ответственно и с пониманием :)

### Коммиты и слияние веток

Коммиты также лучше приводить к стандартному виду. Название коммита должно иметь вначале названия указатель категории изменения (feat, fix, build, test, misc, docs, merge), а далее краткое и лаконичное описание на русском языке. Подробное описание опционально.

> Как пример, название выглядеть коммита может быть такое: `Fix: исправил потерю пакетов`. Описание коммита: `Добавил подтверждение получение пакета через счётчик пакетов, возвращаемый отправителю`

После создания первого коммита в своей ветке у вас появится возможность создать **запрос на слияние**. Лучше создать его сразу, чтобы в дальнейшем было легче вести обзор внесённых изменений. Для подтвержения слияния нужно через уведомление назначенному проверяющему получить разрешение. В дальшнейшем он или вы можете произвести слияние. Работы по изменению веткок main ведут только руководители разработок.

### Ссылки на референсные проект:

- [Guide](https://github.com/Yuexuan9/Tinker/tree/main/guide)
- [BOM](https://github.com/Yuexuan9/Tinker/tree/main/bom)
- [Assemble manual](https://github.com/Yuexuan9/Tinker/tree/main/assemble)
- [Development](https://github.com/Yuexuan9/Tinker/tree/main/development)

---

## Параметры робота

### Характеристики

- Номинальная скорость передвижения: __ ? __ м/с
- Грузоподъемность: __ ? __ кг
- Время автономной работы: до __ ? __ часов
  - Параметры аккумулятора: 21.6V / 5Ah / 108Wh

### Устройства

- Контроллеры:
  - NVIDIA Jetson Nano
  - [Odroid C4](https://wiki.odroid.com/odroid-c4/odroid-c4#odroid-c4) или [Raspberry Pi 4](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/)
  - STM32 плата
- Интерфейсы:
  - Wi-Fi
  - Ethernet

---

## Видео

YouTube:
[![Tinker Demo](https://img.youtube.com/vi/nC0g2TXLNzI/0.jpg)](https://youtu.be/nC0g2TXLNzI)

bilibili:
[![Китайская версия](https://github.com/Yuexuan9/Tinker/raw/main/docs/images/videos/24fp.png)](https://b23.tv/GL5qTvX)

[Другие ролики](https://youtu.be/ASK_Aj-35oE)

---

## Архивные теги (`archive/*`)

Удалённые ветки сохранены аннотированными тегами. Восстановить ветку:

```bash
git checkout -b <имя-ветки> archive/<имя-тега>
```

### `software/` — ROS 2, sim, inference

| Тег | Бывшая ветка | Tip | Содержимое |
|---|---|---|---|
| `archive/ros2-node-unified_messages` | `ros2-node/unified_messages` | `07e6bfb` | `motor_control`, unified `tinker_msgs` |
| `archive/dev-software-ros2-unified` | `dev-software-ros2-unified` | `232ec66` | интеграция ROS2-стека в `software/ros2/src/` |
| `archive/ros2-gui-unified_messages` | `ros2-gui/unified_messages` | `c3ad921` | `tinker_gui`, `tinker_joy` |
| `archive/dev-software-ros2-gui-buttons` | `dev-software-ros2-gui-buttons` | `867b7a0` | `button_control` |
| `archive/dev-software-ros2-laptop-node` | `dev-software-ros2-laptop-node` | `4a382eb` | `test_talker` |
| `archive/dev-software-mujoco` | `dev-software-mujoco` | `8423e3f` | MuJoCo Playground, `export_inference` |
| `archive/dev-inf` | `dev-inf` | `a6c179f` | docker inference + sim2sim (→ `software/sim/legacy/dev-inf/`) |
| `archive/dev-software-sim2sim` | `dev-software-sim2sim` | `ea45f40` | docker sim2sim (#18), подмножество `dev-inf` |
| `archive/dev-software-simGym` | `dev-software-simGym` | `bc98cb8` | Isaac Gym + Docker (`software/sim/gym/`) |
| `archive/dev-software-2026-02` | `dev-software` | `a6c179f` | umbrella-ветка (feb 2026) |
| `archive/dev-software-controller` | `dev-software-controller` | `f77642b` | docker inference controller (step 1) |

### Сборка, прошивка, CAD

| Тег | Бывшая ветка | Tip | Содержимое |
|---|---|---|---|
| `archive/assemble` | `assemble` | `c5d1924` | инструкция сборки → сейчас `dev-assemble` |
| `archive/3d_models` | `3d_models` | `c5d1924` | CAD / модели → сейчас `dev-assemble-3d_models` |
| `archive/dev-firmware` | `dev-firmware` | `c5d1924` | прошивка → сейчас `dev-software-firmware` |
| `archive/dev-firmware-freertos` | `dev-firmware-freertos` | `1050e49` | FreeRTOS-прошивка |
| `archive/dev-firmware-opensource` | `dev-firmware-opensource` | `f97bf9c` | opensource-прошивка |
| `archive/dev-ros2-DmitryTorov` | `dev-ros2-DmitryTorov` | `70d85e6` | ранний `tinker_joy` / GUI (до unified) |

### Прочие (legacy)

| Тег | Бывшая ветка | Tip |
|---|---|---|
| `archive/docker_jetson_nano` | `docker_jetson_nano` | `f526f31` |
| `archive/docker_notebook_budkhovskaia` | `docker_notebook_budkhovskaia` | `6793c70` |
| `archive/docker_rl` | `docker_rl` | `b19329c` |
| `archive/pc_usage_without_docker` | `pc_usage_without_docker` | `1d25123` |
| `archive/Chellenge-Tinker` | `Chellenge-Tinker` | `4558d90` |
| `archive/DzirtH-patch-1` | `DzirtH-patch-1` | `0f2d051` |

---
