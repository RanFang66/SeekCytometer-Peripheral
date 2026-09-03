/*
 * param_store.c
 *
 *  Append-only journal of pressure-control PI / feed-forward parameters in the
 *  last flash sector. See param_store.h for the layout and the reasoning.
 *
 *  Created on: 2026-09-02
 *      Author: ranfa
 */

#include "param_store.h"

#include <stddef.h>
#include <string.h>

#include "stm32f4xx_hal.h"
#include "debug_shell.h"

/* Sector 11 = the 128 KB at PARAM_STORE_ADDR. Kept here rather than in the
 * header so that param_store.h does not have to pull in the HAL. */
#define PARAM_STORE_SECTOR		FLASH_SECTOR_11

/* Word programming needs VDD >= 2.7 V; the board runs at 3.3 V. */
#define PARAM_VOLTAGE_RANGE		FLASH_VOLTAGE_RANGE_3

#define PARAM_ERASED_WORD		0xFFFFFFFFUL

#define PARAM_CRC_LEN			((uint16_t)offsetof(ParamRecord_t, crc))

_Static_assert(sizeof(ParamRecord_t) == PARAM_SLOT_SIZE,
		"ParamRecord_t must exactly fill one flash slot");
_Static_assert((sizeof(ParamRecord_t) % 4U) == 0U,
		"ParamRecord_t must be word aligned for HAL_FLASH_Program");

static ParamRecord_t cache;
static uint16_t  writeSlot;			/* next free slot index */
static uint16_t  statusFlags;
static bool      dirty;
static bool      changePending;		/* a change is waiting to (re)start the debounce */
static uint32_t  dirtyTime;
static volatile bool saveRequest;
static volatile bool eraseRequest;

/* CRC-16/MODBUS. modbus_slave.c has the same routine but it is static there and
 * not declared in the header; duplicating twelve lines is safer than reaching
 * into the Modbus slave for this. */
static uint16_t ParamStore_CRC16(const uint8_t *buffer, uint16_t length)
{
	uint16_t crc = 0xFFFF;
	for (uint16_t i = 0; i < length; i++) {
		crc ^= buffer[i];
		for (int j = 0; j < 8; j++) {
			if (crc & 1) crc = (crc >> 1) ^ 0xA001;
			else crc >>= 1;
		}
	}
	return crc;
}

static inline const ParamRecord_t *slotAt(uint16_t slot)
{
	return (const ParamRecord_t *)(PARAM_STORE_ADDR + (uint32_t)slot * PARAM_SLOT_SIZE);
}

static inline bool slotIsErased(uint16_t slot)
{
	return *(const volatile uint32_t *)slotAt(slot) == PARAM_ERASED_WORD;
}

static bool recordIsValid(const ParamRecord_t *rec)
{
	if (rec->magic != PARAM_MAGIC || rec->version != PARAM_VERSION) {
		return false;
	}
	if (rec->length != PARAM_CRC_LEN) {
		return false;
	}
	return ParamStore_CRC16((const uint8_t *)rec, PARAM_CRC_LEN) == rec->crc;
}

static void fillWithDefaults(ParamRecord_t *rec)
{
	memset(rec, 0, sizeof(*rec));
	rec->magic   = PARAM_MAGIC;
	rec->version = PARAM_VERSION;
	rec->length  = PARAM_CRC_LEN;
	rec->seq     = 0;
	for (uint8_t i = 0; i < PRESS_CTRL_CH_NUM; i++) {
		rec->kp_x100[i] = (uint16_t)(PRESS_CTRL_DEFAULT_KP * 100.0f);
		rec->ki_x100[i] = (uint16_t)(PRESS_CTRL_DEFAULT_KI * 100.0f);
		rec->ff[i]      = (uint16_t)PRESS_CTRL_DEFAULT_FEEDFORWARD;
	}
	memset(rec->reserved, 0xFF, sizeof(rec->reserved));
}

