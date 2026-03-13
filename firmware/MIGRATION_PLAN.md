# План миграции firmware: legacy -> HAL + FreeRTOS

## Текущий статус

### Уже реализовано
- Базовый HAL-драйвер CAN1 в новом шаблоне.
- Подключены прерывания CAN1 RX0/SCE.
- Добавлен отдельный модуль MIT-протокола (pack/unpack) как независимый слой.
- Добавлена RTOS-задача CAN service в инициализационный сценарий.
- Подключены CAN1 пины PB8/PB9 в новом board_pins.
- Добавлена очередь TX для MIT-команд в CAN service.
- Реализована state machine RUN/DEGRADED/SAFE_STOP с мягким safe-stop профилем.
- Добавлено декодирование MIT feedback в таблицу по motor_id для просмотра в debug.
- Добавлена поддержка двух CAN шин (CAN1+CAN2) и маршрутизация 10 моторов с учетом типа мотора.
- Временный keep-alive отправляется на все 10 моторов для проверки получения feedback на обеих шинах.
- Изменения успешно проходят сборку и прошивку через CMSIS tasks.

### Следующий практический шаг
- Интегрировать реальные MIT-команды от control-задачи вместо временного keep-alive и начать публикацию CAN-статусов/feedback в SPI-пакет для наблюдения на управляющей плате.

## Правила выполнения работ
- Прошивка и reset платы выполняются только по явной команде пользователя.
- В отчете по этапу описывается только проверка с пользовательской стороны: наблюдаемое поведение платы и ожидаемые данные по SPI.
- Технические детали CI/сборки/предупреждений не считаются результатом проверки работоспособности.

## Цель
Перенести функционал из src_/include_ в новый каркас src/include поэтапно: сначала стабильная периферийная основа (HAL драйверы + RTOS сервисы), затем перенос логики управления и safety. Каждый этап завершается проверкой на железе через CMSIS Toolbox + pyOCD, чтобы не накапливать интеграционные риски.

## Этапы

### 1. Фаза A - База миграции и критерии готовности
1. Зафиксировать целевой MVP и DoD: CAN(MIT), IMU(SPI3), SPI slave с одноплатником, Flash params, Beep/LED, Watchdog/Safety.
2. Подготовить карту соответствия legacy->new (модуль, API, RTOS-задача, приоритет, период).
3. Границы первой итерации: без UART и без Battery/ADC.

### 2. Фаза B - Платформенный слой HAL
1. Развернуть базовые HAL-драйверы как независимые сервисы: CAN, SPI master(IMU), SPI slave(PI link), Flash, IWDG, GPIO(LED/Beep).
2. Единый API на модуль: init/start/stop/read/write/ioctl + status/error.
3. Подготовить таблицу IRQ/DMA приоритетов и владельцев каналов.
4. Добавить обязательные runtime-проверки ошибок HAL (возвраты, timeout, recovery).

### 3. Фаза C - RTOS каркас
1. Разложить запуск на задачи: init, comm, sensors, control, safety, ui.
2. Межзадачная модель: osMessageQueue, osEventFlags, osMutex.
3. Базовые приоритеты и периоды:
- safety/watchdog: highest, 10-20 ms
- can_txrx: high, 1 ms
- imu_fusion: high/above normal, 2 ms
- pi_link_spi2: normal, event-driven + timeout
- flash_service: below normal
- led_beep_ui: low, 50-100 ms
4. Ввести RTOS телеметрию: stack watermark, missed deadlines, fault counters.

### 4. Фаза D - Миграция CAN MIT
1. Перенести и адаптировать только протокол (float/int packing, limits), не перенося старый init.
2. Реализовать HAL CAN backend: mailbox/FIFO/filter/IRQ callbacks.
3. Добавить task_can_tx/task_can_rx или единую задачу + очереди команд/feedback.
4. Реализовать безопасный fallback при потере связи: не мгновенный ноль, а safe-stop профиль.
5. Проверка: loopback/реальная шина, 1 kHz, контроль overflow FIFO/mailbox.

### 5. Фаза E - Миграция IMU (SPI3) и оценка ориентации
1. Перенести low-level ICM20602 в HAL SPI master + ISR/DMA.
2. Перенести алгоритмы ориентации как pure compute модуль.
3. Разделить acquisition и fusion: sensor_raw_queue -> fusion_task.
4. Добавить валидацию NaN/Inf и reinit-стратегию.
5. Проверка: 500 Hz, стабильность, jitter.

### 6. Фаза F - Миграция SPI slave канала с одноплатником
1. Перенести формат протокола из Custom_SPI_Device и нормализовать state machine.
2. Реализовать non-blocking DMA ring/двойной буфер + события завершения transfer.
3. Добавить heartbeat и timeout без опасного мгновенного отключения приводов.
4. Проверка: устойчивый обмен под нагрузкой и корректное восстановление после обрыва мастера.

### 7. Фаза G - Flash params + конфиг
1. Перенести структуру параметров и адресацию в HAL_FLASH слой.
2. Ввести deferred write, CRC, versioning, defaults fallback.
3. Проверка: cold boot, power-cycle, консистентность параметров.

### 8. Фаза H - Safety, Watchdog, индикация
1. Реализовать централизованный health monitor: связи модулей, таймауты, fault levels.
2. Кормить IWDG только при выполнении health условий.
3. Перенести профили LED/Beep в отдельную low-priority задачу.
4. Проверка: fault injection (SPI/CAN/IMU) и ожидаемое безопасное поведение.

### 9. Фаза I - Интеграция control-логики
1. Перенести структуры и математику (base_struct/gait/common/filter/RT_math) как доменный слой.
2. Переписать semantics legacy scheduler в RTOS control_task без super-loop.
3. Добавить контроль deadline и деградационные режимы.
4. Проверка: функциональный паритет с legacy.

### 10. Фаза J - Стабилизация и техдолг
1. Профилирование CPU/stack/heap и финальная настройка приоритетов.
2. Регресс на железе по чек-листу.
3. Документирование архитектуры потоков и интерфейсов для 2-й волны (ADC и дополнительные SPI сценарии).

## Верификация по этапам
1. Сборка после каждой фазы:
   cbuild F407_FC.csolution.yml --context F407_FC+TinkerFirmware
2. Прошивка/запуск после каждой фазы:
   только по явной команде пользователя
3. Дополнительно по этапам:
- B/C: smoke test RTOS
- D: CAN throughput/latency на 1 kHz
- E: IMU rate/jitter на 500 Hz
- F: SPI endurance + восстановление
- G: persistence test
- H: safety fault-injection
- I/J: parity checklist с legacy
4. Формат отчета по проверке:
- Что должно наблюдаться на плате (LED/beep/реакция приводов).
- Что должно наблюдаться на стороне управляющей платы по SPI (пакеты, тайминги, флаги состояний).
- Критерий приемки этапа в терминах поведения системы.

## Принятые решения
- Основной источник миграции: src_/include_.
- MVP первой волны: CAN MIT, IMU, SPI slave с одноплатником, Flash params, Beep/LED, Watchdog/Safety.
- UART не используется в текущей архитектуре взаимодействия; основной канал обмена с управляющей платой - SPI.
- Первый проход без Battery/ADC.
- Стратегия: сначала периферия и инфраструктура RTOS, затем перенос control-логики.
- Подход качества: сбалансированный.

## Примечания по библиотекам
- Базовый стек: HAL + CMSIS-FreeRTOS из паков.
- CMSIS-DSP подключать на этапе I после профилирования math-пути.
- Для SPI/CAN safety заранее оформить формальный state machine: RUN/DEGRADED/SAFE_STOP.
