#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#include <stdint.h>
#include "gd32f30x.h"

// 用户数据 Flash 区域定义（最后 16KB）
#define FLASH_USER_BASE_ADDR 0x08000000U                                           // 用户 Flash 起始地址
#define FLASH_USER_TOTAL_SIZE (256 * 1024U)                                        // 总大小 256KB
#define FLASH_LOGICAL_PAGE_SIZE (2 * 1024U)                                        // 逻辑页大小 2KB
#define FLASH_LOGICAL_PAGE_COUNT (FLASH_USER_TOTAL_SIZE / FLASH_LOGICAL_PAGE_SIZE) // 共 128 页

// API 接口（仅按页操作）
uint8_t bsp_flash_erase_page(uint32_t page_index);
uint8_t bsp_flash_write_page(uint32_t page_index, const uint16_t *data); // data长度必须为FLASH_LOGICAL_PAGE_SIZE/2
void bsp_flash_read_page(uint32_t page_index, uint16_t *data);           // data长度必须为FLASH_LOGICAL_PAGE_SIZE/2

#endif
