#ifndef _USART_H
#define _USART_H

#include "stm32f4xx.h"

typedef struct
{
	float x;
	float y;
	float z;
	float zz;
} END_POSS;

extern u8 KEY[8];

typedef struct
{
	float ip1;
	float ip2;
	float extcan_link;
	float spi_link;
	float can1_link;
	float can2_link;
} _Robot;

extern _Robot robot;

void Uart6_Init(u32 br_num);
void Uart6_SendBytes(const uint8_t *data, uint16_t length);
void Uart6_SendString(const char *str);
uint16_t Uart6_Available(void);
uint16_t Uart6_Read(uint8_t *data, uint16_t max_len);
void Uart6_PollCommandLine(void);

typedef struct int16_rcget
{
	int16_t ROLL;
	int16_t PITCH;
	int16_t THROTTLE;
	int16_t YAW;
	int16_t MOV1, MOV2;
	int16_t AUX1;
	int16_t AUX2;
	int16_t AUX3;
	int16_t AUX4;
	int16_t AUX5;
	int16_t AUX6;
	float SBUS_CH[6];
	int16_t AUX_SEL1, AUX_SEL2, AUX_SEL3, AUX_SEL4, AUX_SEL5, AUX_SEL6;
	int16_t HEIGHT_MODE;
	int16_t POS_MODE;
	u8 update, Heart, Heart_rx, Heart_error;
	int lose_cnt, lose_cnt_rx;
	u8 connect;
	float signal_rate;
	int16_t RST;
} RC_GETDATA;

extern RC_GETDATA Rc_Get, Rc_Get_PWM, Rc_Get_SBUS, Rc_Wifi;

typedef struct
{
	int connect;
	char power;
	int cap;
	int mode;
	int loss_connect;
	float att_set[3];
	END_POSS pos_set;
	float att_now[3];
	END_POSS pos_now;
	float dis;
	float move_spd;
	float torque;
} _ARMSS;

extern _ARMSS arm_cmd_s;

typedef struct
{
	int connect;
	int cnt_loss;
	float dt;
	float att[3];
	float rate[3];
	float acc_b[3];
	float acc_n[3];
} _IMUO;

extern _IMUO imuo;

typedef struct
{
	char check;
	int x;
	int y;
	int w;
	int h;
	int s;
	float angle;
	float pos[3], att[3];
} _PIX_TAR;

typedef struct
{
	_PIX_TAR cube, color, face, line;
	float target_pos[3];
	float target_att[3];

	float cmd_spd[3];
	float cmd_pos[3];
	float cmd_att[4];
	u8 power;
	u8 cmd_mode;
	u8 visual_mode;
	u8 connect;
	u16 lost_cnt;
} _PI;

extern _PI pi;

typedef struct
{
	int origin[2];
	float spd_o[2];
	float spd_flt[2];
	float dis;
	u8 mode;
	u8 connect;
	u16 lost_cnt;
} _ODOMETER;

extern _ODOMETER flow;

typedef struct
{
	u8 mode;
	u8 connect;
	u16 lost_cnt;
	float pos[3], spd[3], att[3];
} _LINK_CMD;

extern _LINK_CMD o_cmd;

typedef struct
{
	u8 mode;
	u8 connect;
	u16 loss_cnt;
	u8 type;
	int pix[3], spd_o[3];
	float ang[3], out[3];
	float spd[3], spd_compass[3], height;
	float spd_flt[3], height_flt;
	float spdo[3], out_reg[3], spdo_flt[3];
	float k_all, cut_rate;
	float k_att_compass[3];
} _FLOW;

extern _FLOW optical_flow;

typedef struct
{
	float Pit, Rol, Yaw;
	double Lat, Lon;
	float H, H_Spd, H_G;
	float q[4];
	u8 GPS_STATUS;
	float X_Spd_b, Y_Spd_b, X_Spd_n, Y_Spd_n;
	int Dop[6];
	u8 STATUS;
	float Bat;
	int Rc_pit, Rc_rol, Rc_yaw, Rc_thr, Rc_mode, Rc_gear;
	u8 m100_connect, m100_data_refresh;
	double Init_Lat, Init_Lon;
	float Y_Pos_local, X_Pos_local, spd[3];
	long r1, r2;
	float rx_dt;
	u8 control_connect, connect;
	u16 control_loss, cnt_m100_data_refresh, loss_cnt;
	u8 px4_tar_mode;
	float control_spd[3];
	float control_yaw;
	float uwb_o[3];
	u8 save_data, save_video;
	u8 navigation_mode;
	float uwb_f[4];
} M100;

extern M100 m100, px4;

typedef struct
{
	u8 connect;
	u8 mode;
	int cmd_flag;
	float rad_set;

	float current;
	float tau_now;
	float rad_now;
	float q_now;
	float v_now;
	char err;
	float r;
	float dt_rx;
	float i_2_tau;
	int rx_cnt;
	int loss_cnt;
} _WHEEL_WX;

typedef struct
{
	float w_set;
	float v_set;
	float v_set_flt;
	float w_now, w_imu;
	float v_now;
	float r;
	float x_n, yn;
	float vx_n, vy_n;
	float h;
	float ws;
	float w_set_flt, w;
} _WHEEL_2Dof;

extern _WHEEL_2Dof _wheel_2d;
extern _WHEEL_WX _wheel_wx[4];
extern float ws_set_flt;

extern u8 RxState1;
extern int pwm_dj[5];
extern int time_dj[5];
extern int16_t BLE_DEBUG[16];

#if USE_AUDIO
void Write_Audio_Data(uint8_t dat);
#endif

#endif
