builtin_interfaces/Time timestamp_state
float32 position
float32 velocity
float32 torque
int16 temperature_mosfet
int16 temperature_rotor
bool connected
bool enabled

# connected: связь с мотором по CAN (десятки статус-байта SPI, connect_motor)
# enabled: мотор включён/выключен (единицы статус-байта SPI, ready)
# temperature_*: прошивка пока не передаёт (0 = нет данных)
#
# Статус-байт SPI на мотор (прошивка): connect*100 + connect_motor*10 + ready
#
# Ранее поле uint8 error и константы (пока не приходят по SPI, оставлены для будущего API):
# uint8 DISABLING = 0
# uint8 ENABLE = 1
# uint8 OVERVOLTAGE = 8
# uint8 LOWVOLTAGE = 9
# uint8 OVERCURRENT = 10
# uint8 OVERTEMPERATURE_MOSFET = 11
# uint8 OVERTEMPERATURE_ROTOR = 12
# uint8 LOSS_CONNECTION = 13
# uint8 OVERLOAD = 14
#
# Коды CAN-связи (не ошибки драйвера, из старых комментариев):
#     0x01: приём неуспешен
#     0x11: приём успешен
#     0x02: отправка неуспешна
#     0x12: отправка успешна
#     0x03: смена baudrate неуспешна
#     0x13: смена baudrate успешна
#     0xEE: ошибка связи
