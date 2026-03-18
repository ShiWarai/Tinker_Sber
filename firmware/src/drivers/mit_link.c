#include "can.h"
#include "include.h"
#include "base_struct.h"
#include <math.h>
#include "can.h"
#include "gait_math.h"

float P_MIN_CAN_MIT[10] ={-12.5f,-12.5f,-12.5f};
float P_MAX_CAN_MIT[10]  ={12.5f,12.5f,12.5f};
float V_MIN_CAN_MIT[10] ={-38.2f,-38.2f,-38.2f};
float V_MAX_CAN_MIT[10] ={38.2f,38.2f,38.2f};
float KP_MIN_CAN_MIT[10] ={0.0f,0.0f,0.0f};
float KP_MAX_CAN_MIT[10] ={500.0f,500.0f,500.0f};
float KD_MIN_CAN_MIT[10] ={0.0f,0.0f,0.0f};
float KD_MAX_CAN_MIT[10] ={5.0f,5.0f,5.0f};
float T_MIN_CAN_MIT[10] ={-12.0f,-12.0f,-12.0f};
float T_MAX_CAN_MIT[10] ={12.0f,12.0f,12.0f};

float fmaxf_mit(float x, float y){
    /// Returns maximum of x, y ///
    return (((x)>(y))?(x):(y));
    }

float fminf_mit(float x, float y){
    /// Returns minimum of x, y ///
    return (((x)<(y))?(x):(y));
    }

int float_to_uint_mit(float x, float x_min, float x_max, int bits){
    /// Converts a float to an unsigned int, given range and number of bits ///
    float span = x_max - x_min;
    float offset = x_min;
		x=LIMIT(x,x_min,x_max);
    return (int) ((x-offset)*((float)((1<<bits)-1))/span);
    }

float uint_to_float_mit(int x_int, float x_min, float x_max, int bits){
    /// converts unsigned int to float, given range and number of bits ///
    float span = x_max - x_min;
    float offset = x_min;
    return ((float)x_int)*span/((float)((1<<bits)-1)) + offset;
    }

int float_to_uint(float x_float, float x_min, float x_max, int bits)
{
	/* Converts a float to an unsigned int, given range and number of bits */
	float span = x_max - x_min;
	float offset = x_min;
	return (int) ((x_float-offset)*((float)((1<<bits)-1))/span);
}

uint16_t float_to_uint_gim(float v,float v_min,float v_max,uint32_t width)
{
	float temp;
	int32_t utemp;
	temp=((v-v_min)/(v_max-v_min))*((float)width);
	utemp=(int32_t)temp;
	if(utemp<0)utemp=0;
	if(utemp>width)utemp=width;
	return utemp;
}
		
// включение/выключение MIT-режима мотора
u8 mit_motor_mode_en(char id, char en)
{
	CanTxMsg tx;
	u8       mbox;
	u16      wait = 0U;

	/* Локальный ID на шине (1..5), ID 5..9 уезжают на CAN2 как 1..5 */
	if (id < 5) {
		tx.StdId = (u32)(id + 1);
	} else {
		tx.StdId = (u32)(id + 1 - 5);
	}

	tx.ExtId = 0x00U;
	tx.IDE   = CAN_ID_STD;
	tx.RTR   = CAN_RTR_Data;
	tx.DLC   = 8U;

	tx.Data[0] = 0xFFU;
	tx.Data[1] = 0xFFU;
	tx.Data[2] = 0xFFU;
	tx.Data[3] = 0xFFU;
	tx.Data[4] = 0xFFU;
	tx.Data[5] = 0xFFU;
	tx.Data[6] = 0xFFU;
	tx.Data[7] = en ? 0xFCU : 0xFDU;

	if (id < 5) {
		mbox = CAN_Transmit(CAN1, &tx);
		while ((CAN_TransmitStatus(CAN1, mbox) == CAN_TxStatus_Failed) && (wait < 0x0FFFU)) {
			wait++;
		}
	} else {
		mbox = CAN_Transmit(CAN2, &tx);
		while ((CAN_TransmitStatus(CAN2, mbox) == CAN_TxStatus_Failed) && (wait < 0x0FFFU)) {
			wait++;
		}
	}

	if (wait >= 0x0FFFU) {
		return 1;
	}

	return 0;
}

