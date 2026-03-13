#include "drivers/can.h"

/* SysTick handler is provided by CMSIS-FreeRTOS integration. */

void CAN1_RX0_IRQHandler(void)
{
	can1_irq_rx0_handler();
}

void CAN1_SCE_IRQHandler(void)
{
	can1_irq_sce_handler();
}

void CAN2_RX0_IRQHandler(void)
{
	can2_irq_rx0_handler();
}

void CAN2_SCE_IRQHandler(void)
{
	can2_irq_sce_handler();
}
