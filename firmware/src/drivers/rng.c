#include "rng.h"
#include "stm32f4xx_rng.h"
#include "time.h"
//��ʼ��RNG
u8 RNG_Init(void)
{
	u16 retry=0; 
	
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);//����RNGʱ��,����PLL48CLK
	
	RNG_Cmd(ENABLE);	//ʹ��RNG
	
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)==RESET&&retry<10000)	//�ȴ����������
	{
		retry++;
		Delay_us(100);
	}
	if(retry>=10000)return 1;//���������������������
	return 0;
}
//�õ������
//����ֵ:��ȡ���������
u32 RNG_Get_RandomNum(void)
{	 
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)==RESET);	//�ȴ����������  
	return RNG_GetRandomNumber();	
}

//����[min,max]��Χ�������
int RNG_Get_RandomRange(int min,int max)
{ 
   return RNG_Get_RandomNum()%(max-min+1) +min;
}



