#include "dcdc_power_ctrl.h"



void dcdc_set_power(uint16_t mv) {
	uint16_t vin;
	if (mv > 24000) {
		// (Vout - 1.6V)/649k = 1.6V/ 30.1K + (1.6V - Vin) / 90.9K
		// Vin / 90.9 + (Vout - 1.6) / 649 = 1.6/30.1 + 1.6 / 90.9
		// Vin / 90.9 = 1.6/30.1 + 1.6 / 90.9 - (Vout - 1.6) / 649
		// Vin = 1.6 * 90.9 / 30.1 + 1.6 - (Vout - 1.6) *90.9 / 649
		//     = 1.6 * ((90.9 + 30.1) / 30.1) + 1.6 * 90.9 / 649 - Vout * 90.9 / 649
		//     = 1.6 * ((121 / 30.1) + 90.9 / 649) - Vout * 90.9 / 649
		// 	   = 6.656 - Vout / 7.14
		PWR_H_EN_H;
		PWR_L_EN_L;
		vin = 6656 - mv / 7.14f;
		mcp4728_write_single(2, vin);
	} else {
		// Vin = 3300 - Vout / 8
		PWR_H_EN_L;
		PWR_L_EN_H;
		vin = 3300 - mv / 8;
		mcp4728_write_single(3, vin);
	}
}

void dcdc_power_off(void)
{
	PWR_H_EN_L;
	PWR_L_EN_L;
}
