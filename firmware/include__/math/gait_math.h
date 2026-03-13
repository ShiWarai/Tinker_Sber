#ifndef GAIT_MATH_H
#define GAIT_MATH_H

#include <math.h>
#include <stdint.h>
#include "app_main.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define F_PI M_PI

// Low Pass Filter Coefficients (Time Constants RC = 1/(2*PI*fc))
#define LPF_COF_05Hz      (1.0f / (2.0f * F_PI * 0.5f))
#define LPF_COF_1t5Hz     (1.0f / (2.0f * F_PI * 3.0f))
#define LPF_COF_5t10Hz    (1.0f / (2.0f * F_PI * 7.0f))
#define LPF_COF_10t15Hz   (1.0f / (2.0f * F_PI * 12.0f))
#define LPF_COF_15t20Hz   (1.0f / (2.0f * F_PI * 17.0f))
#define LPF_COF_20t25Hz   (1.0f / (2.0f * F_PI * 22.0f))
#define LPF_COF_25t30Hz   (1.0f / (2.0f * F_PI * 27.0f))
#define LPF_COF_30t50Hz   (1.0f / (2.0f * F_PI * 40.0f))
#define LPF_COF_50t70Hz   (1.0f / (2.0f * F_PI * 60.0f))
#define LPF_COF_70t100Hz  (1.0f / (2.0f * F_PI * 80.0f))
#define LPF_COF_100tHz    (1.0f / (2.0f * F_PI * 100.0f))

// Functions
void DigitalLPF(float in, float* out, float cutoff_freq, float dt);
void DigitalLPF_Double(float in, double* out, float cutoff_freq, float dt);

float sind(float in);
float cosd(float in);
float To_180_degrees(float x);

void mat_trans(float src[3][3], float dis[3][3]);
void update_rotation_matrices(float pitch, float roll, float yaw, float Rn_b[3][3], float Rb_n[3][3]);

#endif /* GAIT_MATH_H */
