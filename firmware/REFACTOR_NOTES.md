# Заметки по рефакторингу прошивки

Осмотр проекта выполнен по состоянию на рефакторинг. Ниже — что реально в сборке, что можно удалить и что проверить.

**Подтверждено:** при Rebuild компилируются только файлы из uvprojx (delay, stm32f4xx_it, sys, main, flash_w25, time, init, bat, Custom_SPI_Device, scheduler, flash, spi, dma, watch_dog, beep, StdPeriph, pwm_out, led_fc, wsled_l, startup, rng, SCS*, common_math, RT_math, retarget_io, fliter_math, icm20602, can, mit_link, kin_math, mems, imu, usart_fc). Всё остальное в репозитории в сборку не входит.

---

## 1. Что входит в сборку (F407_FC.uvprojx)

- **System:** `include.h`, `main.c`, `scheduler.c`, `init.c`, `stm32f4xx_it.c`, `sys.c`, `delay.c`
- **Drivers:** Custom_SPI_Device, spi, flash, flash_w25, time, bat, beep, dma, led_fc, pwm_out, watch_dog, wsled_l
- **Communication:** usart_fc, can
- **Sensor:** imu, mems, icm20602
- **Motor_Link:** mit_link + заголовки base_struct, locomotion_header
- **Servos:** SCS, SCSCL, SCSerail, SMS_STS
- **Math/Fusion:** rng, common_math, kin_math, RT_math, fliter_math, gait_math.h, cycle_cal_oldx.lib, arm_cortexM4lf_math.lib
- **Libraries:** STM32F4xx StdPeriph, CMSIS (startup, system)

В IncludePath: `quadruped_src`, `estimator_sr`, `drivers`, `applications`, `balance_controller`, и др.

---

## 2. Кандидаты на удаление (не в проекте и не используются)

### 2.1 Дубликаты и мёртвые заголовки

| Файл | Рекомендация |
|------|--------------|
| `estimator_sr\cycle_cal_oldx.h` | Дубликат `quadruped_src\cycle_cal_oldx.h`. Используется только `quadruped_src` (его подхватывает `mems.c`). Можно удалить. |
| `quadruped_src\baro_ekf_oldx.h` | Нигде не подключается. Удалить. |
| `estimator_sr\baro_ekf_oldx.h` | То же. Удалить. |

### 2.2 Драйверы не в uvprojx и без включений из текущей сборки

Эти файлы **не добавлены в F407_FC.uvprojx** и не подключаются из файлов, входящих в сборку:

| Файлы | Примечание |
|-------|------------|
| `drivers\gps.c`, `drivers\gps.h` | Не в проекте, не включаются. Кандидат на удаление (или вынос в «опциональные»). |
| `drivers\i2c_soft.c`, `drivers\i2c_soft.h` | То же. |
| `drivers\outter_hml.c`, `drivers\outter_hml.h` | То же. Единственное использование — `module.hml_imu_o=1` внутри outter_hml.c; в проекте outter_hml не собирается. |
| `drivers\nlink_utils.c`, `drivers\nlink_utils.h`, `drivers\nlink_typedef.h`, `drivers\nlink_linktrack_aoa_nodeframe0.h` | Не в проекте, не включаются. Кандидаты на удаление. |
| `drivers\rc_mine.c` | Не в проекте. `Nrf_Check_Event` и др. из rc_mine нигде не вызываются. `module` определён в `usart_fc.c`. Можно удалить `rc_mine.c`, оставив `rc_mine.h` (его подключают scheduler, include.h). |

После удаления `rc_mine.c` нужно оставить в репозитории все типы/объявления, которые реально нужны (сейчас они в `rc_mine.h`). Если что-то из rc_mine.h используется только в rc_mine.c — можно почистить и заголовок.

### 2.3 estimator_sr

