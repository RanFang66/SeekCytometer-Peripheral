/*
 * led_pattern.c
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 */

#include "led_pattern.h"
#include "tlc5957.h"
#include "stm32f0xx_hal.h"
#include <string.h>

/* 40 fps. Fast enough that a 200 ms blink still lands on exact edges (8 ticks
 * per cycle) and that breathing shows no steps, cheap enough that the flush it
 * can trigger - about 770 us at the current 1 MHz SCLK - stays under 4% of the
 * superloop. */
#define LED_TICK_MS			(25U)

/* Scene table. Colours are pre-gamma 0..255 values, i.e. what the eye should
 * see, not what goes into the GS latch; the square-law mapping happens in
 * pushFrame(). Brightness is per scene so that an alarm can be loud and an
 * idle breath can be dim without the host having to manage both.
 *
 * PROVISIONAL: this is a first cut following usual instrument convention. The
 * colours are finalised at S4 by stepping through them with "pat -s". */
static const led_scene_t sceneTable[LED_SCENE_COUNT] = {
	/*                     eff                 r    g    b   r2   g2   b2  bri  period */
	[LED_SCENE_OFF]       = {LED_EFFECT_OFF,    0,   0,   0,   0,   0,   0,   0,     0},
	[LED_SCENE_IDLE]      = {LED_EFFECT_BREATH, 0,  40, 255,   0,   0,   0,  40,  4000},
	[LED_SCENE_READY]     = {LED_EFFECT_SOLID,  0, 255,  40,   0,   0,   0,  60,     0},
	[LED_SCENE_RUNNING]   = {LED_EFFECT_CHASE,  0,  80, 255,   0,   0,  30,  70,  1500},
	[LED_SCENE_PAUSED]    = {LED_EFFECT_BREATH,255, 170,  0,   0,   0,   0,  60,  2000},
	[LED_SCENE_ALARM]     = {LED_EFFECT_BLINK, 255,   0,   0,  0,   0,   0,  90,   500},
	[LED_SCENE_FAULT]     = {LED_EFFECT_BLINK, 255,   0,   0,  0,   0,   0, 100,   200},
	[LED_SCENE_DOOR_OPEN] = {LED_EFFECT_SOLID,   0, 255, 255,  0,   0,   0,  70,     0},
	[LED_SCENE_MAINT]     = {LED_EFFECT_SOLID, 255, 255, 255,  0,   0,   0,  60,     0},
	[LED_SCENE_COMM_LOST] = {LED_EFFECT_BLINK, 255,  90,   0,  0,   0,   0,  70,  1000},
};

static led_scene_t	active;			/* scene being rendered, table copy or manual */
static uint8_t		sceneId;		/* index into sceneTable, or LED_SCENE_MANUAL_ID */
static uint16_t		phase;			/* animation position, wraps at 2^16 */
static uint16_t		phaseInc;		/* phase units per tick, see applyScene() */
static uint8_t		bright8;		/* brightness as 0..255, avoids a divide per pixel */
static uint32_t		lastTick;
static bool			running;
static bool			frameValid;		/* false forces the next frame out */
static uint8_t		shown[TLC_LED_COUNT][3];	/* last frame actually flushed */

/* ------------------------------------------------------------------------ */

/* v * s / 255 without a divide. (s + 1) rather than s so that a full-scale
 * pair maps to full scale: 255 * 256 >> 8 = 255, where 255 * 255 >> 8 = 254. */
static inline uint8_t scale8(uint8_t v, uint8_t s)
{
	return (uint8_t)(((uint16_t)v * ((uint16_t)s + 1U)) >> 8);
}

/* Recompute everything derived from the scene fields. Does not touch phase or
 * the running flag - restarting an animation is the caller's decision, so that
 * a brightness change mid-breath does not jump back to the start. */
static void applyScene(void)
{
	uint16_t p = active.period_ms;

	if (active.brightness > 100U) {
		active.brightness = 100U;
	}
	if (active.effect >= LED_EFFECT_COUNT) {
		active.effect = LED_EFFECT_OFF;
	}

	/* 0..100 -> 0..255. 653/256 is 2.5508, and 100 * 653 >> 8 is exactly 255,
	 * so this hits both endpoints without a division. */
	bright8 = (uint8_t)(((uint32_t)active.brightness * 653U) >> 8);

	/* The only division in this file, and it runs once per scene change.
	 * Anything faster than two ticks per cycle cannot be rendered, so clamp
	 * rather than alias. */
	if (p < (LED_TICK_MS * 2U)) {
		p = (uint16_t)(LED_TICK_MS * 2U);
	}
	phaseInc = (uint16_t)(((uint32_t)LED_TICK_MS * 65536UL) / p);
	if (phaseInc == 0U) {
		phaseInc = 1U;		/* period longer than 27 minutes; still moves */
	}

	frameValid = false;		/* force the next frame out even if it is identical */
}