// команда калибровки нуля позиции
u8 mit_set_pos_zero(char id)
{
	CanTxMsg tx;
	u8       mbox;
	u16      wait = 0U;

	if (id < 5) {
		tx.StdId = (u32)(id + 1);
	} else {
		tx.StdId = (u32)(id + 1 - 5);
	}

	tx.ExtId = 0x00U;
	tx.IDE   = CAN_ID_STD;
	tx.RTR   = CAN_RTR_Data;
	tx.DLC   = 8U;

	tx.Data[0] = 0xFFU;
	tx.Data[1] = 0xFFU;
	tx.Data[2] = 0xFFU;
	tx.Data[3] = 0xFFU;
	tx.Data[4] = 0xFFU;
	tx.Data[5] = 0xFFU;
	tx.Data[6] = 0xFFU;
	tx.Data[7] = 0xFEU;

	if (id < 5) {
		mbox = CAN_Transmit(CAN1, &tx);
		while ((CAN_TransmitStatus(CAN1, mbox) == CAN_TxStatus_Failed) && (wait < 0x0FFFU)) {
			wait++;
		}
	} else {
		mbox = CAN_Transmit(CAN2, &tx);
		while ((CAN_TransmitStatus(CAN2, mbox) == CAN_TxStatus_Failed) && (wait < 0x0FFFU)) {
			wait++;
		}
	}

	if (wait >= 0x0FFFU) {
		return 1;
	}

	return 0;
}


int qd_mid_f_mit=3;
void data_can_mit_anal(motor_measure_t *ptr, uint8_t buf_rx[8])
{
	float dt = Get_Cycle_T(ptr->param.id + 50);

	// Обновление статуса связи
	ptr->param.connect = 1;
	ptr->param.loss_cnt = 0;
	ptr->param.rx_dt = dt;

	// === Парсинг CAN-пакета ===
	uint16_t p_int = (buf_rx[1] << 8) | buf_rx[2];              // Позиция (16 бит)
	uint16_t v_int = (buf_rx[3] << 4) | (buf_rx[4] >> 4);       // Скорость (12 бит)
	uint16_t i_int = ((buf_rx[4] & 0xF) << 8) | buf_rx[5];      // Момент (12 бит)

	// === Позиция: в радианах (без перевода в градусы) ===
	float q_float = uint_to_float_mit(p_int, P_MIN_CAN_MIT[ptr->param.id], P_MAX_CAN_MIT[ptr->param.id], 16);

	// === Обработка позиции: нормализация в [-PI, PI] рад ===
	ptr->param.total_angle_out = q_float;
	ptr->param.cnt_rotate = (int)(ptr->param.total_angle_out / (2.0f * M_PI));
	ptr->param.total_angle_out_single = To_PI(fmod(ptr->param.total_angle_out, (double)(2.0f * M_PI)));

	// Используем всё равно старые значения
	ptr->q_now = q_float;
	ptr->q_now_flt = q_float;

	// === Скорость: рад/с ===
	float v_float = uint_to_float_mit(v_int, V_MIN_CAN_MIT[ptr->param.id], V_MAX_CAN_MIT[ptr->param.id], 12);
	ptr->qd_now = v_float;
	ptr->qd_now_flt = v_float;

	// === Момент: Н·м ===
	float t_float = uint_to_float_mit(i_int, -T_MAX_CAN_MIT[ptr->param.id], T_MAX_CAN_MIT[ptr->param.id], 12);
	ptr->t_now_flt = t_float;

	// Сохранение значений для следующей итерации
	ptr->param.q_now_reg = ptr->q_now;
	ptr->param.qd_now_reg = ptr->qd_now;
}

// float v_des_ff[10];
float k_spd_all = 5.0f;  // коэффициент для скоростного режима (новое значение)

