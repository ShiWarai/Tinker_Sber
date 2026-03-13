#include "drivers/icm20602.h"
#include "system/system_init.h"
#include "system/board_pins.h"
#include "drivers/flash.h"
#include "app/app_main.h"
#include <math.h>

#define ICM_CAL_SAMPLES  50

// External SPI Handle
extern SPI_HandleTypeDef hspi3;

// Global sensor data
_center_pos_st center_pos;
_sensor_rotate_st sensor_rot;
LIS3MDL_S lis3mdl, lis3mdl_cov;
uint8_t mpu_buffer[14];
uint8_t icm_id;

static float _accel_scale;
static float _gyro_scale;

// SPI helper functions
static uint8_t icm20602_read_write_byte(uint8_t TxData) {
    uint8_t Rxdata;
    HAL_SPI_TransmitReceive(&hspi3, &TxData, &Rxdata, 1, 100);
    return Rxdata;
}

uint8_t icm20602_write_reg(uint8_t reg, uint8_t val) {
    HAL_GPIO_WritePin(ICM_CS_PORT, ICM_CS_PIN, GPIO_PIN_RESET);
    icm20602_read_write_byte(reg & 0x7f);
    icm20602_read_write_byte(val);
    HAL_GPIO_WritePin(ICM_CS_PORT, ICM_CS_PIN, GPIO_PIN_SET);
    return 0;
}

uint8_t icm20602_read_reg(uint8_t reg) {
    uint8_t res;
    HAL_GPIO_WritePin(ICM_CS_PORT, ICM_CS_PIN, GPIO_PIN_RESET);
    icm20602_read_write_byte(reg | 0x80);
    res = icm20602_read_write_byte(0xff);
    HAL_GPIO_WritePin(ICM_CS_PORT, ICM_CS_PIN, GPIO_PIN_SET);
    return res;
}

uint8_t icm20602_read_buffer(uint8_t reg, uint8_t *buffer, uint16_t len) {
    HAL_GPIO_WritePin(ICM_CS_PORT, ICM_CS_PIN, GPIO_PIN_RESET);
    icm20602_read_write_byte(reg | 0x80);
    for (uint16_t i = 0; i < len; i++) {
        buffer[i] = icm20602_read_write_byte(0xff);
    }
    HAL_GPIO_WritePin(ICM_CS_PORT, ICM_CS_PIN, GPIO_PIN_SET);
    return 0;
}

uint8_t icm20602_set_accel_fullscale(uint8_t fs) {
    switch (fs) {
        case ICM20_ACCEL_FS_2G:  _accel_scale = 1.0f / 16348.0f; break;
        case ICM20_ACCEL_FS_4G:  _accel_scale = 1.0f / 8192.0f;  break;
        case ICM20_ACCEL_FS_8G:  _accel_scale = 1.0f / 4096.0f;  break;
        case ICM20_ACCEL_FS_16G: _accel_scale = 1.0f / 2048.0f;  break;
        default:
            fs = ICM20_ACCEL_FS_8G;
            _accel_scale = 1.0f / 4096.0f;
            break;
    }
    return icm20602_write_reg(ICM20_ACCEL_CONFIG, fs);
}

uint8_t icm20602_set_gyro_fullscale(uint8_t fs) {
    switch (fs) {
        case ICM20_GYRO_FS_250:  _gyro_scale = 1.0f / 131.068f; break;
        case ICM20_GYRO_FS_500:  _gyro_scale = 1.0f / 65.534f;  break;
        case ICM20_GYRO_FS_1000: _gyro_scale = 1.0f / 32.767f;  break;
        case ICM20_GYRO_FS_2000: _gyro_scale = 1.0f / 16.3835f; break;
        default:
            fs = ICM20_GYRO_FS_2000;
            _gyro_scale = 1.0f / 16.3835f;
            break;
    }
    return icm20602_write_reg(ICM20_GYRO_CONFIG, fs);
}

uint8_t icm20602_init(void) {
    icm20602_write_reg(ICM20_PWR_MGMT_1, 0x80); // Reset
    HAL_Delay(50);
    
    icm_id = icm20602_read_reg(ICM20_WHO_AM_I);
    if (icm_id != 0x12) return 1;
    
    icm20602_write_reg(ICM20_PWR_MGMT_1, 0x01); // Auto clock source
    HAL_Delay(10);
    
    icm20602_write_reg(ICM20_PWR_MGMT_2, 0x00);
    icm20602_write_reg(ICM20_SMPLRT_DIV, 0);
    icm20602_write_reg(ICM20_CONFIG, DLPF_BW_92);
    icm20602_write_reg(ICM20_ACCEL_CONFIG2, ACCEL_AVER_4 | 0x07); // 0x07 is ACCEL_DLPF_BW_21
    
    icm20602_set_accel_fullscale(ICM20_ACCEL_FS_8G);
    icm20602_set_gyro_fullscale(ICM20_GYRO_FS_2000);
    
    return 0;
}

