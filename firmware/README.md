# Прошивка платы управления роботом

Проект прошивки для **STM32F405RGTx** (target: `TinkerFirmware`).

Плата выполняет:
- обмен по **CAN** с приводами DM8006/DM6006 (MIT protocol);
- чтение **IMU** по SPI и передачу данных на одноплатник;
- работу с Flash-памятью, индикацией (LED, beep) и системной периферией.

## Текущий toolchain

- Сборка: **CMSIS-Toolbox** (`cbuild`), компилятор **AC6/armclang**.
- Прошивка/отладка: **pyOCD + STLink**.
- Конфигурация проекта:
  - `F407_FC.csolution.yml`
  - `F407_FC.cproject.yml`
  - `F407_FC_TinkerFirmware.sct` (scatter-файл линкера)

## Сборка

Из корня `firmware`:

```powershell
cbuild F407_FC.csolution.yml --context F407_FC+TinkerFirmware
```

Результат:
- `out/F407_FC/TinkerFirmware/F407_FC.axf`

## Прошивка через STLink

```powershell
pyocd load --probe stlink: --cbuild-run out/F407_FC+TinkerFirmware.cbuild-run.yml
```

## Отладка

В VS Code:
1. `Run and Debug`
2. Выбрать `CMSIS_DAP@pyOCD (launch)` или `CMSIS_DAP@pyOCD (attach)`

Альтернатива через задачу:

```powershell
pyocd gdbserver --probe stlink: --connect attach --persist --reset-run --cbuild-run out/F407_FC+TinkerFirmware.cbuild-run.yml
```

## Структура проекта

- `src/app/` — `main`, `scheduler`, `init`, обработчики прерываний.
- `src/drivers/` — CAN, SPI, Flash, USART, LED, beep, RNG и пр.
- `src/math/` — математика (`common_math`, `fliter_math`, `RT_math`).
- `src/sensors/` — IMU, MEMS.
- `src/system/` — системные модули (`sys`, `delay`).
- `include/` — заголовки по подсистемам.
- `Libraries/`, `DSP_LIB/`, `lib/` — сторонние/вендорные библиотеки.

## Примечание по CAN ID

Для каждой CAN-шины используется одинаковый набор ID:
- `1` — DM6006
- `2` — DM8006
- `3` — DM8006
- `4` — DM8006
- `5` — DM6006

