import sysv_ipc
import struct
import time
import sys

# Константы из C++ кода
MEM_SPI_KEY = 1  # key_t MEM_SPI = 0001
MEM_SIZE = 2048
SHARED_MEM_SIZE = 4 + MEM_SIZE  # flag (int) + szMsg[MEM_SIZE]
HALF_MEM_SIZE = MEM_SIZE // 2  # 1024
MOTOR_COUNT = 10
BYTES_PER_FLOAT = 4
BYTES_PER_MOTOR = 12  # q_set (4) + dq_set (4) + tau_ff (4)

# Смещения в szMsg (после flag в 0-4)
WRITE_OFFSET_BASE = HALF_MEM_SIZE  # 1024: начало раздела чтения (команд)

# Смещения для параметров
Q_SET_OFFSET_START = WRITE_OFFSET_BASE + 0
DQ_SET_OFFSET_START = WRITE_OFFSET_BASE + MOTOR_COUNT * 4
TAU_FF_OFFSET_START = WRITE_OFFSET_BASE + MOTOR_COUNT * 8
# После всех моторов: 14 * 12 = 168
KP_OFFSET = WRITE_OFFSET_BASE + MOTOR_COUNT * BYTES_PER_MOTOR  # 1024 + 168 = 1192
KD_OFFSET = KP_OFFSET + BYTES_PER_FLOAT  # 1196
EN_MOTOR_OFFSET = KD_OFFSET + BYTES_PER_FLOAT  # 1200 (1 byte)
RESET_Q_OFFSET = EN_MOTOR_OFFSET + 1  # 1201 (1 byte)
RESET_ERR_OFFSET = RESET_Q_OFFSET + 1  # 1202 (1 byte)
ACC_CAL_OFFSET = RESET_ERR_OFFSET + 1  # 1203 (1 byte)
GYRO_CAL_OFFSET = ACC_CAL_OFFSET + 1  # 1204 (1 byte)
MAG_CAL_OFFSET = GYRO_CAL_OFFSET + 1  # 1205 (1 byte)
BEEP_OFFSET = MAG_CAL_OFFSET + 1  # 1206 (1 byte)

# Функция для упаковки float в little-endian (как в setDataFloat_spi)
def pack_float(value):
    return struct.pack('<f', float(value))

# Функция для упаковки байта (для en_motor и т.д.)
def pack_byte(value):
    return bytes([int(value) & 0xFF])

# Функция ожидания flag == 1
def wait_for_flag(shm):
    while True:
        memory = shm.read(4, 0)  # Чтение flag (первые 4 байта)
        flag = struct.unpack('<i', memory)[0]
        if flag == 1:
            return
        time.sleep(0.1)  # Опрос каждые 100 мс

# Функция установки flag в 0
def set_flag_to_zero(shm):
    shm.write(struct.pack('<i', 0), 0)
    print("Флаг установлен в 0. Изменения будут прочитаны оборудованием.")

# Функция обновления параметра в shared memory
def update_parameter(shm, param_name, id=0, value=None):
    if param_name in ['q_set', 'dq_set', 'tau_ff']:
        if id == 0 or not (1 <= id <= MOTOR_COUNT):
            raise ValueError(f"Для {param_name} требуется индекс (1-{MOTOR_COUNT})")

        index = id - 1  # Преобразуем в 0-based индекс
        # Структура: q_set[0], dq_set[0], tau_ff[0], q_set[1], dq_set[1], tau_ff[1], ...
        # Каждый двигатель занимает 12 байт (3 * 4 байта)
        motor_offset = index * 12  # 12 байт на двигатель
        
        if param_name == 'q_set':
            offset = WRITE_OFFSET_BASE + motor_offset + 4  # +4 из-за flag
        elif param_name == 'dq_set':
            offset = WRITE_OFFSET_BASE + motor_offset + 4 + 4  # +4 для q_set, +4 для dq_set
        else:  # tau_ff
            offset = WRITE_OFFSET_BASE + motor_offset + 4 + 8  # +4 для q_set, +4 для dq_set, +4 для tau_ff
        data = pack_float(value)
        shm.write(data, offset)
        print(f"Обновлен {param_name}[{index}] на {value} по смещению {offset-4} в szMsg.")
    
    elif param_name == 'kp':
        offset = KP_OFFSET + 4  # +4 из-за flag
        data = pack_float(value)
        shm.write(data, offset)
        print(f"Обновлен {param_name} на {value} по смещению {offset-4} в szMsg.")
    
    elif param_name == 'kd':
        offset = KD_OFFSET + 4
        data = pack_float(value)
        shm.write(data, offset)
        print(f"Обновлен {param_name} на {value} по смещению {offset-4} в szMsg.")
    
    elif param_name == 'en_motor':
        offset = EN_MOTOR_OFFSET + 4
        data = pack_byte(value)
        shm.write(data, offset)
        print(f"Обновлен {param_name} на {value} по смещению {offset-4} в szMsg.")
    
    else:
        raise ValueError(f"Неизвестный параметр: {param_name}")

