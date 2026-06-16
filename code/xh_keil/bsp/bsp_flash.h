#ifndef  __BSP_FLASH_H__
#define  __BSP_FLASH_H__ 

#include "hc32_ll.h"
#include "hc32_ll_efm.h"

// 用户数据 Flash 区域定义（最后 16KB）
#define FLASH_USER_BASE_ADDR 0x00000000U                                           // 用户 Flash 起始地址
#define FLASH_USER_TOTAL_SIZE (128 * 1024U)                                        // 总大小 128KB
#define FLASH_LOGICAL_PAGE_SIZE (4 * 1024U)                                        // 逻辑页大小 4KB
#define FLASH_LOGICAL_PAGE_COUNT (FLASH_USER_TOTAL_SIZE / FLASH_LOGICAL_PAGE_SIZE) // 共 128/4 = 32 页

// API 接口（仅按页操作）
uint8_t bsp_flash_erase_page(uint32_t page_index);
//uint8_t bsp_flash_write_page(uint32_t page_index, const uint16_t *data); 
uint8_t bsp_flash_write_page(uint32_t page_index, const uint32_t *data);// data长度必须为FLASH_LOGICAL_PAGE_SIZE/4
void bsp_flash_read_page(uint32_t page_index, uint32_t *data);

#endif