- **Оставить:** `nav.h` (используют scheduler, usart_fc, rc_mine), `cycle_cal_oldx.lib` (в uvprojx; файл может быть в .gitignore — проверить наличие в репо/сборке).
- **Можно удалить:** `cycle_cal_oldx.h` (дубликат, см. выше), `baro_ekf_oldx.h` (не используется).

---

## 3. Что используется и трогать осторожно

- **balance_controller/** — в проекте: common_math, kin_math, RT_math, fliter_math, gait_math.h, base_struct.h, locomotion_header.h. README помечает как «остатки логики походки/баланса»; для режима «только мост» не используются, но **сборка их компилирует**. Удалять только после перевода на чистый «мост» и исключения из uvprojx.
- **quadruped_src/** — в проекте: include.h, rng.c, imu.c, mems.c; mems.c включает `cycle_cal_oldx.h` и вызывает `cycle_cal_oldx()` из `cycle_cal_oldx.lib`. Не удалять без замены/удаления этой логики.
- **flash_nav.h** — подключается из `flash.c`, нужен.
- **sbus.h** — используется в base_struct (поля sbus_*). Не удалять, если оставляете структуры с sbus.

---

## 4. Артефакты и папка build

- В `build/` есть старые .d/.o от модулей, которых уже нет в uvprojx (mavlink, gps, oled, ms5611, ekf, eso, pose_kf, baro_kf, aoa_uwb, protocol, nlink_utils, outter_hml, gui_paint, usb_*, и т.д.). После рефакторинга полезно сделать **Clean** в Keil и пересобрать; при желании можно добавить в .gitignore всё содержимое `build/` (часть уже игнорируется).
- Файлы `F407_FC.uvguix.*`, `F407_FC.uvoptx` уже в .gitignore — ок.

---

## 5. Мёртвый код внутри файлов (по карте линкера)

Линкер выкидывает неиспользуемые символы (уже не попадают в прошивку), но код остаётся в репозитории:

- `fliter_math.c`: `OLDX_SMOOTH_IN_ESOX` — вызывается из `bat.c` в `get_leg_press()`, но `get_leg_press` в итоге не вызывается (или путь отключён), поэтому линкер удалил функцию. Можно удалить вызов из bat и при желании саму `OLDX_SMOOTH_IN_ESOX` из fliter_math.
- `Custom_SPI_Device.c`: `Custom_SPI_DEVICE_TestCommand` — удалён линкером. Можно удалить функцию из исходника.
- `wsled_l.c`: `test_ws` — удалён линкером. Можно удалить.
- `usart_fc.c`: `test_wheel` — удалён линкером. Можно удалить.
- `icm20602.c`: `icm20602_get_temp` — удалён линкером. Можно удалить.
- В SCS: `ReadTemper` / `ReadTemper_sms` — удалены линкером. Можно почистить вызовы/функции при рефакторинге.

---

## 6. Рекомендуемый порядок действий

1. **Безопасно удалить сейчас:**  
   `quadruped_src\baro_ekf_oldx.h`, `estimator_sr\baro_ekf_oldx.h`, `estimator_sr\cycle_cal_oldx.h`.
2. **Проверить наличие:**  
   `estimator_sr\cycle_cal_oldx.lib` (если нет в репо — добавить в репо или в инструкцию сборки).
3. **По желанию удалить неиспользуемые драйверы:**  
   gps, i2c_soft, outter_hml, nlink_*, при необходимости — rc_mine.c (оставив/подчистив rc_mine.h).
4. **После перехода на режим «только мост»:**  
   исключить из uvprojx и затем удалить каталоги/файлы balance_controller и quadruped_src (или их части), предварительно перенеся в мост только нужные типы/структуры (например, из base_struct).
5. **Очистка мёртвого кода:**  
   убрать неиспользуемые функции в bat, fliter_math, Custom_SPI_Device, wsled_l, usart_fc, icm20602, SCS (см. выше).
6. **Сборка:**  
   после удалений — Clean + полная пересборка и проверка прошивки на стенде.

Если нужно, могу предложить конкретные патчи (diff) по шагам 1–3 и 5.