// Отправка данных по CAN через протокол MIT
char data_can_mit_send(motor_measure_t *ptr)
{
	uint8_t  frame[8] = {0};
	float    set_q;

	if (ptr == NULL) {
		return 1;
	}

	/* Тестовый режим: синусоидальная команда по позиции вокруг смещения */
	if (ptr->cmd_mode == 2) {
		ptr->set_q = LIMIT(ptr->set_q_test + ptr->set_q_test_bias, -M_PI, M_PI);
	}

	set_q = ptr->set_q;

	ptr->param.set_q = set_q;  /* сохраняем для отладки/аналитики */

	/* Желаемая скорость и момент — без инверсии по q_flag */
	const float set_dq = ptr->set_qd;
	const float set_t  = ptr->set_t;

	/* Ограничение команд в пределах таблиц MIT для конкретного мотора */
	const uint8_t id = (uint8_t)ptr->param.id;

	const float p_des = fminf_mit(fmaxf_mit(P_MIN_CAN_MIT[id], set_q),  P_MAX_CAN_MIT[id]);
	const float v_des = fminf_mit(fmaxf_mit(V_MIN_CAN_MIT[id], set_dq), V_MAX_CAN_MIT[id]);
	float       kp    = fminf_mit(fmaxf_mit(KP_MIN_CAN_MIT[id], ptr->stiff * ptr->kp), KP_MAX_CAN_MIT[id]);
	float       kd    = fminf_mit(fmaxf_mit(KD_MIN_CAN_MIT[id], ptr->stiff * ptr->kd), KD_MAX_CAN_MIT[id]);
	float       t_ff  = fminf_mit(
	                      fmaxf_mit(T_MIN_CAN_MIT[id], LIMIT(set_t, -ptr->max_t, ptr->max_t)),
	                      T_MAX_CAN_MIT[id]);

	// /* Режим управления скоростью (control_mode == 1): особая обработка kp/kd/v_des */
	// if (ptr->param.control_mode == 1) {
	// 	kp = 0.0f;
	// 	kd = LIMIT(kd, KD_MIN_CAN_MIT[id], KD_MAX_CAN_MIT[id]);
	// 	v_des_ff[id] = LIMIT(set_dq * k_spd_all, V_MIN_CAN_MIT[id], V_MAX_CAN_MIT[id]);
	// }

	/* Квантизация в MIT-формат (p, v, kp, kd, t) */
	const uint16_t p_int  = float_to_uint_mit(p_des, P_MIN_CAN_MIT[id], P_MAX_CAN_MIT[id], 16);
	const uint16_t v_int  = float_to_uint_mit(v_des, V_MIN_CAN_MIT[id], V_MAX_CAN_MIT[id], 12);
	const uint16_t kp_int = float_to_uint_mit(kp,    KP_MIN_CAN_MIT[id], KP_MAX_CAN_MIT[id], 12);
	const uint16_t kd_int = float_to_uint_mit(kd,    KD_MIN_CAN_MIT[id], KD_MAX_CAN_MIT[id], 12);
	const uint16_t t_int  = float_to_uint_mit(t_ff,  T_MIN_CAN_MIT[id],  T_MAX_CAN_MIT[id],  12);

	frame[0] = (uint8_t)(p_int >> 8);
	frame[1] = (uint8_t)(p_int & 0xFFU);
	frame[2] = (uint8_t)(v_int >> 4);
	frame[3] = (uint8_t)(((v_int & 0x0FU) << 4) | (kp_int >> 8));
	frame[4] = (uint8_t)(kp_int & 0xFFU);
	frame[5] = (uint8_t)(kd_int >> 4);
	frame[6] = (uint8_t)(((kd_int & 0x0FU) << 4) | (t_int >> 8));
	frame[7] = (uint8_t)(t_int & 0xFFU);

	/* Формируем и отправляем CAN-кадр (StdId 0..4 на CAN1, 5..9 на CAN2) */
	CanTxMsg tx;
	uint8_t  mbox;
	uint16_t wait = 0U;

	if (id < 5U) {
		tx.StdId = 0x00U + (uint32_t)id + 1U;
	} else {
		tx.StdId = 0x00U + (uint32_t)id + 1U - 5U;
	}

	tx.ExtId = 0x00U;
	tx.IDE   = CAN_ID_STD;
	tx.RTR   = CAN_RTR_Data;
	tx.DLC   = 8U;

	for (uint32_t j = 0; j < 8U; j++) {
		tx.Data[j] = frame[j];
	}

	if (id < (uint8_t)5U) {
		mbox = CAN_Transmit(CAN1, &tx);
		while ((CAN_TransmitStatus(CAN1, mbox) == CAN_TxStatus_Failed) && (wait < 0x0FFFU)) {
			wait++;
		}
	} else {
		mbox = CAN_Transmit(CAN2, &tx);
		while ((CAN_TransmitStatus(CAN2, mbox) == CAN_TxStatus_Failed) && (wait < 0x0FFFU)) {
			wait++;
		}
	}

	if (wait >= 0x0FFFU) {
		return 1;
	}

	return 0;
}

