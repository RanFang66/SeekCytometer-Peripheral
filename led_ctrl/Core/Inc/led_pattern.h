/*
 * led_pattern.h
 *
 *  Created on: 2026年8月31日
 *      Author: ranfa
 *
 *  Scene / effect engine for the 14-LED strip.
 *
 *  The whole strip shows one colour, optionally animated - there is no
 *  per-LED addressing in the external interface. The host normally writes a
 *  single scene number and the firmware looks up colour, effect and period
 *  from a Flash table; the manual path exists so colours can be retuned on the
 *  bench (and later over Modbus) without reflashing.
 *
 *  Two properties this module is built around:
 *    - No division and no floating point on the hot path. A Cortex-M0 has
 *      neither, and __udivsi3 is 460 bytes of libgcc. Animation phase is a
 *      free-running uint16 advanced by a precomputed increment, so the only
 *      division in the file happens once per scene change.
 *    - The strip is only written when the rendered frame actually changes.
 *      The TLC5957 keeps displaying its GS latch on its own (auto display
 *      repeat), so a static scene costs zero bus traffic. This is directly
 *      observable: scope SCLK during a SOLID scene and it must be silent.
 */

#ifndef INC_LED_PATTERN_H_
#define INC_LED_PATTERN_H_

#include <stdint.h>
#include <stdbool.h>

/* ---- Effects --------------------------------------------------------- */
typedef enum {
	LED_EFFECT_OFF = 0,		/* dark                                       */
	LED_EFFECT_SOLID,		/* colour 1, constant                         */
	LED_EFFECT_BREATH,		/* colour 1, triangle-ramped                  */
	LED_EFFECT_BLINK,		/* colour 1 / dark, 50% duty                  */
	LED_EFFECT_CHASE,		/* one lit dot travelling over colour 2       */
	LED_EFFECT_WIPE,		/* colour 1 filling in over colour 2          */
	LED_EFFECT_COUNT
} led_effect_t;

/* ---- One scene ------------------------------------------------------- */
/* Field order keeps the struct at 10 bytes with no padding: eight uint8
 * followed by the uint16. Ten scenes therefore cost 100 bytes of Flash. */
typedef struct {
	uint8_t  effect;			/* led_effect_t                           */
	uint8_t  r, g, b;			/* colour 1, 0..255                       */
	uint8_t  r2, g2, b2;		/* colour 2 / background, 0..255          */
	uint8_t  brightness;		/* 0..100 %                               */
	uint16_t period_ms;			/* one full animation cycle; 0 if static  */
} led_scene_t;

/* ---- Scene numbers --------------------------------------------------- */
/* These are the values carried in the CSLED_SCENE_SET holding register, so
 * they are part of the external interface - append, never renumber. */
typedef enum {
	LED_SCENE_OFF = 0,
	LED_SCENE_IDLE,
	LED_SCENE_READY,
	LED_SCENE_RUNNING,
	LED_SCENE_PAUSED,
	LED_SCENE_ALARM,
	LED_SCENE_FAULT,
	LED_SCENE_DOOR_OPEN,
	LED_SCENE_MAINT,
	LED_SCENE_COMM_LOST,		/* entered by the firmware only, see below */
	LED_SCENE_COUNT
} led_scene_id_t;

/* The highest scene the host is allowed to select. COMM_LOST is what the
 * board shows when the host has stopped talking to it, so letting the host
 * set it would make a live link indistinguishable from a dead one. */
#define LED_SCENE_HOST_MAX		(LED_SCENE_MAINT)

void LedPattern_Init(void);
void LedPattern_Tick(uint32_t now);

/* Select a table scene. Rejects out-of-range ids; COMM_LOST is accepted here
 * because the caller inside the firmware needs it - the range check against
 * LED_SCENE_HOST_MAX belongs to the Modbus/shell layer. Resumes the engine if
 * it was suspended. */
bool LedPattern_SetScene(uint8_t scene);

/* Override the table with an explicit effect. Marks the current scene as
 * LED_SCENE_MANUAL_ID so status reporting can tell the two apart. */
#define LED_SCENE_MANUAL_ID		(0xFFU)
void LedPattern_SetManual(const led_scene_t *s);

/* Brightness applies on top of whatever is running, table scene or manual. */
void LedPattern_SetBrightness(uint8_t pct);

/* Stop rendering and leave the GS latch exactly as it is. The S3 calibration
 * commands write the shadow buffer directly, and a 40 fps engine flushing on
 * top of them would erase the thing being measured. */
void LedPattern_Suspend(void);
bool LedPattern_IsRunning(void);

uint8_t LedPattern_GetScene(void);
const led_scene_t *LedPattern_GetActive(void);

#endif /* INC_LED_PATTERN_H_ */
