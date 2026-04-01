#include "scheduler.h"
#include "include.h"
#include "imu.h"
#include "flash.h"
#include "led_fc.h"
#include "rc_mine.h"
#include "watch_dog.h"
#include "usart_fc.h"
#include "beep.h"
#include "spi.h"
#include "can.h"
#include "beep.h"
#include "gait_math.h"
#include "wsled.h"
#include "Custom_SPI_DEVICE.h"

typedef struct
{
	float fake_yaw;
} _NAV;

VMC vmc[4];
VMC_ALL vmc_all;
robotTypeDef robotwb;
_OCU ocu, ocu_rx;
_NAV nav;
POS_FORCE_PARM pos_force_p;
VMC_ROBOT_PARM vmc_robot_p;
float MIN_Z = -0.1;
float MAX_Z = -0.19;
float MIN_Y = -0.1;
float MAX_Y = -0.1;
float MIN_X = -0.15;
float MAX_X = 0.15;
float MAX_SPD = 0;
float MAX_SPD_RAD = 50;

// Отладка ST-Link: значение вычисленного z в тестовом блоке.
// volatile — чтобы в отладчике значение не «исчезало» при -O.
volatile float debug_z = 0.0f;

_SYSTEM_DT system_dt;
s16 loop_cnt;
loop_t loop;
float leg_dt[GET_TIME_NUM];
float trig_test_dt[3] = {0};
void Loop_check() // прерывание таймера
{
	loop.time++; // u16
	loop.cnt_2ms++;
	loop.cnt_5ms++;
	loop.cnt_10ms++;
	loop.cnt_20ms++;
	loop.cnt_50ms++;
	loop.cnt_1s++;
	loop.cnt_2s++;
	if (loop.time < 0)
		loop.time = 0;
	if (loop.cnt_2ms < 0)
		loop.cnt_2ms = 0;
	if (loop.cnt_5ms < 0)
		loop.cnt_5ms = 0;
	if (loop.cnt_10ms < 0)
		loop.cnt_10ms = 0;
	if (loop.cnt_20ms < 0)
		loop.cnt_20ms = 0;
	if (loop.cnt_50ms < 0)
		loop.cnt_50ms = 0;
	if (loop.cnt_1s < 0)
		loop.cnt_1s = 0;
	if (loop.cnt_2ms < 0)
		loop.cnt_2ms = 0;
	if (loop.cnt_5ms < 0)
		loop.cnt_5ms = 0;
	if (loop.cnt_10ms < 0)
		loop.cnt_10ms = 0;
	if (loop.cnt_20ms < 0)
		loop.cnt_20ms = 0;
	if (loop.cnt_50ms < 0)
		loop.cnt_50ms = 0;
	if (loop.cnt_2s < 0)
		loop.cnt_2s = 0;

	if (loop.check_flag == 1)
	{
		loop.err_flag++; // цикл не успел завершиться за отведённое время
	}
	else
	{
		loop.check_flag = 1; // сбрасывается в конце основного цикла
	}
	if (loop.err_flag > 9999)
		loop.err_flag = 0;
}

void Duty_Servo()
{
	system_dt.can_task = leg_dt[0] = Get_Cycle_T(0);
	// if(leg_dt[0]>0.00225)
	// 	can_rx_over[4]++;

	CAN_motor_sm(leg_dt[0]);
}

#define EN_GYRO_Z_F_ODOM 1
u8 en_nav = 1;
float FLT_ACC = 10;
float FLT_ATT_RT = 0; // 20;//15;//1.68*2;
char att_fusion_use[2] = {1, 1};
Vect3 vect_n_test, vect_b_test;
float FLT_ATT_RATE = 0; // WS

static void copy_imu_to_robotwb(float dt)
{
	char i, j;
	robotwb.IMU_now.pitch = vmc_all.att[PITr];
	robotwb.IMU_now.roll = vmc_all.att[ROLr];
	robotwb.IMU_now.yaw = vmc_all.att[YAWr];

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			robotwb.Rb_n[i][j] = vmc_all.Rb_n[i][j];
			robotwb.Rn_b[i][j] = vmc_all.Rn_b[i][j];
			robotwb.Rb_n_noroll[i][j] = vmc_all.Rb_n_noroll[i][j];
			robotwb.Rn_b_noroll[i][j] = vmc_all.Rn_b_noroll[i][j];
		}
	}

	DigitalLPF_Double(vmc_all.att_rate[PITr], &robotwb.IMU_dot.pitch, FLT_ATT_RATE, dt);
	DigitalLPF_Double(vmc_all.att_rate[ROLr], &robotwb.IMU_dot.roll, FLT_ATT_RATE, dt);
	DigitalLPF_Double(vmc_all.att_rate[YAWr], &robotwb.IMU_dot.yaw, FLT_ATT_RATE, dt);

	robotwb.now_att = robotwb.IMU_now;
	robotwb.now_rate = robotwb.IMU_dot;
}