// char data_can_sample_only(motor_measure_t *ptr){//���ݲɼ�
// 	u8 canbuft1[8];
// /// limit data to be within bounds ///  
// 	int q_flag=1;
// 	if(ptr->param.q_flag)
// 		q_flag=1;
// 	else
// 		q_flag=-1; 
// 	float set_q=q_flag*To_PI(ptr->set_q-To_PI(ptr->param.q_reset_angle));
// 	ptr->param.set_q=set_q;
 
// 	float p_des = fminf_mit(fmaxf_mit(P_MIN_CAN_MIT[ptr->param.id], set_q), P_MAX_CAN_MIT[ptr->param.id]);        
// 	float v_des = fminf_mit(fmaxf_mit(V_MIN_CAN_MIT[ptr->param.id], 0.0f), V_MAX_CAN_MIT[ptr->param.id]);   
// 	float kp = fminf_mit(fmaxf_mit(KP_MIN_CAN_MIT[ptr->param.id],  0), KP_MAX_CAN_MIT[ptr->param.id]);   
// 	float kd = fminf_mit(fmaxf_mit(KD_MIN_CAN_MIT[ptr->param.id],  0), KD_MAX_CAN_MIT[ptr->param.id]); 
// 	float t_ff = fminf_mit(fmaxf_mit(T_MIN_CAN_MIT[ptr->param.id], 0), T_MAX_CAN_MIT[ptr->param.id]); 
// 	/// convert floats to unsigned ints ///    
// 	uint16_t p_int = float_to_uint_mit(p_des, P_MIN_CAN_MIT[ptr->param.id], P_MAX_CAN_MIT[ptr->param.id], 16);      
// 	uint16_t v_int = float_to_uint_mit(v_des, V_MIN_CAN_MIT[ptr->param.id], V_MAX_CAN_MIT[ptr->param.id], 12);    
// 	uint16_t kp_int = float_to_uint_mit(kp, KP_MIN_CAN_MIT[ptr->param.id], KP_MAX_CAN_MIT[ptr->param.id], 12);    
// 	uint16_t kd_int = float_to_uint_mit(kd, KD_MIN_CAN_MIT[ptr->param.id], KD_MAX_CAN_MIT[ptr->param.id], 12);    
// 	uint16_t t_int = float_to_uint_mit(t_ff, T_MIN_CAN_MIT[ptr->param.id], T_MAX_CAN_MIT[ptr->param.id], 12);  

// 	canbuft1[0] = p_int>>8;      
// 	canbuft1[1] = p_int&0xFF;  
// 	canbuft1[2] = v_int>>4;      
// 	canbuft1[3] = ((v_int&0xF)<<4)|(kp_int>>8);   
// 	canbuft1[4] = kp_int&0xFF;  
// 	canbuft1[5] = kd_int>>4;    
// 	canbuft1[6] = ((kd_int&0xF)<<4)|(t_int>>8);   
// 	canbuft1[7] = t_int&0xff;
	
// 	if(ptr->param.id >= 0x2) {
// 		q_flag = 1;
// 		printf("Test!");
// 	}
	
