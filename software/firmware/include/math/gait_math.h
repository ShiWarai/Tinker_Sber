#ifndef __GAIT_MATH_H__
#define __GAIT_MATH_H__
#include "base_struct.h"
#include <math.h>

#define TAN_MAP_RES 0.003921569f /* (smallest non-zero value in table) */
#define RAD_PER_DEG 0.017453293f
#define TAN_MAP_SIZE 256
#define MY_PPPIII 3.14159f
#define MY_PPPIII_HALF 1.570796f

#define ABS(x) ((x) > 0 ? (x) : -(x))
#define LIMIT(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

//-----------------------------------common math-----------------------
float sindw(float in);
float cosdw(float in);
float To_180_degreesw(float x);
float To_360_degreesw(float x);
float limitw(float x, float min, float max);
float deadw(float x, float zoom);
float my_abs(float f);
float fast_atan2(float y, float x);
float my_atan(float x, float y);
float my_pow(float a);
float my_sqrt(float number);
double mx_sin(double rad);
double my_sin(double rad);
float my_cos(double rad);
float my_deathzoom(float x, float zoom);
float my_deathzoom_2(float x, float zoom);
float my_deathzoom_rc(float x, float zoom);
float limit_mine(float x, float zoom);
float limit_mine2(float x, float min, float max);
float To_180_degrees(float x);
/** Нормализация угла в диапазон [-PI, PI] (радианы) */
float To_PI(float x);
float my_pow_2_curve(float in, float a, float max);
float fast_sqrt(float number);
float cosd(double in);
float tand(double in);
float sqrtd(double in);
float sind(float in);
float dead(float x, float zoom);
void invet22(const float A[4], float *dA, float inA[4]);
void invet33(const float A[9], float *dA, float inA[9]);

//----------------------------------RT Matrix math---------------------
void mat_trans(float src[3][3], float dis[3][3]);
//------------------------------------Filter math-----------------------------------------------------------
#define F_PI 3.1415926
#define LPF_COF_05Hz 1.0f / (2 * F_PI * 0.5)
#define LPF_COF_1t5Hz 1.0f / (2 * F_PI * 3)
#define LPF_COF_5t10Hz 1.0f / (2 * F_PI * 7)
#define LPF_COF_10t15Hz 1.0f / (2 * F_PI * 12)
#define LPF_COF_15t20Hz 1.0f / (2 * F_PI * 17)
#define LPF_COF_20t25Hz 1.0f / (2 * F_PI * 22)
#define LPF_COF_25t30Hz 1.0f / (2 * F_PI * 27)
#define LPF_COF_30t50Hz 1.0f / (2 * F_PI * 40)
#define LPF_COF_50t70Hz 1.0f / (2 * F_PI * 60)
#define LPF_COF_70t100Hz 1.0f / (2 * F_PI * 80)
#define LPF_COF_100tHz 1.0f / (2 * F_PI * 100)
#define MED_WIDTH_NUM 20
#define MED_FIL_ITEM 30

/* 2 Dimension */
typedef struct
{
	float x[2];    /* state: [0]-angle [1]-diffrence of angle, 2x1 */
	float A[2][2]; /* X(n)=A*X(n-1)+U(n),U(n)~N(0,q), 2x2 */
	float H[2];    /* Z(n)=H*X(n)+W(n),W(n)~N(0,r), 1x2   */
	float q[2];    /* process(predict) noise convariance,2x1 [q0,0; 0,q1] */
	float r;       /* measure noise convariance */
	float p[2][2]; /* estimated error convariance,2x2 [p0 p1; p2 p3] */
	float gain[2]; /* 2x1 */
} kalman2_state;
extern void kalman2_init(kalman2_state *state, float *init_x, float (*init_p)[2]);
extern float kalman2_filter(kalman2_state *state, float z_measure);

void DigitalLPF(float in, float *out, float cutoff_freq, float dt);
void DigitalLPF_Double(float in, double *out, float cutoff_freq, float dt);
void Moving_Average(float in, float moavarray[], u16 len, u16 fil_cnt[2], float *out);

extern float med_filter_tmp[MED_FIL_ITEM][MED_WIDTH_NUM];
extern float med_filter_out[MED_FIL_ITEM];
extern u8 med_fil_cnt[MED_FIL_ITEM];
float Moving_Median(u8 item, u8 width_num, float in);

void kalman2_init(kalman2_state *state, float *init_x, float (*init_p)[2]);
float kalman2_filter(kalman2_state *state, float z_measure);
void DigitalLPFw(float in, float *out, float cutoff_freq, float dt);

typedef struct
{ // control parameter
	float h0;
	float v1, v2, r0;
} ESO_X;

#define ESO_AngularRate_his_length 4

typedef struct
{
	float beta1;
	float beta2;

	float T;
	float invT;
	float z_inertia;
	float z1;
	float z2;
	float u;
	float his_z1[ESO_AngularRate_his_length];

	float h;

	char err_sign;
	float err_continues_time;

	float b;
} ESO_AngularRate;

extern ESO_AngularRate leg_td2[4][3];

typedef struct
{
	unsigned char tracking_mode;

	float x1;
	float x2;
	float x3;
	float x4;

	float P1;
	float P2;
	float P3;
	float P4;

	float r2p, r2n, r3p, r3n, r4p, r4n;
} _TD4;

extern ESO_AngularRate leg_td2[4][3];
extern _TD4 leg_td4[4][3];
extern _TD4 odom_td[3];
extern _TD4 bldc_td4[4][3];

void TD4_reset(_TD4 *filter);
void TD4_init(_TD4 *filter, float P1, float P2, float P3, float P4);
float TD4_track4(_TD4 *filter, const float expect, const float h);
float TD4_track3(_TD4 *filter, const float expect, const float h);
void init_ESO_AngularRate(ESO_AngularRate *eso, float T, float b, float beta1, float beta2);
void ESO_AngularRate_update_u(ESO_AngularRate *eso, float u);
float ESO_AngularRate_run(ESO_AngularRate *eso, const float v, const float h);
float sign(float x);

#endif
