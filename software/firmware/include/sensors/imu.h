#ifndef _IMU_H_
#define _IMU_H_

#include "stm32f4xx.h"

#include "parameter.h"
#include "gait_math.h"

typedef struct
{
	xyz_f_t err;
	xyz_f_t err_tmp;
	xyz_f_t err_lpf;
	xyz_f_t err_Int;
	xyz_f_t g;

} ref_t;
int madgwick_update_new(float T, float wx, float wy, float wz, float ax, float ay, float az, float *rol, float *pit,
                        float *yaw);
extern float reference_vr[3];
void IMUupdate(float half_T, float gx, float gy, float gz, float ax, float ay, float az, float *rol, float *pit,
               float *yaw);
extern float Roll, Pitch, Yaw;
#endif
