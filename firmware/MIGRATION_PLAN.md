# План миграции firmware: StdPeriph → HAL + FreeRTOS

## Текущее состояние

### Сделано
- [x] HAL bring-up: `HAL_Init`, `SystemClock_Config`, GPIO
- [x] LED RED/BLUE (PB3, PB4) — тестовое мигание
- [x] LED status (PA0, PA1), SCL/SCP (PA6, PA7), leg power (PC5)
- [x] Pack-компоненты: Device:Startup, CMSIS:CORE
- [x] Удалены legacy: lib/CMSIS, lib/STM32F4xx_StdPeriph_Driver, lib/DSP
- [x] Buzzer PB7, TIM4_CH2 — HAL PWM + новая стартовая мелодия
- [x] CAN1/CAN2 HAL bring-up (MVP): init/start/filter + send/receive API

### Пин-аут (board_pins.h)
| Сигнал | Порт | Пин | Legacy |
|--------|------|-----|--------|
| LED RED | PB | 3 | led_fc.c |
| LED BLUE | PB | 4 | led_fc.c |
| LED Status RED | PA | 0 | led_fc.c |
| LED Status BLUE | PA | 1 | led_fc.c |
| LED SCL | PA | 6 | led_fc.c |
| LED SCP | PA | 7 | led_fc.c |
| Leg power | PC | 5 | led_fc.c |
| Button KEY_DOG | PB | 12 | init.c |
| **Buzzer (Beep)** | **PB** | **7** | **beep.c, TIM4_CH2** |

---

## План миграции (по приоритету)

### 1. CAN — управление моторами
**Legacy:** `src_/drivers/can.c`, StdPeriph CAN1/CAN2

- **Задачи:**
  - [x] Добавить HAL CAN: `stm32f4xx_hal_can.c`
  - [x] Перенести базовые CAN1_Mode_Init, CAN2_Mode_Init (MVP)
  - Перенести обработку прерываний RX и таблицу команд
  - Сохранить MIT-протокол, бинарную совместимость
  - CAN_motor_sm(), Duty_Servo() — 1 ms цикл

- **Зависимости:** `delay.c`, `time.c` (Get_Cycle_T)

---

### 2. SPI2 Slave + Custom_SPI — связь с Linux (Raspberry Pi)
**Legacy:** `src_/drivers/Custom_SPI_Device.c`, `spi.c` (SPI2)

- **Пины:** PB13 SCK, PB14 MISO, PB15 MOSI, PB12 CS (EXTI)
- **Протокол:** DataSize 162 байта, DMA RX/TX, EXTI по CS
- **DataRxBuffer / DataTxBuffer** — формат пакетов

- **Задачи:**
  - HAL SPI2 slave mode
  - HAL DMA для SPI RX/TX
  - EXTI на PB12 (CS)
  - Сохранить формат пакетов и протокол

---

### 3. Buzzer (Beep) — ГОТОВО (MVP)
**Legacy:** `src_/drivers/beep.c` — PB7, TIM4_CH2, PWM

- **Сделано:**
  - Добавлен `BEEP_PORT`/`BEEP_PIN` в `board_pins.h` (PB7)
  - Реализован HAL TIM4 PWM (Channel 2) в `src/drivers/beep.c`
  - Простая стартовая мелодия (двойной короткий бип) в `beep_startup_melody()`
  - Вызов в `app_main()` после инициализации LED

- **Дальше (позже):**
  - Перенести сложную state-машину beep’ов из legacy (`Play_Music_Task`, коды BEEP_*) при переносе логики робота

---

### 4. Память: W25 + внутренняя flash
**Legacy:** `flash_w25.c` (SPI), `flash.c` (params)

- **Задачи:**
  - W25QXX на HAL SPI (SPI1 или SPI3)
  - Flash params через HAL Flash API
  - READ_PARM(), WRITE_PARM(), READ_WAY_POINTS()

---

### 5. Кнопка PB12 (KEY_DOG)
**Legacy:** `KEY_DOG()` в init.c, IWDG

- **Задачи:**
  - GPIO input PB12 (уже в board_pins)
  - Обработка нажатия (EXTI или polling)
  - Интеграция с IWDG

---

### 6. UART
**Legacy:** `usart_fc.c` — UART1/2/3/6, DMA

- **Задачи:**
  - HAL UART + DMA
  - Usart1_Init (9600/115200), Usart2_Init, Usart3_Init, Uart6_Init

---

### 7. SPI3 — IMU (icm20602)
**Legacy:** `spi.c`, `icm20602.c`

- **Задачи:**
  - HAL SPI3 master
  - icm20602_init(), чтение данных

---

### 8. PWM, ADC, RNG, Watchdog
**Legacy:** `pwm_out.c`, `bat.c` (ADC), `rng.c`, `watch_dog.c`

- **Задачи:**
  - HAL TIM PWM (для приводов, LED и пр., buzzer уже реализован отдельно)
  - HAL ADC
  - HAL RNG
  - HAL IWDG

---

### 9. FreeRTOS (CMSIS-RTOS2)
**Legacy:** Superloop с SysTick 1 ms

- **Задачи:**
  - Добавить CMSIS-RTOS2 (RTX или FreeRTOS)
  - Задачи: Duty_Servo (1 ms), Duty_Att (2 ms), Duty_System (50 ms)
  - Потокобезопасность: volatile, мьютексы для shared data

---

### 10. Остальные подсистемы
- `scheduler.c` — Duty_Loop, Duty_Att_Fushion, Duty_Navigation, Duty_System
- `mit_link.c` — MIT protocol
- `gait_math.c`, `mems.c`, `imu.c` — математика, IMU
- `wsled.c` — PWM LED
- `time.c`, `delay.c` — системное время

---

## Рекомендуемый порядок

1. **CAN** — чтобы снова заработало управление моторами
2. **SPI2 slave + Custom_SPI** — связь с Linux
3. **LED-индикация по состояниям** — перенести паттерны из `led_fc.c` (состояния, ошибки, режимы) на HAL GPIO/таймеры, сверяясь с legacy
4. **W25 + flash** — параметры и waypoints
5. **Кнопка PB12** — watchdog
6. **UART, IMU, PWM и т.д.** — по мере необходимости
7. **FreeRTOS** — переход от superloop

---

## Ссылки на legacy

| Файл | Описание |
|------|----------|
| `src_/app/main.c` | main → All_Init, Duty_Loop |
| `src_/app/init.c` | All_Init — инициализация всего |
| `src_/app/scheduler.c` | Duty_Loop, Duty_Servo, Duty_System |
| `src_/drivers/can.c` | CAN1/CAN2, motor protocol |
| `src_/drivers/Custom_SPI_Device.c` | SPI2 slave, Linux |
| `src_/drivers/beep.c` | Buzzer, TIM4_CH2, PB7 |
| `src_/drivers/flash_w25.c` | W25 flash |
| `src_/drivers/flash.c` | Params |
| `include_/drivers/Custom_SPI_Device.h` | DataSize 162, пины |