/**
 * @brief Erases the whole parameter sector. Stalls the CPU for 1-2 s: the F405
 *        has a single flash bank, so nothing -- not even an interrupt -- runs
 *        while the erase is in progress.
 */
static bool eraseSector(void)
{
	FLASH_EraseInitTypeDef eraseInit = {
		.TypeErase    = FLASH_TYPEERASE_SECTORS,
		.Banks        = FLASH_BANK_1,
		.Sector       = PARAM_STORE_SECTOR,
		.NbSectors    = 1,
		.VoltageRange = PARAM_VOLTAGE_RANGE,
	};
	uint32_t sectorError = 0;

	if (HAL_FLASH_Unlock() != HAL_OK) {
		return false;
	}
	HAL_StatusTypeDef ret = HAL_FLASHEx_Erase(&eraseInit, &sectorError);
	HAL_FLASH_Lock();

	if (ret != HAL_OK) {
		return false;
	}
	writeSlot = 0;
	return true;
}

/**
 * @brief Programs one record into the given slot, word by word.
 *        16 words, ~0.3 ms typical and under 2 ms worst case.
 */
static bool programSlot(uint16_t slot, const ParamRecord_t *rec)
{
	const uint32_t base = PARAM_STORE_ADDR + (uint32_t)slot * PARAM_SLOT_SIZE;
	const uint32_t *src = (const uint32_t *)rec;
	bool ok = true;

	if (HAL_FLASH_Unlock() != HAL_OK) {
		return false;
	}
	for (uint32_t i = 0; i < sizeof(ParamRecord_t) / 4U; i++) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, base + i * 4U, src[i]) != HAL_OK) {
			ok = false;
			break;
		}
	}
	HAL_FLASH_Lock();

	/* Read the record back through the CRC so a half-written slot is caught
	 * here rather than at the next boot. */
	if (ok && !recordIsValid(slotAt(slot))) {
		ok = false;
	}
	return ok;
}

/**
 * @brief Writes the cache as a new record. Erases first if the sector is full,
 *        which normally cannot happen because ParamStore_Init() compacts.
 */
static bool writeCache(void)
{
	if (writeSlot >= PARAM_SLOT_COUNT) {
		if (!eraseSector()) {
			return false;
		}
		statusFlags |= PARAM_ST_COMPACTED;
	}

	cache.seq++;
	cache.magic   = PARAM_MAGIC;
	cache.version = PARAM_VERSION;
	cache.length  = PARAM_CRC_LEN;
	cache.crc     = ParamStore_CRC16((const uint8_t *)&cache, PARAM_CRC_LEN);

	/* Advance past the slot either way. A slot whose programming failed has
	 * bits burned into it and can never hold a good record again -- retrying
	 * into it would fail forever, because NOR flash can only clear bits. */
	bool ok = programSlot(writeSlot, &cache);
	writeSlot++;
	return ok;
}

void ParamStore_Init(void)
{
	uint16_t slot = 0;

	statusFlags = 0;
	dirty = false;
	changePending = false;
	saveRequest = false;
	eraseRequest = false;

	/* Records are appended in order, so the first erased slot is the write
	 * pointer and everything below it has been written. */
	while (slot < PARAM_SLOT_COUNT && !slotIsErased(slot)) {
		slot++;
	}
	writeSlot = slot;

	/* Walk back to the newest record that passes its CRC. Anything after it is
	 * a slot that was interrupted mid-write by a power cut. */
	bool found = false;
	for (uint16_t i = writeSlot; i > 0; i--) {
		const ParamRecord_t *rec = slotAt(i - 1);
		if (recordIsValid(rec)) {
			memcpy(&cache, rec, sizeof(cache));
			found = true;
			break;
		}
	}

	if (found) {
		statusFlags |= PARAM_ST_LOADED;
	} else {
		fillWithDefaults(&cache);
		statusFlags |= PARAM_ST_DEFAULTS;
	}

	/* Pull the expensive erase forward to boot time: the scheduler has not
	 * started, there is no watchdog and no communication, so a 1-2 s stall is
	 * harmless here and never happens while the pressure loop is running. */
	if (PARAM_SLOT_COUNT - writeSlot < PARAM_COMPACT_SLOTS) {
		if (eraseSector()) {
			statusFlags |= PARAM_ST_COMPACTED;
			if (found && !writeCache()) {
				statusFlags |= PARAM_ST_SAVE_ERROR;
			}
		} else {
			statusFlags |= PARAM_ST_SAVE_ERROR;
		}
	}
}

