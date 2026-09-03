/*
 * param_store.h
 *
 *  Non-volatile storage for the pressure-control PI / feed-forward parameters.
 *
 *  The five pressure channels' Kp / Ki / feed-forward used to live in RAM only,
 *  so every reset threw away whatever the HMI had tuned. This module keeps them
 *  in the last flash sector and reloads them before the scheduler starts.
 *
 *  Storage layout: sector 11 (0x080E0000, 128 KB) is used as an append-only
 *  journal of fixed 64-byte records. A save writes one record into the next
 *  free slot -- no erase, so the CPU stalls for well under 2 ms. The sector is
 *  only erased when it runs out of slots, and that erase is deliberately
 *  pulled forward into ParamStore_Init() (see param_store.c) where a 1-2 s
 *  stall costs nothing.
 *
 *  Created on: 2026-09-02
 *      Author: ranfa
 */

#ifndef INC_PARAM_STORE_H_
#define INC_PARAM_STORE_H_

#include <stdint.h>
#include <stdbool.h>

#include "press_control.h"

/* Flash geometry. STM32F405RG sector 11 is the last 128 KB of the 1 MB bank.
 * STM32F405RGTX_FLASH.ld shrinks the FLASH region to 896K so that a link error
 * -- not a silent overwrite -- is what happens if the firmware ever grows into
 * this area. */
/* Overridable so that a host-side test can point the journal at a plain array;
 * the firmware always uses the real sector-11 address. */
#ifndef PARAM_STORE_ADDR
#define PARAM_STORE_ADDR		0x080E0000UL
#endif
#define PARAM_STORE_SIZE		(128U * 1024U)
#define PARAM_SLOT_SIZE			64U
#define PARAM_SLOT_COUNT		(PARAM_STORE_SIZE / PARAM_SLOT_SIZE)	/* 2048 */

#define PARAM_MAGIC				0x3143464DUL	/* "MFC1", little endian */
#define PARAM_VERSION			1U

/* Erase the sector at boot once fewer than this many slots are left, so that a
 * save at run time never has to erase. */
#define PARAM_COMPACT_SLOTS		16U

/* A save is written this long after the last parameter change. The HMI pushes
 * the five channels as five separate SET_PI commands, so the debounce collapses
 * them into a single record. */
#define PARAM_SAVE_DEBOUNCE_MS	1000U

/* Bits of the status word exposed on holding register index 87. */
#define PARAM_ST_LOADED			(1U << 0)	/* a valid record was read at boot */
#define PARAM_ST_DEFAULTS		(1U << 1)	/* nothing valid found, using build defaults */
#define PARAM_ST_DIRTY			(1U << 2)	/* changes pending inside the debounce window */
#define PARAM_ST_SAVE_ERROR		(1U << 3)	/* the last write failed */
#define PARAM_ST_COMPACTED		(1U << 4)	/* the sector was erased during this boot */

/* One stored record. Kp and Ki are held exactly as the HMI sends them over
 * Modbus (value x100 in a uint16); the feed-forward is a raw DAC count. Keeping
 * the on-wire encoding means the load path can reuse the Modbus conversion and
 * no float ever round-trips through flash. */
typedef struct {						/* offset */
	uint32_t magic;						/*  0 */
	uint16_t version;					/*  4 */
	uint16_t length;					/*  6  bytes covered by crc */
	uint32_t seq;						/*  8  increments on every save */
	uint16_t kp_x100[PRESS_CTRL_CH_NUM];/* 12 */
	uint16_t ki_x100[PRESS_CTRL_CH_NUM];/* 22 */
	uint16_t ff[PRESS_CTRL_CH_NUM];		/* 32 */
	uint16_t reserved[10];				/* 42  room to grow, written as 0xFFFF */
	uint16_t crc;						/* 62  CRC-16/MODBUS over [0, 62) */
} ParamRecord_t;						/* 64 */

/**
 * @brief Reads the stored parameters into the RAM cache.
 *
 * Must run before the scheduler starts (it may erase the sector, which stalls
 * the CPU for up to 2 s). If nothing valid is stored the cache is filled with
 * the compile-time defaults, so the cache is always usable afterwards.
 */
void ParamStore_Init(void);

/**
 * @brief Returns the RAM cache. Never NULL, valid after ParamStore_Init().
 */
const ParamRecord_t *ParamStore_GetCached(void);

/**
 * @brief Records one channel's new parameters and arms the save debounce.
 *
 * Does nothing if the values already match the cache, so re-sending the same
 * parameters costs no flash wear.
 */
void ParamStore_UpdateChannel(uint8_t ch, uint16_t kp_x100, uint16_t ki_x100, uint16_t ff);

/**
 * @brief Drives the debounce and performs the actual flash write.
 *
 * Flash is only ever written from here, and this is only called from the
 * pressure-control task -- that single-writer rule is what lets the module do
 * without a mutex.
 */
void ParamStore_Tick(uint32_t now);

/**
 * @brief Asks for an immediate save. Only sets a flag; ParamStore_Tick() does
 *        the write. Safe to call from any task.
 */
void ParamStore_RequestSave(void);

/**
 * @brief Asks for the whole parameter sector to be erased. Only sets a flag;
 *        ParamStore_Tick() does the erase. The next boot falls back to defaults.
 */
void ParamStore_RequestErase(void);

/**
 * @brief Status word for holding register index 87. Low byte is PARAM_ST_*,
 *        high byte is the percentage of slots used.
 */
uint16_t ParamStore_GetStatus(void);

/**
 * @brief Sequence number of the record currently in the cache (diagnostics).
 */
uint32_t ParamStore_GetSeq(void);

/**
 * @brief Number of slots written so far (diagnostics).
 */
uint16_t ParamStore_GetUsedSlots(void);

#endif /* INC_PARAM_STORE_H_ */
