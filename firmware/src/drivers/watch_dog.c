#include "watch_dog.h"
#include "stm32f4xx_iwdg.h"
void IWDG_Init(u8 prer,u16 rlr)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); //ʹ�ܶ�IWDG->PR IWDG->RLR��д
	
	IWDG_SetPrescaler(prer); //����IWDG��Ƶϵ��

	IWDG_SetReload(rlr);   //����IWDGװ��ֵ

	IWDG_ReloadCounter(); //reload
	
	IWDG_Enable();       //ʹ�ܿ��Ź�
	IWDG_Feed();
}

//ι�������Ź�
void IWDG_Feed(void)
{
	IWDG_ReloadCounter();//reload
}