void Duty_Att_Fushion() // оценка ориентации (AHRS), ~100 Гц
{
	u8 i;
	static u8 init;
	static u16 cnt_init;
	static float timer_baro;
	static float att_rt_use[3];
	float T;
	if (!init)
	{
		init = 1;
	}
	system_dt.ahrs_task = T = leg_dt[3] = Get_Cycle_T(3);

	if (!module.flash_lock && !imuo.connect)
	{
		IMU_Read();
		IMU_Data_Prepare(T);
	}

	if (cnt_init++ > 1 / T && !imuo.connect)
	{
		cnt_init = 65530;
#if defined(ATT_MAD)
		madgwick_update_new(T, mems.Gyro_deg_rt.x / 57.3, mems.Gyro_deg_rt.y / 57.3, mems.Gyro_deg_rt.z / 57.3,
		                    mems.Acc_rt.x, mems.Acc_rt.y, mems.Acc_rt.z, &Pitch, &Roll, &Yaw);
#endif
#if defined(ATT_COM) //&&!defined(BOARD_FOR_CAN)
		IMUupdate(T / 2, mems.Gyro_deg_rt.x, mems.Gyro_deg_rt.y, mems.Gyro_deg_rt.z, mems.Acc_rt.x, mems.Acc_rt.y,
		          mems.Acc_rt.z, &Pitch, &Roll, &Yaw);
#endif
		float a_br[3], acc_temp[3];
		static float acc_flt[3];
		a_br[0] = (float)mems.Acc_rt.x / 4096.;
		a_br[1] = (float)mems.Acc_rt.y / 4096.;
		a_br[2] = (float)mems.Acc_rt.z / 4096.;
		acc_temp[0] = a_br[1] * reference_vr[2] - a_br[2] * reference_vr[1];
		acc_temp[1] = a_br[2] * reference_vr[0] - a_br[0] * reference_vr[2];
		acc_temp[2] = reference_vr[2] * a_br[2] + reference_vr[0] * a_br[0] + reference_vr[1] * a_br[1] - 1;

		vmc_all.acc_nn.x = acc_temp[0] * 9.81;
		vmc_all.acc_nn.y = acc_temp[1] * 9.81;
		vmc_all.acc_nn.z = acc_temp[2] * 9.81;

#if defined(CODE_VER2)
		vmc_all.att[PITr] = Pitch - vmc_all.tar_att_bias[PITr] * 0;
		vmc_all.att[ROLr] = -Roll - vmc_all.tar_att_bias[ROLr] * 0;
		vmc_all.att[YAWr] = Yaw;
#else
		vmc_all.att[PITr] = Pitch;
		vmc_all.att[ROLr] = -Roll;
		vmc_all.att[YAWr] = Yaw;
#endif
		vmc_all.acc_b.x = a_br[0];
		vmc_all.acc_b.y = a_br[1];
		vmc_all.acc_b.z = a_br[2];

		vmc_all.acc_n.x = -vmc_all.acc_b.x * sind(vmc_all.att[YAWr]) + vmc_all.acc_b.y * cosd(vmc_all.att[YAWr]);
		vmc_all.acc_n.y = vmc_all.acc_b.x * cosd(vmc_all.att[YAWr]) + vmc_all.acc_b.y * sind(vmc_all.att[YAWr]);

		DigitalLPF(vmc_all.att[PITr], &att_rt_use[PITr], FLT_ATT_RT, T);
		DigitalLPF(vmc_all.att[ROLr], &att_rt_use[ROLr], FLT_ATT_RT, T);
		DigitalLPF(vmc_all.att[YAWr], &att_rt_use[YAWr], FLT_ATT_RT, T);
		//	if(fabs(vmc_all.att[ROLr])>15)
		//		att_rt_use[ROLr]=0;
		att_rt_use[YAWr] = 0;
		vmc_all.Rn_b[0][0] = cosd(-att_rt_use[PITr]) * cosd(-att_rt_use[YAWr]);
		// cy cz
		vmc_all.Rn_b[1][0] = -cosd(-att_rt_use[ROLr]) * sind(-att_rt_use[YAWr]) +
		                     sind(-att_rt_use[PITr]) * sind(-att_rt_use[ROLr]) * cosd(-att_rt_use[YAWr]);
		//-cx sz + sy sx cz
		vmc_all.Rn_b[2][0] = sind(-att_rt_use[ROLr]) * sind(-att_rt_use[YAWr]) +
		                     cosd(-att_rt_use[ROLr]) * sind(-att_rt_use[PITr]) * cosd(-att_rt_use[YAWr]);
		// sx sz + cx sy cz

		vmc_all.Rn_b[0][1] = cosd(-att_rt_use[PITr]) * sind(-att_rt_use[YAWr]);
		// cy sz
		vmc_all.Rn_b[1][1] = cosd(-att_rt_use[ROLr]) * cosd(-att_rt_use[YAWr]) +
		                     sind(-att_rt_use[ROLr]) * sind(-att_rt_use[PITr]) * sind(-att_rt_use[YAWr]);
		// cx cz + sx sy sz
		vmc_all.Rn_b[2][1] = -sind(-att_rt_use[ROLr]) * cosd(-att_rt_use[YAWr]) +
		                     cosd(-att_rt_use[ROLr]) * sind(-att_rt_use[PITr]) * sind(-att_rt_use[YAWr]);
		//-sx cz + cx sy sz

		vmc_all.Rn_b[0][2] = -sind(-att_rt_use[PITr]);
		//-sy
		vmc_all.Rn_b[1][2] = sind(-att_rt_use[ROLr]) * cosd(-att_rt_use[PITr]);
		// sx cy
		vmc_all.Rn_b[2][2] = cosd(-att_rt_use[ROLr]) * cosd(-att_rt_use[PITr]);
		// cx cy

		mat_trans(vmc_all.Rn_b, vmc_all.Rb_n);
		att_rt_use[ROLr] = 0;
		att_rt_use[YAWr] = 0;
		vmc_all.Rn_b_noroll[0][0] = cosd(-att_rt_use[PITr]) * cosd(-att_rt_use[YAWr]);
		vmc_all.Rn_b_noroll[1][0] = -cosd(-att_rt_use[ROLr]) * sind(-att_rt_use[YAWr]) +
		                            sind(-att_rt_use[PITr]) * sind(-att_rt_use[ROLr]) * cosd(-att_rt_use[YAWr]);
		vmc_all.Rn_b_noroll[2][0] = sind(-att_rt_use[ROLr]) * sind(-att_rt_use[YAWr]) +
		                            cosd(-att_rt_use[ROLr]) * sind(-att_rt_use[PITr]) * cosd(-att_rt_use[YAWr]);

		vmc_all.Rn_b_noroll[0][1] = cosd(-att_rt_use[PITr]) * sind(-att_rt_use[YAWr]);
		vmc_all.Rn_b_noroll[1][1] = cosd(-att_rt_use[ROLr]) * cosd(-att_rt_use[YAWr]) +
		                            sind(-att_rt_use[ROLr]) * sind(-att_rt_use[PITr]) * sind(-att_rt_use[YAWr]);
		vmc_all.Rn_b_noroll[2][1] = -sind(-att_rt_use[ROLr]) * cosd(-att_rt_use[YAWr]) +
		                            cosd(-att_rt_use[ROLr]) * sind(-att_rt_use[PITr]) * sind(-att_rt_use[YAWr]);

		vmc_all.Rn_b_noroll[0][2] = -sind(-att_rt_use[PITr]);
		vmc_all.Rn_b_noroll[1][2] = sind(-att_rt_use[ROLr]) * cosd(-att_rt_use[PITr]);
		vmc_all.Rn_b_noroll[2][2] = cosd(-att_rt_use[ROLr]) * cosd(-att_rt_use[PITr]);

		mat_trans(vmc_all.Rn_b_noroll, vmc_all.Rb_n_noroll);

#if VIR_MODEL
		vmc_all.att[YAWr] = nav.fake_yaw;
#endif
		vmc_all.att_rate[PITr] = mems.Gyro_deg_rt.x;
		vmc_all.att_rate[ROLr] = mems.Gyro_deg_rt.y;
		vmc_all.att_rate[YAWr] = mems.Gyro_deg_rt.z;
		DigitalLPF(acc_temp[0] * 9.8, &vmc_all.acc[Xr], FLT_ACC, T);
		DigitalLPF(-acc_temp[1] * 9.8, &vmc_all.acc[Yr], FLT_ACC, T);
		DigitalLPF(acc_temp[2] * 9.8, &vmc_all.acc[Zr], FLT_ACC, T);

		copy_imu_to_robotwb(T);
	}
}