const ParamRecord_t *ParamStore_GetCached(void)
{
	return &cache;
}

void ParamStore_UpdateChannel(uint8_t ch, uint16_t kp_x100, uint16_t ki_x100, uint16_t ff)
{
	if (ch >= PRESS_CTRL_CH_NUM) {
		return;
	}
	if (cache.kp_x100[ch] == kp_x100 && cache.ki_x100[ch] == ki_x100 && cache.ff[ch] == ff) {
		return;		/* unchanged: do not spend a flash slot on it */
	}

	cache.kp_x100[ch] = kp_x100;
	cache.ki_x100[ch] = ki_x100;
	cache.ff[ch]      = ff;

	/* The debounce timestamp is taken in ParamStore_Tick(), which runs in the
	 * same task a moment later. Keeping every timestamp on the caller's clock
	 * avoids mixing the HAL time base (TIM-driven here) with the RTOS tick. */
	dirty = true;
	changePending = true;
	statusFlags |= PARAM_ST_DIRTY;
}

void ParamStore_RequestSave(void)
{
	saveRequest = true;
}

void ParamStore_RequestErase(void)
{
	eraseRequest = true;
}

void ParamStore_Tick(uint32_t now)
{
	if (changePending) {
		/* Restart the debounce on every change, so a burst of per-channel
		 * writes from the HMI collapses into a single record. */
		changePending = false;
		dirtyTime = now;
	}

	if (eraseRequest) {
		eraseRequest = false;
		dirty = false;
		changePending = false;
		statusFlags &= (uint16_t)~PARAM_ST_DIRTY;
		if (eraseSector()) {
			statusFlags |= PARAM_ST_COMPACTED;
			LOG_INFO("Param store erased, defaults will be used after reset");
		} else {
			statusFlags |= PARAM_ST_SAVE_ERROR;
			LOG_ERROR("Param store erase FAILED");
		}
		return;
	}

	bool due = saveRequest || (dirty && (uint32_t)(now - dirtyTime) >= PARAM_SAVE_DEBOUNCE_MS);
	if (!due) {
		return;
	}
	saveRequest = false;

	if (writeCache()) {
		dirty = false;
		statusFlags &= (uint16_t)~(PARAM_ST_DIRTY | PARAM_ST_SAVE_ERROR);
		statusFlags |= PARAM_ST_LOADED;
		statusFlags &= (uint16_t)~PARAM_ST_DEFAULTS;
		LOG_INFO("Press params saved, seq %lu, slot %u", (unsigned long)cache.seq, writeSlot);
	} else {
		/* Keep the dirty flag so the next tick retries. */
		statusFlags |= PARAM_ST_SAVE_ERROR;
		LOG_ERROR("Press params save FAILED");
	}
}

uint16_t ParamStore_GetStatus(void)
{
	uint16_t usedPct = (uint16_t)(((uint32_t)writeSlot * 100U) / PARAM_SLOT_COUNT);
	return (uint16_t)((usedPct << 8) | (statusFlags & 0x00FFU));
}

uint32_t ParamStore_GetSeq(void)
{
	return cache.seq;
}

uint16_t ParamStore_GetUsedSlots(void)
{
	return writeSlot;
}
