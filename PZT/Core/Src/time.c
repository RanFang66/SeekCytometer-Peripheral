#include "time.h"

static SoftTimer softTimers[SOFTTIMERMAX];

SoftTimer* setSoftTimer(uint16_t nms,void (*listener)(void))
{
	uint8_t i;
	__disable_irq();
	 for(i = 0; i < SOFTTIMERMAX; i++)
	{
		if(DISABLE == softTimers[i].bRun)
		{
			softTimers[i].count = softTimers[i].reLoadCount=nms;
			softTimers[i].fnListener = listener;
			softTimers[i].bRun = ENABLE;
			__enable_irq();
			return &softTimers[i];
		}
	}
	__enable_irq();
	return 0;
}

void stopSoftTimer(SoftTimer* stimer)
{
	if(stimer	==	0)	return;
	__disable_irq();
	stimer->bRun = DISABLE;
	stimer->fnListener = 0;
	stimer->count = stimer->reLoadCount=0;
	__enable_irq();
}

void bsp_delayus(uint16_t us)
{
	uint32_t temp = SysTick->VAL;
	uint32_t usticks = us * BSP_TIME_USTICKS;
	if (temp >= usticks) {
		usticks = temp - usticks;
		while(SysTick->VAL > usticks && SysTick->VAL < temp);
	} else {
		usticks = BSP_TIME_MSTICKS + temp - usticks;
		while(SysTick->VAL > usticks || SysTick->VAL < temp);
	}
}

void Time_Tick(void)
{
	uint8_t i;
	for(i=0; i < SOFTTIMERMAX; i++)
	{
		if(softTimers[i].bRun)
		{
			softTimers[i].count--;
			if(softTimers[i].count == 0)
			{
				softTimers[i].fnListener();
				softTimers[i].count = softTimers[i].reLoadCount;
			}
		}
	}
}


