#ifndef MOTOR_CONTROL_SPI_NODE_HPP
#define MOTOR_CONTROL_SPI_NODE_HPP

#define SPI_BUF_SIZE 255
#define CAN_T_DIV 500.0
#define CAN_I_DIV 100.0
#define CAN_F_DIV 100.0
#define CAN_POS_DIV 30.0
#define CAN_DPOS_DIV 20.0
#define CAN_GAIN_DIV_P 500.0
#define CAN_GAIN_DIV_I 10000.0
#define CAN_GAIN_DIV_D 1000.0

#define BYTE0(dwTemp)       (*(char *)(&dwTemp))
#define BYTE1(dwTemp)       (*((char *)(&dwTemp) + 1))
#define BYTE2(dwTemp)       (*((char *)(&dwTemp) + 2))
#define BYTE3(dwTemp)       (*((char *)(&dwTemp) + 3))

typedef struct {
    float x;
    float y;
    float z;
    float zz;
} END_POS;

typedef struct {
    char connect;
    int loss_cnt;
    char mode;
    char up_mode;
    float rc_spd_b[3], rc_rate_b[3];
    char cmd_robot_state;
    float rc_spd_w[2], rc_att_w[2], rate_yaw_w;
    int key_ud, key_lr, key_x, key_y, key_a, key_b, key_ll, key_rr, key_st, key_back, key_ud_reg, key_lr_reg, key_x_reg, key_y_reg, key_a_reg, key_b_reg, key_ll_reg, key_rr_reg, key_st_reg, key_back_reg;
    float curve[20];
    float sbus_ch[6];
    int sbus_aux[6];
    char sbus_conncect;
    char sbus_power_sw, sbus_power_sw_reg;
    int sbus_rc_main[6], sbus_rc_main_reg[6];
    int sbus_mode, sbus_mode_reg;
    int sbus_mode_e, sbus_mode_e_reg;
    float sbus_height, sbus_height_reg;
    int sbus_aux_reg[6];
} _OCU;

typedef struct {
    int rssi;
    float dis, angle;
} _AOA;

typedef struct {
    int connect;
    char power;
    int cap;
    int mode;
    int loss_connect;
    float att_set[3];
    END_POS pos_set;
    float att_now[3];
    END_POS pos_now;
    float move_spd;
    float torque;
} _ARMSS;

typedef struct {
    float att[3];
    float att_rate[3];
    float acc_b[3];
    float acc_n[3];
    float att_usb[3];
    float att_rate_usb[3];
    float acc_b_usb[3];
    float att_usb_bias[3];
    float att_rate_usb_bias[3];
    float acc_b_usb_bias[3];
    float q[10];
    float dq[10];
    float tau[10];
    float bat_v[4];
    char connect[4];
    char connect_motor[10];  // связь по CAN (десятки статус-байта SPI)
    char ready[10];          // вкл/выкл мотора (единицы статус-байта SPI)
    _OCU ocu;
    _AOA aoa;
    _ARMSS arm_cmd_s;
} _SPI_RX;

typedef struct {
    float q_set[10];
    float dq_set[10];
    float tau_ff[10];
    float q_reset[10];
    float t_to_i;
    float max_i;
    float kp[10], kd[10];
    float ki;
    float kp_st, ki_st, kd_st;
    float kp_sw, ki_sw, kd_sw;
    float kp_st_d[3], ki_st_d[3], kd_st_d[3];
    float kp_sw_d[3], ki_sw_d[3], kd_sw_d[3];
    char param_sel[4];
    char en_motor, reset_q, reset_err;
    char led_enable[2];
    char beep_state;
    _ARMSS arm_cmd_s;
} _SPI_TX;

typedef struct {
    float x, y, z;
} xyz_f_t;

typedef struct {
    int16_t x, y, z;
} xyz_s16_t;

typedef struct {
    char Acc_CALIBRATE;
    char Gyro_CALIBRATE;
    char Cali_3d;
    xyz_s16_t Acc_I16;
    xyz_s16_t Gyro_I16;
    xyz_f_t imu_pos;
    xyz_f_t imu_att;
    xyz_f_t gps_pos;
    xyz_f_t Acc;
    xyz_f_t Acc_rt;
    xyz_f_t Gyro;
    xyz_f_t Gyro_deg;
    xyz_f_t Gyro_deg_rt;
    xyz_f_t Mag, Mag_rt, Mago, Mago_rt;
    xyz_f_t Acc_Offset;
    xyz_f_t Acc_Scale;
    xyz_f_t Gyro_Offset;
    xyz_f_t Gyro_Scale;
    xyz_f_t Gyro_Auto_Offset;
    xyz_f_t Gain_3d;
    xyz_f_t Off_3d;
    char Mag_CALIBRATE, Mag_Have_Param, Mag_ERR, Mag_update;
    xyz_s16_t Mag_Adc, Mag_Adc_o;
    xyz_f_t Mag_Offset, Mag_Offseto;
    xyz_f_t Mag_Offset_c, Mag_Offset_co;
    xyz_f_t Mag_Gain, Mag_Gaino;
    xyz_f_t Mag_Gain_c, Mag_Gain_co;
    xyz_f_t Mag_Val, Mag_Val_t, Mag_Valo, Mag_Val_to;
    float hmlOneMAG, hmlOneACC;
    float Yaw_Mag;
    float Ftempreature;
} _MEMS;

#define FRAME_HEAD 0xfc
#define FRAME_END 0xfd
#define TYPE_IMU 0x40
#define TYPE_AHRS 0x41
#define TYPE_INSGPS 0x42
#define TYPE_GROUND 0xf0
#define IMU_LEN 0x38
#define AHRS_LEN 0x30
#define INSGPS_LEN 0x42
#define FRAME_HEADER 0x7B
#define FRAME_TAIL 0x7D
#define SEND_DATA_SIZE 24
#define RECEIVE_DATA_SIZE 11

typedef struct {
    float gyroscope_x, gyroscope_y, gyroscope_z;
    float accelerometer_x, accelerometer_y, accelerometer_z;
    float magnetometer_x, magnetometer_y, magnetometer_z;
    float imu_temperature;
    float Pressure;
    float pressure_temperature;
    long Timestamp;
} IMUData_Packet_t;

typedef struct {
    float RollSpeed, PitchSpeed, HeadingSpeed;
    float Roll, Pitch, Heading;
    float Qw, Qx, Qy, Qz;
    long Timestamp;
} AHRSData_Packet_t;

#endif /* MOTOR_CONTROL_SPI_NODE_HPP */
