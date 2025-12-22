#ifndef __BSP_TIMEBASE_H
#define __BSP_TIMEBASE_H
#include "main.h"

#define SOFTTIMERMAX	4
#define BSP_TIME_USTICKS 168
#define BSP_TIME_MSTICKS 168000
typedef struct
{
	uint16_t count;
	uint16_t reLoadCount;
	FunctionalState bRun;
	void (*fnListener)(void);
}SoftTimer;

#define bsp_time_less(time, var) (HAL_GetTick() - (time) < (var))
#define bsp_time_more(time, var) (HAL_GetTick() - (time) > (var))
#define bsp_time_clear(time)	((time) = HAL_GetTick())
#define bsp_time_get(time) (HAL_GetTick() - (time))

void bsp_delayus(uint16_t us);
SoftTimer* setSoftTimer(uint16_t nms,void (*listener)(void));
void stopSoftTimer(SoftTimer* stimer);

void Time_Tick(void);
#endif
