builtin_interfaces/Time timestamp_state
float32 position
float32 velocity
float32 torque
int16 temperature_mosfet
int16 temperature_rotor
bool connected
bool enabled

# connected: CAN link to motor (SPI status byte tens digit)
# enabled: motor on/off state (SPI status byte ones digit, ready)
# temperature_*: not yet provided by firmware (0 = no data)
