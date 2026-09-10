#ifndef __DMA_H
#define __DMA_H
#include "sys.h"
void MYDMA_Config(DMA_Stream_TypeDef *DMA_Streamx, u32 chx, u32 par, u32 mar, u16 ndtr, u8 pri); // настройка DMAx_CHx
void MYDMA_Enable(DMA_Stream_TypeDef *DMA_Streamx, u16 ndtr);                                    // одна передача DMA
#endif
