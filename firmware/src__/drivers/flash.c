#include "flash.h"
#include "flash_w25.h"
#include "app_main.h"
#include <math.h>
#include <string.h>

#define FLASH_SIZE (8*1024*1024) // 8MB for W25Q64
#define PARAM_ADDR (FLASH_SIZE - (SIZE_PARAM + 10))

static int flash_cnt = 0;

static void setDataInt(uint8_t *buf, int val) {
    buf[flash_cnt++] = (val >> 0) & 0xFF;
    buf[flash_cnt++] = (val >> 8) & 0xFF;
    buf[flash_cnt++] = (val >> 16) & 0xFF;
    buf[flash_cnt++] = (val >> 24) & 0xFF;
}

static void setDataFloat(uint8_t *buf, float val) {
    uint32_t i;
    memcpy(&i, &val, 4);
    buf[flash_cnt++] = (i >> 0) & 0xFF;
    buf[flash_cnt++] = (i >> 8) & 0xFF;
    buf[flash_cnt++] = (i >> 16) & 0xFF;
    buf[flash_cnt++] = (i >> 24) & 0xFF;
}

static int intFromData(uint8_t *buf, int *pos) {
    int val = 0;
    val |= (buf[*pos + 0] << 0);
    val |= (buf[*pos + 1] << 8);
    val |= (buf[*pos + 2] << 16);
    val |= (buf[*pos + 3] << 24);
    *pos += 4;
    return val;
}

static float floatFromData(uint8_t *buf, int *pos) {
    uint32_t i = 0;
    float val;
    i |= (buf[*pos + 0] << 0);
    i |= (buf[*pos + 1] << 8);
    i |= (buf[*pos + 2] << 16);
    i |= (buf[*pos + 3] << 24);
    memcpy(&val, &i, 4);
    *pos += 4;
    if (isnan(val)) return 0.0f;
    return val;
}

void READ_PARM(void) {
    uint8_t buf[SIZE_PARAM];
    int pos = 0;
    W25QXX_Read(buf, PARAM_ADDR, SIZE_PARAM);

    mems.Gyro_Offset.x = intFromData(buf, &pos);
    mems.Gyro_Offset.y = intFromData(buf, &pos);
    mems.Gyro_Offset.z = intFromData(buf, &pos);

    mems.Acc_Offset.x = intFromData(buf, &pos);
    mems.Acc_Offset.y = intFromData(buf, &pos);
    mems.Acc_Offset.z = intFromData(buf, &pos);

    mems.Mag_Offset.x = intFromData(buf, &pos);
    mems.Mag_Offset.y = intFromData(buf, &pos);
    mems.Mag_Offset.z = intFromData(buf, &pos);

    mems.Mag_Gain.x = floatFromData(buf, &pos);
    mems.Mag_Gain.y = floatFromData(buf, &pos);
    mems.Mag_Gain.z = floatFromData(buf, &pos);

    // Skip secondary mag params if they aren't needed, but keep offset for alignment
    pos += (3 * 4) + (3 * 4); 

    vmc_all.tar_att_bias[0] = floatFromData(buf, &pos); // PITr
    vmc_all.tar_att_bias[1] = floatFromData(buf, &pos); // ROLr
}

void WRITE_PARM(void) {
    uint8_t buf[SIZE_PARAM];
    memset(buf, 0, SIZE_PARAM);
    flash_cnt = 0;

    setDataInt(buf, mems.Gyro_Offset.x);
    setDataInt(buf, mems.Gyro_Offset.y);
    setDataInt(buf, mems.Gyro_Offset.z);

    setDataInt(buf, mems.Acc_Offset.x);
    setDataInt(buf, mems.Acc_Offset.y);
    setDataInt(buf, mems.Acc_Offset.z);

    setDataInt(buf, mems.Mag_Offset.x);
    setDataInt(buf, mems.Mag_Offset.y);
    setDataInt(buf, mems.Mag_Offset.z);

    setDataFloat(buf, mems.Mag_Gain.x);
    setDataFloat(buf, mems.Mag_Gain.y);
    setDataFloat(buf, mems.Mag_Gain.z);

    // Reserved for secondary mag params (alignment with legacy)
    flash_cnt += (3 * 4) + (3 * 4);

    setDataFloat(buf, vmc_all.tar_att_bias[0]);
    setDataFloat(buf, vmc_all.tar_att_bias[1]);

    W25QXX_Write(buf, PARAM_ADDR, SIZE_PARAM);
}
