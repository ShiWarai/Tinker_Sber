uint8 DISABLING = 0
uint8 ENABLE = 1
uint8 OVERCURRENT = 10
uint8 OVERVOLTAGE = 8
uint8 LOWVOLTAGE = 9
uint8 OVERTEMPERATURE_MOSFET = 11
uint8 OVERTEMPERATURE_ROTOR = 12
uint8 LOSS_CONNECTION= 13
uint8 OVERLOAD = 14

builtin_interfaces/Time timestamp_state
float32 position
float32 velocity
float32 torque
int16 temperature_mosfet
int16 temperature_rotor
uint8 error


#     0x01: receive fail
#     0x11: receive success
#     0x02: send fail
#     0x12: send success
#     0x03: set baudrate fail
#     0x13: set baudrate success
#     0xEE: communication error
