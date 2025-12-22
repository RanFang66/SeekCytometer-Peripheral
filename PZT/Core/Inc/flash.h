#ifndef __FLASH_H
#define __FLASH_H

#include "main.h"
// 第一种  单page参数保存 自带擦除 和加校验后缀
		// 给目标地址 转换为sector序号
// 第二种  不满1page的写入 首次写入 单page擦除 给地址 转换为目标sector
// 第三种  不满1page的写入 后续写入 给地址 转换为目标sector

#define PARAM_STORE_ADDR 0x080E0000 // 0x08004000
#define PARAM_STROE_ADDR_END 0x080FFFFF // 0x0800BFFF
#define BOOT_STORE_ADDR 0x0800C000

uint32_t flash_from_addr_to_sector(uint32_t addr);
void flash_write_package(uint32_t addr, uint8_t *buf, uint16_t len);
uint16_t flash_get_checksum(uint8_t *buf, uint16_t len);
int flash_check_package(uint32_t startaddr,uint16_t len);
void flash_write_data(uint32_t addr, uint8_t *buf, uint16_t len);
void flash_write_data_continue(uint32_t addr, uint8_t *buf, uint16_t len);
#endif
