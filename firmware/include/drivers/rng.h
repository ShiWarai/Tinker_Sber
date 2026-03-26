#ifndef __RNG_H
#define __RNG_H	 
#include "sys.h" 
	
u8  RNG_Init(void);			// инициализация RNG
u32 RNG_Get_RandomNum(void);// получить случайное число
int RNG_Get_RandomRange(int min,int max);// случайное число в [min,max]
#endif
