
#include "include.h"
#include "flash.h"
#include "mems.h"
#include "flash_w25.h"
#include "icm20602.h"
#include "gait_math.h"
#include "can.h"
int flash_cnt = 0;
float isnan_checkf(float in)
{
	if (isnan(in))
		return 0;
	else
		return in;
}

char isnan_checkc(char in)
{
	if (isnan(in))
		return 0;
	else
		return in;
}

int isnan_checki(int in)
{
	if (isnan(in))
		return 0;
	else
		return in;
}
//????
static void setDataIntf(char *FLASH_Buffer, int i)
{
	*(FLASH_Buffer + flash_cnt++) = ((i << 24) >> 24);
	*(FLASH_Buffer + flash_cnt++) = ((i << 16) >> 24);
	*(FLASH_Buffer + flash_cnt++) = ((i << 8) >> 24);
	*(FLASH_Buffer + flash_cnt++) = (i >> 24);
}

static void setDataFloatf(char *FLASH_Buffer, float f)
{
	int i = *(int *)&f;
	*(FLASH_Buffer + flash_cnt++) = ((i << 24) >> 24);
	*(FLASH_Buffer + flash_cnt++) = ((i << 16) >> 24);
	*(FLASH_Buffer + flash_cnt++) = ((i << 8) >> 24);
	*(FLASH_Buffer + flash_cnt++) = (i >> 24);
}
//????
static float floatFromDataf(unsigned char *data, int *anal_cnt)
{
	int i = 0x00;
	float out = 0;
	i |= (*(data + *anal_cnt + 3) << 24);
	i |= (*(data + *anal_cnt + 2) << 16);
	i |= (*(data + *anal_cnt + 1) << 8);
	i |= (*(data + *anal_cnt + 0));

	*anal_cnt += 4;

	out = isnan_checkf(*(float *)&i);
	return out;
}

static char charFromDataf(unsigned char *data, int *anal_cnt)
{
	char out = 0;
	int temp = *anal_cnt;
	*anal_cnt += 1;
	out = *(data + temp);
	return isnan_checkc(out);
}

static int intFromDataf(unsigned char *data, int *anal_cnt)
{
	int i = 0x00;
	i |= (*(data + *anal_cnt + 3) << 24);
	i |= (*(data + *anal_cnt + 2) << 16);
	i |= (*(data + *anal_cnt + 1) << 8);
	i |= (*(data + *anal_cnt + 0));
	*anal_cnt += 4;
	return isnan_checki(i);
}

//-----------------------------------------??????
#define SIZE_PARAM 50 * 10
u32 FLASH_SIZE = 16 * 1024 * 1024; // FLASH ????16???

u8 need_init_mems = 0; // mems flash error
u16 SBUS_MIN = 868;
u16 SBUS_MAX = 2180;
u16 SBUS_MID = 1524;
u16 SBUS_MIN_A = 644;  // 954;
u16 SBUS_MAX_A = 2484; // 2108;
u16 SBUS_MID_A = 1524;
char flash_rd_end = 0;
void READ_PARM(void)
{
	u8 FLASH_Buffer[SIZE_PARAM] = {0};
	u8 need_init = 0;
	int i = 0, j = 0, temp = 0;
	int anal_cnt = 0;
	module.flash_lock = 1;
	W25QXX_Read(FLASH_Buffer, FLASH_SIZE - (SIZE_PARAM + 10), SIZE_PARAM);
	module.flash_lock = 0;
	mems.Gyro_Offset.x = intFromDataf(FLASH_Buffer, &anal_cnt);
	mems.Gyro_Offset.y = intFromDataf(FLASH_Buffer, &anal_cnt);
	mems.Gyro_Offset.z = intFromDataf(FLASH_Buffer, &anal_cnt);

	mems.Acc_Offset.x = intFromDataf(FLASH_Buffer, &anal_cnt);
	mems.Acc_Offset.y = intFromDataf(FLASH_Buffer, &anal_cnt);
	mems.Acc_Offset.z = intFromDataf(FLASH_Buffer, &anal_cnt);

	mems.Mag_Offset.x = intFromDataf(FLASH_Buffer, &anal_cnt);
	mems.Mag_Offset.y = intFromDataf(FLASH_Buffer, &anal_cnt);
	mems.Mag_Offset.z = intFromDataf(FLASH_Buffer, &anal_cnt);

	mems.Mag_Gain.x = floatFromDataf(FLASH_Buffer, &anal_cnt);
	mems.Mag_Gain.y = floatFromDataf(FLASH_Buffer, &anal_cnt);
	mems.Mag_Gain.z = floatFromDataf(FLASH_Buffer, &anal_cnt);

	if (fabs(mems.Mag_Gain.x) < 10 && mems.Mag_Gain.x != -1 && fabs(mems.Mag_Offset.x) < 666 &&
	    mems.Mag_Offset.x != -1 && mems.Mag_Gain.x != 0 && mems.Mag_Gain.y != 0 && mems.Mag_Gain.z != 0)
		mems.Mag_Have_Param = 1;

	mems.Mag_Offseto.x = intFromDataf(FLASH_Buffer, &anal_cnt);
	mems.Mag_Offseto.y = intFromDataf(FLASH_Buffer, &anal_cnt);
	mems.Mag_Offseto.z = intFromDataf(FLASH_Buffer, &anal_cnt);

	mems.Mag_Gaino.x = floatFromDataf(FLASH_Buffer, &anal_cnt);
	mems.Mag_Gaino.y = floatFromDataf(FLASH_Buffer, &anal_cnt);
	mems.Mag_Gaino.z = floatFromDataf(FLASH_Buffer, &anal_cnt);

	vmc_all.tar_att_bias[PITr] = floatFromDataf(FLASH_Buffer, &anal_cnt);
	vmc_all.tar_att_bias[ROLr] = floatFromDataf(FLASH_Buffer, &anal_cnt);

	temp = intFromDataf(FLASH_Buffer, &anal_cnt);
	vmc_all.your_key[0] = temp / 10000;
	vmc_all.your_key[1] = (temp - vmc_all.your_key[0] * 10000) / 100;
	vmc_all.your_key[2] = (temp - vmc_all.your_key[0] * 10000 - vmc_all.your_key[1] * 100);
	//------------
	for (i = 0; i < 10; i++)
	{
		motor_chassis[i].param.q_reset_angle = floatFromDataf(FLASH_Buffer, &anal_cnt);
		/* legacy: в flash могло быть сохранено 180 (градусы) — приводим к рад */
		if (motor_chassis[i].param.q_reset_angle > 10.0f)
			motor_chassis[i].param.q_reset_angle = M_PI;
		motor_chassis[i].param.t_inv_flag_cmd = intFromDataf(FLASH_Buffer, &anal_cnt);
		motor_chassis[i].param.t_inv_flag_measure = intFromDataf(FLASH_Buffer, &anal_cnt);
		motor_chassis[i].param.q_flag = intFromDataf(FLASH_Buffer, &anal_cnt);
		motor_chassis[i].motor.type = intFromDataf(FLASH_Buffer, &anal_cnt);
		motor_chassis[i].param.control_mode = intFromDataf(FLASH_Buffer, &anal_cnt);
	}

	flash_rd_end = charFromDataf(FLASH_Buffer, &anal_cnt);

	//??????
	for (i = 0; i < 10; i++)
	{
		if (motor_chassis[i].motor.type > 30)
		{
			motor_chassis[i].param.t_inv_flag_measure = 1;
			motor_chassis[i].param.t_inv_flag_cmd = 1;
			motor_chassis[i].param.q_flag = 1;
			motor_chassis[i].motor.type = DM_J4310;
			WRITE_PARM();
		}
	}

	if (module.hml_imu_o && mems.Mag_Have_Param)
		mems.Mag_Have_Param = 0;
	if (fabs(mems.Mag_Gaino.x) < 10 && mems.Mag_Gaino.x != -1 && fabs(mems.Mag_Offseto.x) < 666 &&
	    mems.Mag_Offseto.x != -1 && mems.Mag_Gaino.x != 0 && mems.Mag_Gaino.y != 0 && mems.Mag_Gaino.z != 0)
		mems.Mag_Have_Param = 1;

	if (SBUS_MIN == 65535)
	{
		SBUS_MIN = 860;
		SBUS_MID = 1524;
		SBUS_MAX = 2180;

		SBUS_MIN_A = 644;
		SBUS_MID_A = 1524;
		SBUS_MAX_A = 2484;
	}
}

