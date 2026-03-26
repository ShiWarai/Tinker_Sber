#include "delay.h"
#include "sys.h"
#include "time.h"
////////////////////////////////////////////////////////////////////////////////// 	 
// При использовании uCOS достаточно подключить заголовок ниже.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					// использование uCOS
#endif

static u8  fac_us=0;// коэффициент для задержки в мкс
static u16 fac_ms=0;// коэффициент для задержки в мс; в uCOS — мс на один тик ОС
__IO uint32_t TimingMillis = 0;

uint32_t millis(void)
{
	return TimingMillis;
}

#ifdef OS_CRITICAL_METHOD 	// если задано OS_CRITICAL_METHOD — используется uCOS II
// Обработчик прерывания SysTick (для uCOS)
void SysTick_Handler(void)
{				   
	  OSIntEnter();		// вход в прерывание
    OSTimeTick();       // служба времени uCOS
    OSIntExit();        // возможное переключение задач
}
#endif
			   
// Инициализация задержек
// В режиме uCOS настраивает тик ОС
// Тактирование SYSTICK: HCLK/8
// SYSCLK: частота ядра (МГц)
void delay_init(u8 SYSCLK)
{
#ifdef OS_CRITICAL_METHOD 	// если задано OS_CRITICAL_METHOD — используется uCOS II
	u32 reload;
#endif
 	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
	fac_us=SYSCLK/8;		// нужен всегда, с uCOS или без
	    
#ifdef OS_CRITICAL_METHOD 	// если задано OS_CRITICAL_METHOD — используется uCOS II
	reload=SYSCLK/8;		// число отсчётов счётчика в секунду (тыс.)
	reload*=1000000/OS_TICKS_PER_SEC;// период переполнения по OS_TICKS_PER_SEC
							// LOAD — 24 бита, макс. 16777216; при 168 МГц ~0,7989 с
	fac_ms=1000/OS_TICKS_PER_SEC;// минимальный шаг задержки в мс в uCOS
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;   	// прерывание SysTick
	SysTick->LOAD=reload; 	// период 1/OS_TICKS_PER_SEC с
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;   	// включить SysTick
#else
	fac_ms=(u16)fac_us*1000;// без uCOS: число тактов SysTick на 1 мс
#endif
}								    

#ifdef OS_CRITICAL_METHOD 	// если задано OS_CRITICAL_METHOD — используется uCOS II
// Задержка n мкс
// nus: длительность в микросекундах
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;	// значение LOAD
	ticks=nus*fac_us; 			// нужное число шагов счётчика
	tcnt=0;
	OSSchedLock();				// запрет переключения задач на время мкс-задержки
	told=SysTick->VAL;        	// начальное значение счётчика
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;// SysTick — убывающий счётчик
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;// выход по истечении времени
		}  
	};
	OSSchedUnlock();			// снова разрешить планировщик uCOS
}
// Задержка n мс
// nms: длительность в миллисекундах
void delay_ms(u16 nms)
{	
		if(OSRunning==OS_TRUE&&OSLockNesting==0)// ОС уже запущена
	{		  
		if(nms>=fac_ms)// дольше минимального тика uCOS
		{
   			OSTimeDly(nms/fac_ms);	// задержка через uCOS
		}
		nms%=fac_ms;				// остаток — обычной задержкой в мкс
	}
	delay_us((u32)(nms*1000));		// обычная задержка
}
#else  // без uCOS
// Задержка n мкс
// nus: длительность в микросекундах
// Внимание: nus не больше 798915 мкс
void delay_us(u32 nus)
{		
	if(nus>0)
		Delay_us(nus);
//	u32 temp;	    	 
//	SysTick->LOAD=nus*fac_us; // загрузка времени
//	SysTick->VAL=0x00;        // сброс счётчика
//	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          // запуск отсчёта
//	do
//	{
//		temp=SysTick->CTRL;
//	}
//	while((temp&0x01)&&!(temp&(1<<16)));// ожидание
//	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       // останов счётчика
//	SysTick->VAL =0X00;       // сброс
}
// Задержка n мс
// Допустимый диапазон nms
// SysTick->LOAD — 24 бита, максимум:
// nms<=0xffffff*8*1000/SYSCLK
// SYSCLK в Гц, nms в мс
// при 168 МГц: nms<=798 мс
void delay_xms(u16 nms)
{	 		  	  
	u32 temp;		   
	SysTick->LOAD=(u32)nms*fac_ms;// загрузка (LOAD — 24 бита)
	SysTick->VAL =0x00;           // сброс счётчика
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          // запуск отсчёта
	do
	{
		temp=SysTick->CTRL;
	}
	while((temp&0x01)&&!(temp&(1<<16)));// ожидание
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       // останов счётчика
	SysTick->VAL =0X00;       // сброс
} 
// Задержка n мс
// nms: 0~65535
void delay_ms(u16 nms)
{	 	 
//	u8 repeat=nms/540;	// 540 — запас при разгоне частоты (напр. 248 МГц: delay_xms ~541 мс)
//	u16 remain=nms%540;
//	while(repeat)
//	{
//		delay_xms(540);
//		repeat--;
//	}
//	if(remain)delay_xms(remain);
		Delay_ms(nms);
} 
#endif
		
