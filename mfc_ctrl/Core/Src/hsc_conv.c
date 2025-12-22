/*
 * hsc_conv.c
 *
 *  Created on: Oct 24, 2025
 *      Author: ranfa
 */

// hsc_conv.c
#include "hsc_conv.h"

#define HSC_14B_10PCT        1638.0f
#define HSC_14B_90PCT        14745.0f
#define HSC_TEMP_FULLSCALE   2047.0f // 11-bit


#define P_MAX_PSI					15.0	// PSI
#define P_MIN_PSI					-15.0	// PSI

#define PSI_TO_MBAR					68.94757

#define P_MAX_MBAR					(P_MAX_PSI * PSI_TO_MBAR)  // mbar
#define P_MIN_MBAR					(P_MIN_PSI * PSI_TO_MBAR) // mbar


#define T_MIN						(-50.0)
#define T_MAX						(150.0)


#define P_SOURCE_MAX_PSI 		60.0
#define P_SOURCE_MIN_PSI		-60.0

#define P_SOURCE_MAX_MBAR					(P_SOURCE_MAX_PSI * PSI_TO_MBAR)  // mbar
#define P_SOURCE_MIN_MBAR					(P_SOURCE_MIN_PSI * PSI_TO_MBAR) // mbar

static inline float clampf(float x, float lo, float hi){ return x < lo ? lo : (x > hi ? hi : x); }

float HSC_CountsToUnit_TF_A(uint16_t counts)
{
    float c = (float)(counts & 0x3FFF);
    float u = (c - HSC_14B_10PCT) / (HSC_14B_90PCT - HSC_14B_10PCT);
    return clampf(u, 0.0f, 1.0f);
}

float HSC_CountsToPressure_mbar(uint16_t counts)
{
    float u = HSC_CountsToUnit_TF_A(counts);
    return P_MIN_MBAR + (P_MAX_MBAR - P_MIN_MBAR) * u;
}


float HSC_SourceCountsToPressure_mbar(uint16_t counts)
{
    float u = HSC_CountsToUnit_TF_A(counts);
    float pmbar = P_SOURCE_MIN_MBAR + (P_SOURCE_MAX_MBAR - P_SOURCE_MIN_MBAR) * u;
    return pmbar;
}

float HSC_CountsToPressure_psi(uint16_t counts)
{
	float u = HSC_CountsToUnit_TF_A(counts);
	return P_MIN_PSI + (P_MAX_PSI - P_MIN_PSI) * u;
}



float HSC_TempCountsToCelsius(uint16_t t_counts)
{
    float t = (float)(t_counts & 0x07FF);
    float u = t / HSC_TEMP_FULLSCALE;      // 0..1
    return T_MIN + (T_MAX - T_MIN) * u;
}