void WRITE_PARM(void)
{

	int16_t _temp;
	u8 cnt = 0, i;
	char FLASH_Buffer[SIZE_PARAM] = {0};
	flash_cnt = 0;
	setDataIntf(FLASH_Buffer, mems.Gyro_Offset.x);
	setDataIntf(FLASH_Buffer, mems.Gyro_Offset.y);
	setDataIntf(FLASH_Buffer, mems.Gyro_Offset.z);
	setDataIntf(FLASH_Buffer, mems.Acc_Offset.x);
	setDataIntf(FLASH_Buffer, mems.Acc_Offset.y);
	setDataIntf(FLASH_Buffer, mems.Acc_Offset.z);
	setDataIntf(FLASH_Buffer, mems.Mag_Offset.x);
	setDataIntf(FLASH_Buffer, mems.Mag_Offset.y);
	setDataIntf(FLASH_Buffer, mems.Mag_Offset.z);
	setDataFloatf(FLASH_Buffer, mems.Mag_Gain.x);
	setDataFloatf(FLASH_Buffer, mems.Mag_Gain.y);
	setDataFloatf(FLASH_Buffer, mems.Mag_Gain.z);
	setDataIntf(FLASH_Buffer, mems.Mag_Offseto.x);
	setDataIntf(FLASH_Buffer, mems.Mag_Offseto.y);
	setDataIntf(FLASH_Buffer, mems.Mag_Offseto.z);
	setDataFloatf(FLASH_Buffer, mems.Mag_Gaino.x);
	setDataFloatf(FLASH_Buffer, mems.Mag_Gaino.y);
	setDataFloatf(FLASH_Buffer, mems.Mag_Gaino.z);

	setDataFloatf(FLASH_Buffer, vmc_all.tar_att_bias[PITr]);
	setDataFloatf(FLASH_Buffer, vmc_all.tar_att_bias[ROLr]);
	setDataIntf(FLASH_Buffer, vmc_all.your_key[0] * 10000 + vmc_all.your_key[1] * 100 + vmc_all.your_key[2]);

	//--------
	for (i = 0; i < 10; i++)
	{
		setDataFloatf(FLASH_Buffer, motor_chassis[i].param.q_reset_angle);
		setDataIntf(FLASH_Buffer, motor_chassis[i].param.t_inv_flag_cmd);
		setDataIntf(FLASH_Buffer, motor_chassis[i].param.t_inv_flag_measure);
		setDataIntf(FLASH_Buffer, motor_chassis[i].param.q_flag);
		setDataIntf(FLASH_Buffer, motor_chassis[i].motor.type);
		setDataIntf(FLASH_Buffer, motor_chassis[i].param.control_mode);
	}
	FLASH_Buffer[flash_cnt++] = 99;

	//????
	module.flash_lock = 1;
	W25QXX_Write((u8 *)FLASH_Buffer, FLASH_SIZE - (SIZE_PARAM + 10), SIZE_PARAM);
	module.flash_lock = 0;
}