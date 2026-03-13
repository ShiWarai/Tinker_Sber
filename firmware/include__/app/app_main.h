#ifndef APP_MAIN_H
#define APP_MAIN_H

#include <stdint.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define LIMIT(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#define NUM_MOTORS 10

typedef float fp32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;

typedef struct {
    float x, y, z;
} xyz_f_t;

typedef struct {
    int16_t x, y, z;
} xyz_s16_t;

typedef struct {
    int32_t x, y, z;
} xyz_s32_t;

typedef struct {
    xyz_s32_t Gyro_Offset;
    xyz_s32_t Acc_Offset;
    xyz_s32_t Mag_Offset;
    xyz_f_t Mag_Gain;
    xyz_f_t imu_att;
} _MEMS;

typedef struct {
    float tar_att_bias[3];
    float Rn_b[3][3];
    float Rb_n[3][3];
    float att[3];      // Standard attitude copy
    float acc_b[3];    // Acceleration in body frame
    float acc_n[3];    // Acceleration in world frame
    float rate[3];     // Angular rates
    uint8_t beep_state;
} VMC_ALL;

extern _MEMS mems;
extern VMC_ALL vmc_all;

typedef enum {
  MOTOR_TYPE_NONE = 0,
  DM_6006,
  DM_8006
} motor_type_e;

typedef struct {
  int id;
  char connect;
  int loss_cnt;
  int usb_cmd_mode;
  int control_mode;
  float rx_dt;
  float q_reset_angle;
  int16_t speed_rpm;
  int over_t_flag;
  float over_t_reg;
  float real_current;
  float given_current;
  float given_torque;
  int32_t round_cnt;
  int cnt_rotate;
  float set_q;
  float total_angle_out, total_angle_out_single, total_angle_out_reg;
  float total_angle_out_off;
  float t_scale;
  float q_now_reg;
  float qd_now_reg;
  float t_now_reg;
  float flt_q;
  float flt_qd;
  float flt_t;
  float bat_v;
  int t_inv_flag_measure;
  int t_inv_flag_cmd;
  int q_flag;
  int err_flag;
} motor_param_t;

typedef struct {
  char ready;
  char cal_zero_state;
  float cal_zero_spd;
  float cal_zero_thr;
  motor_type_e type;
  char anal_type;
} motor_param_m;

typedef struct {
  char en_cmd, en_cmd_ocu;
  char reset_q, reset_q_rx, reset_q_en;
  char reset_q_lock;
  char reset_q_cnt;
  char cmd_mode, cal_div;
  float reset_q_delay_timer;
  char clear_err;
  char en_pos_trig;
  float set_q, set_q_test, set_q_test_bias;
  float set_q_mit_off;
  float set_qd;
  float set_t;
  float q_now, q_now_flt;
  float qd_now, qd_now_flt;
  float t_now, t_now_flt;
  float stiff;
  float kp;
  float ki;
  float kd;
  float max_t;
  float cmd_t, given_current_cmd, current_cmd_tx;
  motor_param_t param;
  motor_param_m motor;
} motor_measure_t;

typedef struct {
  char connect;
  char connect_motor[10];
  char ready[10];
  char reset_q, reset_err;
  char motor_en, motor_en_reg;
  char motor_mode, motor_mode_reg;
  char err_flag[10];
  float q_now[10], qd_now[10], qdd_now[10];
  float t_now[10];
  float q_set[10], qd_set[10], qdd_set[10];
  float set_t[10], set_t_flt[10];
  float kp[10];
  float kd[10];
  float q_reset[10];
  float stiff[10];
  float set_i[10], set_i_flt[10];
  float set_t_w[4];
  float set_i_w[4];
  float dq_now_w[4];
  float t_now_w[4];
  float max_t[10];
  float max_t_w[4];
} _LEG_MOTOR;

extern _LEG_MOTOR leg_motor;
extern motor_measure_t motor_chassis[10];

void app_main(void);
void mit_bldc_thread(char en_all, float dt);

float To_PI(float x);
float Moving_Median(uint8_t item, uint8_t width_num, float in);
float Get_Cycle_T(uint8_t item);

#endif /* APP_MAIN_H */