// 	u8 mbox;
//   u16 i=0;
// 	CanTxMsg TxMessage1;
// 	if(ptr->param.id<5)
// 		TxMessage1.StdId=0x00+ptr->param.id+1;	 // ��׼��ʶ��Ϊ0
// 	else
// 		TxMessage1.StdId=0x00+ptr->param.id+1-5;
	
//   TxMessage1.ExtId=0x00;//0x200;	 // ������չ��ʾ����29λ��
//   TxMessage1.IDE=0;		  // ʹ����չ��ʶ��
//   TxMessage1.RTR=0;		  // ��Ϣ����Ϊ����֡��һ֡8λ
//   TxMessage1.DLC=8;							 // ������֡��Ϣ
// 	TxMessage1.Data[0] = canbuft1[0];//300??
// 	TxMessage1.Data[1] = canbuft1[1];
// 	TxMessage1.Data[2] = canbuft1[2];
// 	TxMessage1.Data[3] = canbuft1[3];
// 	TxMessage1.Data[4] = canbuft1[4];
// 	TxMessage1.Data[5] = canbuft1[5];
// 	TxMessage1.Data[6] = canbuft1[6];
// 	TxMessage1.Data[7] = canbuft1[7];
	
// 	if(ptr->param.id<5){
// 		mbox= CAN_Transmit(CAN1, &TxMessage1);   
// 		i=0;
// 		while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�ȴ����ͽ���
// 	}
// 	else{
// 		mbox= CAN_Transmit(CAN2, &TxMessage1);   
// 		i=0;
// 		while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�ȴ����ͽ���
// 	}
//   if(i>=0XFFF)return 1;
//   return 0;		
// }

char en_mit_out=0;
int mit_delay=200;//300
int mit_connect_cnt=0;

static void mit_motors_init(void)
{
	char i = 0;

	motor_chassis[0].motor.type=DM_6006; motor_chassis[0].param.q_flag=0;
	motor_chassis[1].motor.type=DM_8006; motor_chassis[1].param.q_flag=1;
	motor_chassis[2].motor.type=DM_8006; motor_chassis[2].param.q_flag=1;
	motor_chassis[3].motor.type=DM_8006; motor_chassis[3].param.q_flag=0;
	motor_chassis[4].motor.type=DM_6006; motor_chassis[4].param.q_flag=1;
	
	motor_chassis[5].motor.type=DM_6006; motor_chassis[5].param.q_flag=0;
	motor_chassis[6].motor.type=DM_8006; motor_chassis[6].param.q_flag=1;
	motor_chassis[7].motor.type=DM_8006; motor_chassis[7].param.q_flag=0;
	motor_chassis[8].motor.type=DM_8006; motor_chassis[8].param.q_flag=1;
	motor_chassis[9].motor.type=DM_6006; motor_chassis[9].param.q_flag=0;
	
	for(i=0;i<10;i++)
		motor_chassis[i].param.control_mode=0;//POS
	
	for(i=0;i<10;i++){
		motor_chassis[i].param.id=i;

		switch(motor_chassis[i].motor.type){
		case DM_6006:
			P_MIN_CAN_MIT[i] =-12.5f;
			P_MAX_CAN_MIT[i]  =12.5f;
			V_MIN_CAN_MIT[i] =-45.0f;
			V_MAX_CAN_MIT[i] = 45.0f;
			KP_MIN_CAN_MIT[i] =0.0f;
			KP_MAX_CAN_MIT[i] =500.0f;
			KD_MIN_CAN_MIT[i] =0.0f;
			KD_MAX_CAN_MIT[i] =5.0f;

			T_MIN_CAN_MIT[i] =-12.0f;
			T_MAX_CAN_MIT[i] = 12.0f;
			motor_chassis[i].motor.anal_type=M_MIT;
			break;
		case DM_8006:
			P_MIN_CAN_MIT[i] =-12.5f;
			P_MAX_CAN_MIT[i]  =12.5f;
			V_MIN_CAN_MIT[i] =-45.0f;
			V_MAX_CAN_MIT[i] = 45.0f;
			KP_MIN_CAN_MIT[i] =0.0f;
			KP_MAX_CAN_MIT[i] =500.0f;
			KD_MIN_CAN_MIT[i] =0.0f;
			KD_MAX_CAN_MIT[i] =5.0f;

			T_MIN_CAN_MIT[i] =-20.0f;
			T_MAX_CAN_MIT[i] = 20.0f;
			motor_chassis[i].motor.anal_type=M_MIT;
			break;
		default:		
			P_MIN_CAN_MIT[i] =-12.5f;
			P_MAX_CAN_MIT[i]  =12.5f;
			V_MIN_CAN_MIT[i] =-30.0f;
			V_MAX_CAN_MIT[i] = 30.0f;
			KP_MIN_CAN_MIT[i] =0.0f;
			KP_MAX_CAN_MIT[i] =500.0f;
			KD_MIN_CAN_MIT[i] =0.0f;
			KD_MAX_CAN_MIT[i] =5.0f;

			T_MIN_CAN_MIT[i] =-10.0f;
			T_MAX_CAN_MIT[i] = 10.0f;
			motor_chassis[i].motor.anal_type=M_MIT;
			break;
		}
	}
}