void icm20602_read(void) {
    uint8_t buffer[14];
    icm20602_read_buffer(ICM20_ACCEL_XOUT_H, buffer, 14);
    
    lis3mdl.Acc_I16.x = (int16_t)((buffer[0] << 8) | buffer[1]);
    lis3mdl.Acc_I16.y = (int16_t)((buffer[2] << 8) | buffer[3]);
    lis3mdl.Acc_I16.z = (int16_t)((buffer[4] << 8) | buffer[5]);
    
    lis3mdl.Tempreature = (int16_t)((buffer[6] << 8) | buffer[7]);
    
    lis3mdl.Gyro_I16.x = (int16_t)((buffer[8] << 8) | buffer[9]);
    lis3mdl.Gyro_I16.y = (int16_t)((buffer[10] << 8) | buffer[11]);
    lis3mdl.Gyro_I16.z = (int16_t)((buffer[12] << 8) | buffer[13]);

    // Накопление сырых данных для калибровки (до вычитания смещения)
    static int32_t acc_sum[3]  = {0, 0, 0};
    static int32_t gyro_sum[3] = {0, 0, 0};
    static uint16_t acc_cal_cnt  = 0U;
    static uint16_t gyro_cal_cnt = 0U;

    if (lis3mdl.Acc_CALIBRATE) {
        acc_sum[0] += (int32_t)lis3mdl.Acc_I16.x;
        acc_sum[1] += (int32_t)lis3mdl.Acc_I16.y;
        acc_sum[2] += (int32_t)lis3mdl.Acc_I16.z;
        acc_cal_cnt++;
        if (acc_cal_cnt >= ICM_CAL_SAMPLES) {
            mems.Acc_Offset.x = acc_sum[0] / (int32_t)ICM_CAL_SAMPLES;
            mems.Acc_Offset.y = acc_sum[1] / (int32_t)ICM_CAL_SAMPLES;
            mems.Acc_Offset.z = acc_sum[2] / (int32_t)ICM_CAL_SAMPLES;
            acc_sum[0] = acc_sum[1] = acc_sum[2] = 0;
            acc_cal_cnt = 0U;
            lis3mdl.Acc_CALIBRATE = 0;
            WRITE_PARM();
        }
    }

    if (lis3mdl.Gyro_CALIBRATE) {
        gyro_sum[0] += (int32_t)lis3mdl.Gyro_I16.x;
        gyro_sum[1] += (int32_t)lis3mdl.Gyro_I16.y;
        gyro_sum[2] += (int32_t)lis3mdl.Gyro_I16.z;
        gyro_cal_cnt++;
        if (gyro_cal_cnt >= ICM_CAL_SAMPLES) {
            mems.Gyro_Offset.x = gyro_sum[0] / (int32_t)ICM_CAL_SAMPLES;
            mems.Gyro_Offset.y = gyro_sum[1] / (int32_t)ICM_CAL_SAMPLES;
            mems.Gyro_Offset.z = gyro_sum[2] / (int32_t)ICM_CAL_SAMPLES;
            gyro_sum[0] = gyro_sum[1] = gyro_sum[2] = 0;
            gyro_cal_cnt = 0U;
            lis3mdl.Gyro_CALIBRATE = 0;
            WRITE_PARM();
        }
    }

    // Применение сохранённых смещений (целочисленно, до конвертации в float)
    lis3mdl.Gyro_I16.x -= mems.Gyro_Offset.x;
    lis3mdl.Gyro_I16.y -= mems.Gyro_Offset.y;
    lis3mdl.Gyro_I16.z -= mems.Gyro_Offset.z;

    lis3mdl.Acc_I16.x -= mems.Acc_Offset.x;
    lis3mdl.Acc_I16.y -= mems.Acc_Offset.y;
    lis3mdl.Acc_I16.z -= mems.Acc_Offset.z;
    
    // Convert to physical units
    lis3mdl.Acc.x = (float)lis3mdl.Acc_I16.x * _accel_scale;
    lis3mdl.Acc.y = (float)lis3mdl.Acc_I16.y * _accel_scale;
    lis3mdl.Acc.z = (float)lis3mdl.Acc_I16.z * _accel_scale;
    
    // Rad/s
    float g_scale = _gyro_scale * (3.14159265f / 180.0f); // Assuming _gyro_scale was raw to deg/s
    // Actually in original it was already multiplied by _DEG_TO_RAD.
    lis3mdl.Gyro.x = (float)lis3mdl.Gyro_I16.x * _gyro_scale;
    lis3mdl.Gyro.y = (float)lis3mdl.Gyro_I16.y * _gyro_scale;
    lis3mdl.Gyro.z = (float)lis3mdl.Gyro_I16.z * _gyro_scale;
}