/* Apply a freshly selected scene: recompute, rewind the animation, resume. */
static void startScene(void)
{
	applyScene();
	phase = 0U;
	running = true;
}

/* Render the current phase into a 14 x RGB frame of pre-gamma values. */
static void renderFrame(uint8_t f[TLC_LED_COUNT][3])
{
	uint8_t t = (uint8_t)(phase >> 8);	/* 0..255 within the cycle */
	uint8_t lvl;						/* colour-1 intensity this frame */
	uint8_t idx;
	uint8_t i;

	switch (active.effect) {
	case LED_EFFECT_SOLID:
		lvl = 255U;
		goto fill_uniform;

	case LED_EFFECT_BREATH:
		/* Triangle rather than a sine table: after the square-law gamma in
		 * pushFrame() a linear ramp already looks like a smooth breath, and a
		 * 256-entry table would cost more Flash than this board has to spare. */
		lvl = (t < 128U) ? (uint8_t)(t << 1) : (uint8_t)((255U - t) << 1);
		goto fill_uniform;

	case LED_EFFECT_BLINK:
		lvl = (t < 128U) ? 255U : 0U;
		goto fill_uniform;

	case LED_EFFECT_CHASE:
	case LED_EFFECT_WIPE:
		/* phase * 14 >> 16 spreads the cycle over the strip without a divide. */
		idx = (uint8_t)(((uint32_t)phase * TLC_LED_COUNT) >> 16);
		for (i = 0; i < TLC_LED_COUNT; i++) {
			bool lit = (active.effect == LED_EFFECT_CHASE) ? (i == idx) : (i <= idx);
			f[i][0] = scale8(lit ? active.r  : active.r2,  bright8);
			f[i][1] = scale8(lit ? active.g  : active.g2,  bright8);
			f[i][2] = scale8(lit ? active.b  : active.b2,  bright8);
		}
		return;

	case LED_EFFECT_OFF:
	default:
		memset(f, 0, TLC_LED_COUNT * 3U);
		return;
	}

fill_uniform:
	{
		uint8_t r = scale8(scale8(active.r, lvl), bright8);
		uint8_t g = scale8(scale8(active.g, lvl), bright8);
		uint8_t b = scale8(scale8(active.b, lvl), bright8);
		for (i = 0; i < TLC_LED_COUNT; i++) {
			f[i][0] = r;
			f[i][1] = g;
			f[i][2] = b;
		}
	}
}

/* 8-bit perceptual value -> 16-bit GS. gs = v * v is gamma 2.0: one multiply,
 * no table, and close enough to the 2.2 the eye wants for an indicator. Full
 * scale lands on 65025 rather than 65535, which is 0.8% of nothing. */
static void pushFrame(const uint8_t f[TLC_LED_COUNT][3])
{
	uint8_t i;

	for (i = 0; i < TLC_LED_COUNT; i++) {
		TLC5957_SetRGB(i,
					   (uint16_t)((uint16_t)f[i][0] * f[i][0]),
					   (uint16_t)((uint16_t)f[i][1] * f[i][1]),
					   (uint16_t)((uint16_t)f[i][2] * f[i][2]));
	}
	TLC5957_Flush();
}

/* ------------------------------------------------------------------------ */

void LedPattern_Init(void)
{
	sceneId = LED_SCENE_OFF;
	active = sceneTable[LED_SCENE_OFF];
	startScene();
	lastTick = HAL_GetTick();
}

void LedPattern_Tick(uint32_t now)
{
	uint8_t f[TLC_LED_COUNT][3];

	if (!running) {
		return;
	}
	if ((uint32_t)(now - lastTick) < LED_TICK_MS) {
		return;
	}
	lastTick = now;
	phase = (uint16_t)(phase + phaseInc);

	renderFrame(f);

	/* The point of the whole module: a static scene reaches here 40 times a
	 * second and touches the bus none of them. */
	if (frameValid && memcmp(f, shown, sizeof(shown)) == 0) {
		return;
	}
	memcpy(shown, f, sizeof(shown));
	frameValid = true;
	pushFrame(f);
}

bool LedPattern_SetScene(uint8_t scene)
{
	if (scene >= LED_SCENE_COUNT) {
		return false;
	}
	sceneId = scene;
	active = sceneTable[scene];
	startScene();
	return true;
}

void LedPattern_SetManual(const led_scene_t *s)
{
	sceneId = LED_SCENE_MANUAL_ID;
	active = *s;
	startScene();
}

void LedPattern_SetBrightness(uint8_t pct)
{
	active.brightness = pct;
	applyScene();		/* phase untouched: no visible jump mid-animation */
}

void LedPattern_Suspend(void)
{
	running = false;
	frameValid = false;		/* whatever the calibration wrote is not our frame */
}

bool LedPattern_IsRunning(void)
{
	return running;
}

uint8_t LedPattern_GetScene(void)
{
	return sceneId;
}

const led_scene_t *LedPattern_GetActive(void)
{
	return &active;
}