void Duty_System() // система: RC, защиты, звук
{
	u8 i;
	static u16 cnt_1, cnt_2;
	static u8 cnt;
	static char state_ocu = 0;
	static char state_sdk = 0;
	float T;
	system_dt.system_task = T = leg_dt[7] = Get_Cycle_T(7);
	LEDRGB_STATE(0.05f);
	// внешние LED — индикация состояния
	LED_SCP(io_sel_scp_scl[0]);
	LED_SCL(io_sel_scp_scl[1]);

	if (vmc_all.param.cal_flag[0] && module.flash)
	{
		for (i = 0; i < 4; i++)
			vmc_all.param.ground_force[i][0] = 0.0f;
		mems.Gyro_CALIBRATE = 1;
		vmc_all.param.cal_flag[0] = 0;
	}
	else if (vmc_all.param.cal_flag[1] == 1 && module.flash)
	{
		mems.Gyro_CALIBRATE = 1;
		vmc_all.param.cal_flag[1] = 0;
	}
	else if (vmc_all.param.param_save == 1 && module.flash)
	{
		WRITE_PARM();
		vmc_all.param.param_save = 0;
	}
	else if (vmc_all.param.cal_flag[1] == 2 && module.flash)
	{
		vmc_all.param.cal_flag[1] = 0;
	}

	ocu.sbus_conncect = 0;
	if (o_cmd.lost_cnt++ > 125)
		o_cmd.connect = 0;
	if (ocu.loss_cnt++ > 2 / 0.05)
		ocu.connect = ocu.mode = 0;
	if (ocu_loss_cnt++ > 2 / 0.05)
		ocu_connect = 0;
	if (spi_master_loss_pi++ > 2 / 0.05)
	{
		spi_master_connect_pi = 0;
		spi_master_loss_pi_all++;
	}

	for (i = 0; i < 10; i++)
	{
		leg_motor.connect_motor[i] = motor_chassis[i].param.connect;
		if (motor_chassis[i].param.loss_cnt++ > 0.5 / 0.05)
			motor_chassis[i].param.connect = 0;
	}
	robot.spi_link = spi_master_connect_pi;
	robot.extcan_link = 0;
	robot.can1_link = 0;
	robot.can2_link = 0;
	for (int i = 0; i < 5; i++)
	{
		if (leg_motor.connect_motor[i] == 1)
			robot.can1_link++;
	}
	for (int i = 5; i < 10; i++)
	{
		if (leg_motor.connect_motor[i] == 1)
			robot.can2_link++;
	}

	if (imuo.cnt_loss++ > 0.5 / 0.02)
		imuo.connect = 0;

	static char beep_state = 0;
	static float beep_timer = 0;

#if defined(EN_BEEP)
	if (robotwb.beep_state != 0)
	{
	}
	else if (beep_state == 0 && spi_master_connect_pi)
	{
		beep_timer = 0;
		Reset_Beep_Task();
	}

	switch (beep_state)
	{
	case 0:
		Reset_Beep_Task();
		beep_timer += 0.05;
		if (beep_timer > 0.1)
		{
			beep_timer = 0;
			beep_state++;
		}
		break;
	case 1:
		if (robotwb.beep_state == BEEP_BLDC_ZERO_CAL)
		{
			beep_timer = 0;
			Reset_Beep_Task();
			beep_state = BEEP_BLDC_ZERO_CAL;
		}
		else if (robotwb.beep_state == BEEP_BLDC_ZERO_INIT)
		{
			beep_timer = 0;
			Reset_Beep_Task();
			beep_state = BEEP_BLDC_ZERO_INIT;
		}
		else if (robotwb.beep_state == BEEP_BLDC_GAIT_SWITCH)
		{
			beep_timer = 0;
			Reset_Beep_Task();
			beep_state = BEEP_BLDC_GAIT_SWITCH;
		}
		else if (robotwb.beep_state == BEEP_BLDC_RESET_ERR)
		{
			beep_timer = 0;
			Reset_Beep_Task();
			beep_state = BEEP_BLDC_RESET_ERR;
		}
		else
			Play_Music_Task(BEEP_BLDC_STATE, 0.05);
		break;
	case BEEP_BLDC_ZERO_CAL:
		if (Play_Music_Task(BEEP_BLDC_ZERO_CAL, 0.05))
		{
			beep_timer = 0;
			robotwb.beep_state = beep_state = 0;
			Reset_Beep_Task();
		}
		break;
	case BEEP_BLDC_ZERO_INIT:
		if (Play_Music_Task(BEEP_BLDC_ZERO_INIT, 0.05))
		{
			beep_timer = 0;
			robotwb.beep_state = beep_state = 0;
			Reset_Beep_Task();
		}
		break;
	case BEEP_BLDC_GAIT_SWITCH:
		if (Play_Music_Task(BEEP_BLDC_GAIT_SWITCH, 0.05))
		{
			beep_timer = 0;
			robotwb.beep_state = beep_state = 0;
			Reset_Beep_Task();
		}
		break;
	case BEEP_BLDC_RESET_ERR:
		if (Play_Music_Task(BEEP_BLDC_RESET_ERR, 0.05))
		{
			beep_timer = 0;
			robotwb.beep_state = beep_state = 0;
			Reset_Beep_Task();
		}
		break;
	case BEEP_BLDC_SPI_CONNECT:
		if (Play_Music_Task(BEEP_BLDC_SPI_CONNECT, 0.05))
		{
			beep_timer = 0;
			robotwb.beep_state = beep_state = 0;
			Reset_Beep_Task();
		}
		break;
	case BEEP_BLDC_SPI_CONNECT_THREAD_UP:
		if (Play_Music_Task(BEEP_BLDC_SPI_CONNECT_THREAD_UP, 0.05))
		{
			beep_timer = 0;
			robotwb.beep_state = beep_state = 0;
			Reset_Beep_Task();
		}
		break;
	}
#endif
}

