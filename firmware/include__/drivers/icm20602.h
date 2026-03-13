#ifndef _DRV_ICM20602_H_
#define _DRV_ICM20602_H_

#include <stdint.h>
#include "app_main.h"

// Original sensor structures
typedef struct {
    float center_pos_cm[3];
    float gyro_rad[3];
    float gyro_rad_old[3];
    float gyro_rad_acc[3];
    float linear_acc[3];
} _center_pos_st;

typedef struct {
    uint8_t surface_CALIBRATE;
    float surface_vec[3];
    float surface_unitvec[3];
} _sensor_rotate_st;

typedef struct { 
    uint8_t Cali_3d;	
    char Acc_CALIBRATE;
    char Gyro_CALIBRATE;
    xyz_s16_t Acc_I16;
    xyz_s16_t Gyro_I16;
    xyz_f_t Acc, Acc_t;
    xyz_f_t Gyro, Gyro_t;
    xyz_f_t Gyro_deg, Gyro_deg_t;
    xyz_f_t Acc_Offset;
    xyz_f_t Gyro_Offset;
    uint8_t acc_cal_3d_step;	
    xyz_f_t Gain_3d;
    xyz_f_t Off_3d;
    float att_off[2];
    xyz_f_t Gyro_Auto_Offset;
    float Temprea_Offset;
    float Gyro_Temprea_Adjust;
    float ACC_Temprea_Adjust;
    int16_t Tempreature;
    float TEM_LPF;
    float Ftempreature;
    xyz_s16_t Mag_Adc;
    xyz_f_t Mag_Offset, Mag_Offseto;
    xyz_f_t Mag_Offset_c, Mag_Offset_co;
    xyz_f_t Mag_Gain, Mag_Gaino;
    xyz_f_t Mag_Gain_c, Mag_Gain_co;
    xyz_f_t Mag_Val, Mag_Val_t, Mag_Valo, Mag_Val_to;
    uint8_t Mag_CALIBRATED;
    float yaw;
    float Pressure;
    float Tem_bmp;
    float Alt;
    float hmlOneMAG, hmlOneACC;
} LIS3MDL_S;

extern _center_pos_st center_pos;
extern _sensor_rotate_st sensor_rot;
extern LIS3MDL_S lis3mdl, lis3mdl_cov;
extern uint8_t mpu_buffer[14];

// Device Registers
#define ICM20_SMPLRT_DIV      0x19
#define ICM20_CONFIG          0x1A
#define ICM20_GYRO_CONFIG     0x1B
#define ICM20_ACCEL_CONFIG    0x1C
#define ICM20_ACCEL_CONFIG2   0x1D
#define ICM20_LP_MODE_CFG     0x1E
#define ICM20_FIFO_EN         0x23
#define ICM20_ACCEL_XOUT_H    0x3B
#define ICM20_TEMP_OUT_H      0x41
#define ICM20_GYRO_XOUT_H     0x43
#define ICM20_PWR_MGMT_1      0x6B
#define ICM20_PWR_MGMT_2      0x6C
#define ICM20_WHO_AM_I        0x75

// Configuration Constants
#define ICM20_ACCEL_FS_2G     (0<<3)
#define ICM20_ACCEL_FS_4G     (1<<3)
#define ICM20_ACCEL_FS_8G     (2<<3)
#define ICM20_ACCEL_FS_16G    (3<<3)

#define ICM20_GYRO_FS_250     (0<<3)
#define ICM20_GYRO_FS_500     (1<<3)
#define ICM20_GYRO_FS_1000    (2<<3)
#define ICM20_GYRO_FS_2000    (3<<3)

#define DLPF_BW_92            0x02
#define ACCEL_AVER_4          (0x00<<4)
#define ACCEL_DLPF_BW_21      0x07

// Exported Functions
uint8_t icm20602_init(void);
void icm20602_read(void);
uint8_t icm20602_set_accel_fullscale(uint8_t fs);
uint8_t icm20602_set_gyro_fullscale(uint8_t fs);

#endif /* _DRV_ICM20602_H_ */
