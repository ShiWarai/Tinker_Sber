#include "sys.h"

/* Arm Compiler 6 (armclang) uses GNU-style inline assembly.
   These replace the legacy ARM Compiler 5 __asm blocks. */

void WFI_SET(void)
{
	__asm volatile("wfi");
}

void INTX_DISABLE(void)
{
	__asm volatile("cpsid i" : : : "memory");
}

void INTX_ENABLE(void)
{
	__asm volatile("cpsie i" : : : "memory");
}

void MSR_MSP(u32 addr)
{
	__asm volatile("msr msp, %0" : : "r"(addr) : "memory");
}