int time_scale = 1;
void Duty_Loop() // тик 1 мс; обработчик должен укладываться в период
{
	int id = 0;
	static u8 mav_state;
	static u16 cnt[3];
	static int ext_send_flag = 0;
	static int cnt_1ms = 0;
	static int ip_get = 0;
	static float timer_ip = 0;
	if (loop.check_flag == 1)
	{
		loop_cnt = time_1ms;

		Uart6_PollCommandLine();

		if (!spi_master_connect_pi)
		{
			for (id = 0; id < 10; id++)
			{
				leg_motor.q_set[id] = leg_motor.q_now[id];
				leg_motor.qd_set[id] = 0;
				leg_motor.set_t[id] = 0;
				leg_motor.kp[id] = 0;
				leg_motor.kd[id] = 0;

				leg_motor.motor_en = 0;
			}
		}

		if (loop.cnt_2ms >= 2 * time_scale) // 500 Гц
		{
			loop.cnt_2ms = 0;
			Duty_Att_Fushion(); // IMU

			// // тест
			// if(motor_chassis[0].en_cmd==1) {
			// 	debug_z = fabsf(motor_chassis[9].q_now - motor_chassis[9].set_q);
			// 	if(debug_z > 0.5) {
			// 		leg_motor.kd[5]=1.0;
			// 		leg_motor.set_t[5]=0;
			// 	}
			// }
		}

		if (loop.cnt_5ms >= 5) // 200 Гц
		{
			loop.cnt_5ms = 0;
		}

		if (loop.cnt_10ms >= 10) // 100 Гц
		{
			loop.cnt_10ms = 0;
			Duty_Servo();
		}

		if (loop.cnt_20ms >= 20) // 50 Гц
		{
			loop.cnt_20ms = 0;
		}

		if (loop.cnt_50ms >= 50)
		{
			loop.cnt_50ms = 0;
			Duty_System();
		}

		if (loop.cnt_1s >= 1000)
		{
			loop.cnt_1s = 0;
			printf("Hello ROS\n");
			// can_rx_over[4]=0;
		}

		timer_ip += Get_Cycle_T(25);
		// if( timer_ip>2 )
		// {
		// 	timer_ip = 0;
		// 	if(spi_master_connect_pi==1&&robot.ip1!=0
		// 		&&!ip_get
		// 	){
		// 		ip_get=1;

		// 		IWDG_Init(4,25000);//100ms
		// 		IWDG_Init(4,250);//100ms
		//   }
		// }

		loop.check_flag = 0; // цикл за 1 мс обработан
	}
}
