#include "dma.h"
#include "delay.h"

// Настройка потоков DMAx
// Режим передачи здесь фиксирован — при необходимости меняйте под задачу
// Память -> периферия, 8 бит, инкремент только у памяти
// DMA_Streamx: поток DMA1_Stream0~7 / DMA2_Stream0~7
// chx: канал DMA, см. DMA_Channel_0~7
// par: адрес периферии
// mar: адрес в памяти
// ndtr: число передаваемых элементов
void MYDMA_Config(DMA_Stream_TypeDef *DMA_Streamx, u32 chx, u32 par, u32 mar, u16 ndtr, u8 pri)
{

	DMA_InitTypeDef DMA_InitStructure;

	if ((u32)DMA_Streamx > (u32)DMA2) // DMA2 или DMA1
	{
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE); // тактирование DMA2
	}
	else
	{
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE); // тактирование DMA1
	}
	DMA_DeInit(DMA_Streamx);

	while (DMA_GetCmdStatus(DMA_Streamx) != DISABLE)
	{
	} // ждём, пока DMA можно настроить

	/* Конфигурация DMA Stream */
	DMA_InitStructure.DMA_Channel = chx;                                    // канал
	DMA_InitStructure.DMA_PeripheralBaseAddr = par;                         // адрес периферии
	DMA_InitStructure.DMA_Memory0BaseAddr = mar;                            // адрес памяти (Memory0)
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;                 // память -> периферия
	DMA_InitStructure.DMA_BufferSize = ndtr;                                // число передач
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;        // периферия без инкремента
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                 // память с инкрементом
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // периферия: байт
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;         // память: байт
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                           // обычный режим
	                                                                        /*
                                                                    #define DMA_Priority_Low                  ((uint32_t)0x00000000)
                                                                    #define DMA_Priority_Medium               ((uint32_t)0x00010000)
                                                                    #define DMA_Priority_High                 ((uint32_t)0x00020000)
                                                                    #define DMA_Priority_VeryHigh             ((uint32_t)0x00030000)*/
	switch (pri)
	{
	case 0:
		DMA_InitStructure.DMA_Priority = DMA_Priority_Low; // низкий приоритет
		break;
	case 1:
		DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; // средний
		break;
	case 2:
		DMA_InitStructure.DMA_Priority = DMA_Priority_High; // высокий
		break;
	case 3:
		DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh; // очень высокий
		break;
	}
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; // средний (перезапись после switch)
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;         // одиночный burst памяти
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; // одиночный burst периферии
	DMA_Init(DMA_Streamx, &DMA_InitStructure);                          // инициализация потока DMA
}
// Запуск одной передачи DMA
// DMA_Streamx: поток DMA1/DMA2
// ndtr: число элементов
void MYDMA_Enable(DMA_Stream_TypeDef *DMA_Streamx, u16 ndtr)
{

	DMA_Cmd(DMA_Streamx, DISABLE); // остановить DMA

	while (DMA_GetCmdStatus(DMA_Streamx) != DISABLE)
	{
	} // ждать отключения

	DMA_SetCurrDataCounter(DMA_Streamx, ndtr); // загрузить счётчик

	DMA_Cmd(DMA_Streamx, ENABLE); // запустить DMA
}