void mit_bldc_thread(char en_all,float dt)
{
	char i=0;
	static float auto_off_t=99;
	static float timer_sin=0;
	static int reg_cmd_mode;
	static int motors_inited = 0;

	if (!motors_inited) {
		mit_motors_init();
		motors_inited = 1;
	}

	// Отправка команд или опрос
	for(i=0;i<10;i++){
		if(en_mit_out==2)
		{
			data_can_mit_send(&motor_chassis[i]);
			delay_us(mit_delay);
		}
	}

	// state-машина обнуления
	for(i=0;i<10;i++){
		if((motor_chassis[i].reset_q==1||motor_chassis[i].cal_div==1)&&motor_chassis[i].reset_q_lock==0){
			mit_set_pos_zero(i);
			delay_us(20000U);

			motor_chassis[i].reset_q_lock=1;
			motor_chassis[i].reset_q_cnt=0;
			motor_chassis[i].reset_q=0;
		}
		if(motor_chassis[i].reset_q_lock==1)
		{
			mit_set_pos_zero(i);
			delay_us(20000U);

			motor_chassis[i].reset_q_cnt++;

			if(motor_chassis[i].reset_q_cnt>3)
			{
				motor_chassis[i].reset_q_cnt=0;
				motor_chassis[i].reset_q_lock=2;
			}
		}
		if(motor_chassis[i].reset_q_lock==2)
		{
			motor_chassis[i].reset_q_delay_timer+=dt;
			if(motor_chassis[i].reset_q_delay_timer>3.5)
			{
				motor_chassis[i].reset_q_delay_timer=0;
				motor_chassis[i].reset_q_lock=0;
				motor_chassis[i].reset_q=0;
				motor_chassis[i].reset_q_cnt=0;

				motor_chassis[i].param.q_reset_angle = 0.0f; //to right set_zero_pos
			}
		}
	}

	// state-машина включение отключения
	switch(en_mit_out)
	{
		case 0: // Отключаем моторы
			auto_off_t+=dt;

			if(auto_off_t>0.5) // Иногда повторяем команду
			{
				auto_off_t=0;
				for(i=0;i<10;i++){
					mit_motor_mode_en(i,0);
					delay_us(mit_delay);
				}
			} else if(en_all)
			{
				for(i=0;i<10;i++){
					mit_motor_mode_en(i,1);
					delay_us(mit_delay);
				}
				en_mit_out++;
				auto_off_t=0;
			}

			break;
		case 1: // Включаем моторы
			auto_off_t += dt;

			if(auto_off_t > 0.5) // Иногда повторяем команду
			{
				auto_off_t=0;
				en_mit_out++;
			}

			for(i=0;i<10;i++) {
				mit_motor_mode_en(i,1);
				delay_us(mit_delay);
			}

			en_mit_out++;

			break;
		case 2: // В режиме MIT
			if(!en_all) // Ждём отмены включения моторов
			{
				for(i=0;i<10;i++){
					mit_motor_mode_en(i,0);
					delay_us(mit_delay);
				}
				en_mit_out=0;
			}
			break;
	}
}