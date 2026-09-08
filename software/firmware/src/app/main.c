#include "include.h"
#include "init.h"
#include "scheduler.h"

u8 Init_Finish = 0;
int main(void)
{
	Init_Finish = All_Init();

	while (1)
	{
		Duty_Loop();
	}
}
