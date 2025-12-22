#include "flash.h"

uint32_t flash_from_addr_to_sector(uint32_t addr)
{
	if (addr < 0x08000000)
		return -1;
	else if (addr < 0x08004000)
		return FLASH_SECTOR_0;
	else if (addr < 0x08008000)
		return FLASH_SECTOR_1;
	else if (addr < 0x0800c000)
		return FLASH_SECTOR_2;
	else if (addr < 0x08010000)
		return FLASH_SECTOR_3;
	else if (addr < 0x08020000)
		return FLASH_SECTOR_4;
	else if (addr < 0x08040000)
		return FLASH_SECTOR_5;
	else if (addr < 0x08060000)
		return FLASH_SECTOR_6;
	else if (addr < 0x08080000)
		return FLASH_SECTOR_7;
	else if (addr < 0x080A0000)
		return FLASH_SECTOR_8;
	else if (addr < 0x080C0000)
		return FLASH_SECTOR_9;
	else if (addr < 0x080E0000)
		return FLASH_SECTOR_10;
	else if (addr < 0x08100000)
		return FLASH_SECTOR_11;
	else
		return -1;

}

void flash_write_package(uint32_t addr, uint8_t *buf, uint16_t len)
{
	uint16_t i, d, checksum;
	uint16_t write_size;
	uint32_t err;
	FLASH_EraseInitTypeDef erase_init;

	erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
	erase_init.Banks = FLASH_BANK_1;
	erase_init.Sector = flash_from_addr_to_sector(addr);
	erase_init.NbSectors = 1;
	erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_2;

	// 只写在secotr3 且长度不超
	if (erase_init.Sector != FLASH_SECTOR_3)
		return;
	write_size = (len + 1)&0xFFFE;
	if (addr + len + 2 > 0x08010000)
		return;

	checksum = 0;
	for(i = 0; i < write_size; i++)
			checksum += buf[i];
	__disable_irq();
	HAL_FLASH_Unlock();
	HAL_FLASHEx_Erase(&erase_init, &err);
	for (i = 0; i < len; i += 2) {
		d = buf[i] | (buf[i + 1] << 8);
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, d);
	}
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + write_size, checksum);
	HAL_FLASH_Lock();
	__enable_irq();
}

int flash_check_package(uint32_t startaddr,uint16_t len)
{
	uint16_t write_size, i, checksum;
	write_size = (len + 1)&0xFFFE;

	checksum = 0;
	for(i = 0; i < write_size; i++)
		checksum += *(uint8_t *)(startaddr + i);
	if(checksum != (*(uint16_t *)(startaddr + write_size)))
		return -1;
	return 0;
}

uint16_t flash_get_checksum(uint8_t *buf, uint16_t len)
{
	uint16_t write_size, i, checksum;
	write_size = (len + 1)&0xFFFE;

	checksum = 0;
	for(i = 0; i < write_size; i++)
		checksum += buf[i];
	return checksum;
}

void flash_write_data(uint32_t addr, uint8_t *buf, uint16_t len)
{
	uint16_t i, d;
	uint32_t err;
	FLASH_EraseInitTypeDef erase_init;

	erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
	erase_init.Banks = FLASH_BANK_1;
	erase_init.Sector = flash_from_addr_to_sector(addr);
	erase_init.NbSectors = 1;
	erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_2;

	__disable_irq();
	HAL_FLASH_Unlock();
	HAL_FLASHEx_Erase(&erase_init, &err);
	for (i = 0; i < len; i += 2) {
		d = buf[i] | (buf[i + 1] << 8);
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, d);
	}
	HAL_FLASH_Lock();
	__enable_irq();
}

void flash_write_data_continue(uint32_t addr, uint8_t *buf, uint16_t len)
{
	uint16_t i, d;
	uint32_t err;
	FLASH_EraseInitTypeDef erase_init;
	uint32_t start_sector = flash_from_addr_to_sector(addr - 1);
	uint32_t end_sector = flash_from_addr_to_sector(addr + len - 1);

	__disable_irq();
	HAL_FLASH_Unlock();

	if (start_sector != end_sector) {
		erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
		erase_init.Banks = FLASH_BANK_1;
		erase_init.Sector = start_sector + 1;
		erase_init.NbSectors = end_sector - start_sector;
		erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_2;
		HAL_FLASHEx_Erase(&erase_init, &err);
	}
	for (i = 0; i < len; i += 2) {
		d = buf[i] | (buf[i + 1] << 8);
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, d);
	}
	HAL_FLASH_Lock();
	__enable_irq();
}


