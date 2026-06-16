#include "flash.h"
#include "bsp_flash.h"
#include <string.h>


flash_zone_t flash_zone[] = {
    [FLASH_BOOT_LOADER] = {FLASH_BOOT_LOADER, 5 * FLASH_LOGICAL_PAGE_SIZE},
    [FLASH_APP] = {FLASH_APP, 26 * FLASH_LOGICAL_PAGE_SIZE},
    //[FLASH_BACKUP_APP] = {FLASH_BACKUP_APP, 50 * FLASH_LOGICAL_PAGE_SIZE},
    //[FLASH_RSVD] = {FLASH_RSVD, 3 * FLASH_LOGICAL_PAGE_SIZE},
    [FLASH_UPDATE] = {FLASH_UPDATE, FLASH_LOGICAL_PAGE_SIZE},
    //[FLASH_CFG] = {FLASH_CFG, FLASH_LOGICAL_PAGE_SIZE},
};

int32_t flash_get_page_index(uint32_t zone_index, uint32_t offset)
{
    if ((zone_index >= FLASH_ZONE_MAX) ||
        (offset % FLASH_LOGICAL_PAGE_SIZE != 0))
    {
        return -1;
    }

    int32_t page_index = 0;
    uint32_t zone_index_cnt = 0;
    while (zone_index_cnt < FLASH_ZONE_MAX)
    {
        if (zone_index_cnt == zone_index)
        {
            page_index += offset / FLASH_LOGICAL_PAGE_SIZE;
            break;
        }
        page_index += flash_zone[zone_index_cnt].size / FLASH_LOGICAL_PAGE_SIZE;
        zone_index_cnt++;
    }
    if (zone_index_cnt == FLASH_ZONE_MAX)
    {
        return 0;
    }
    return page_index;
}

int32_t flash_erase(uint32_t zone_index, uint32_t offset)
{
    int32_t page_index = flash_get_page_index(zone_index, offset);
    if (page_index < 0)
    {
        return -1;
    }
    bsp_flash_erase_page(page_index);
    return page_index;
}

// 整页写入：2KB/页，pdata 长度要求为 FLASH_LOGICAL_PAGE_SIZE/2 个 uint16_t
uint8_t flash_write(uint32_t zone_index, uint32_t offset, const uint16_t *pdata)
{
    if ((zone_index >= FLASH_ZONE_MAX) || (pdata == NULL))
    {
        return 0;
    }
    // 必须页对齐，且不能越界分区
    if ((offset % FLASH_LOGICAL_PAGE_SIZE) != 0 ||
        (offset + FLASH_LOGICAL_PAGE_SIZE) > flash_zone[zone_index].size)
    {
        return 0;
    }

    int32_t page_index = flash_get_page_index(zone_index, offset);
    if (page_index < 0)
    {
        return 0;
    }

    return bsp_flash_write_page((uint32_t)page_index, (uint32_t*)pdata) ? 1 : 0;
}

// 整页读取：4KB/页，pdata 长度要求为 FLASH_LOGICAL_PAGE_SIZE/2 个 uint16_t
uint8_t flash_read(uint32_t zone_index, uint32_t offset, uint16_t *pdata)
{
    if ((zone_index >= FLASH_ZONE_MAX) || (pdata == NULL))
    {
        return 0;
    }
    // 必须页对齐，且不能越界分区
    if ((offset % FLASH_LOGICAL_PAGE_SIZE) != 0 ||
        (offset + FLASH_LOGICAL_PAGE_SIZE) > flash_zone[zone_index].size)
    {
        return 0;
    }

    int32_t page_index = flash_get_page_index(zone_index, offset);
    if (page_index < 0)
    {
        return 0;
    }

    bsp_flash_read_page((uint32_t)page_index, (uint32_t*)pdata);
    return 1;
}