# Функция обнуления всех данных
def reset_all(shm):
    # Обнуление массивов (чередующаяся структура)
    for i in range(MOTOR_COUNT):
        motor_offset = i * 12  # 12 байт на двигатель
        shm.write(pack_float(0.0), WRITE_OFFSET_BASE + motor_offset + 4)  # q_set
        shm.write(pack_float(0.0), WRITE_OFFSET_BASE + motor_offset + 8)  # dq_set
        shm.write(pack_float(0.0), WRITE_OFFSET_BASE + motor_offset + 12) # tau_ff
    
    # Обнуление скаляров
    shm.write(pack_float(0.0), KP_OFFSET + 4)
    shm.write(pack_float(0.0), KD_OFFSET + 4)
    shm.write(pack_byte(0), EN_MOTOR_OFFSET + 4)
    shm.write(pack_byte(0), RESET_Q_OFFSET + 4)
    shm.write(pack_byte(0), RESET_ERR_OFFSET + 4)
    shm.write(pack_byte(0), ACC_CAL_OFFSET + 4)
    shm.write(pack_byte(0), GYRO_CAL_OFFSET + 4)
    shm.write(pack_byte(0), MAG_CAL_OFFSET + 4)
    shm.write(pack_byte(0), BEEP_OFFSET + 4)
    
    print("Все данные обнулены.")

# Основное консольное приложение
def main():
    try:
        shm = sysv_ipc.SharedMemory(MEM_SPI_KEY)
        print("Подключено к shared memory с ключом", MEM_SPI_KEY)
    except sysv_ipc.ExistentialError:
        print("Shared memory с ключом", MEM_SPI_KEY, "не существует. Сначала запустите C++ программу.")
        sys.exit(1)

    print("Консольное приложение для обновления параметров в shared memory.")
    print("Доступные параметры: q_set, dq_set, tau_ff (массивы 0-13), kp, kd, en_motor (скаляры)")
    print("Введите 'exit' для завершения или 'reset' для обнуления всех данных.")

    while True:
        # Ожидание flag == 1 перед обновлениями
        wait_for_flag(shm)

        # Запрос ввода
        user_input = input("Введите параметр (например, q_set 5 1.23) или 'reset' или 'exit': ").strip()
        if user_input.lower() == 'exit':
            break
        elif user_input.lower() == 'reset':
            reset_all(shm)
            set_flag_to_zero(shm)
            continue

        parts = user_input.split()
        if len(parts) < 2:
            print("Неверный ввод. Формат: param_name [id] value")
            continue

        param_name = parts[0]
        if param_name in ['q_set', 'dq_set', 'tau_ff']:
            if len(parts) != 3:
                print("Для массивов: param_name id value")
                continue
            try:
                id = int(parts[1])
                value = float(parts[2])
            except ValueError:
                print("Неверный индекс или значение.")
                continue
            update_parameter(shm, param_name, id, value)
        elif param_name in ['kp', 'kd', 'en_motor']:
            if len(parts) != 2:
                print("Для скаляров: param_name value")
                continue
            try:
                value = float(parts[1]) if param_name in ['kp', 'kd'] else int(parts[1])
            except ValueError:
                print("Неверное значение.")
                continue
            update_parameter(shm, param_name, value=value)
        else:
            print("Неизвестный параметр.")
            continue

        # После обновления установить flag в 0 для запуска чтения оборудованием
        set_flag_to_zero(shm)

if __name__ == "__main__":
    main()


