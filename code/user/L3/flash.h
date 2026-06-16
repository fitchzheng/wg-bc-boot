#ifndef __FLASH_H
#define __FLASH_H

#include <stdint.h>

typedef struct {
    uint32_t page_index;
    uint32_t size;
} flash_zone_t;

enum {
    FLASH_BOOT_LOADER,
    FLASH_APP,
//    FLASH_BACKUP_APP,
//    FLASH_RSVD,
    FLASH_UPDATE,
    FLASH_CFG,
    FLASH_ZONE_MAX,
};

extern flash_zone_t flash_zone[];

int32_t flash_get_page_index(uint32_t zone_index, uint32_t offset);
int32_t flash_erase(uint32_t zone_index, uint32_t offset);
// 整页写入/读取：offset 必须为页对齐(2KB)，pdata 长度必须为 FLASH_LOGICAL_PAGE_SIZE/2(uint16_t)
uint8_t  flash_write(uint32_t zone_index, uint32_t offset, const uint16_t *pdata);
uint8_t  flash_read(uint32_t zone_index, uint32_t offset, uint16_t *pdata);

#endif
