#include "bsp_flash.h"
#include "gd32f30x_fmc.h"
#include "string.h"

static uint32_t bsp_flash_get_page_addr(uint32_t page_index)
{
    if (page_index >= FLASH_LOGICAL_PAGE_COUNT)
    {
        return 0xFFFFFFFFU;
    }
    return FLASH_USER_BASE_ADDR + (FLASH_LOGICAL_PAGE_SIZE * page_index);
}

uint8_t bsp_flash_erase_page(uint32_t page_index)
{
    uint32_t addr = bsp_flash_get_page_addr(page_index);
    if (addr == 0xFFFFFFFFU)
        return 0;

    fmc_unlock();
    fmc_page_erase(addr);
    fmc_lock();

    return 1;
}

// 新增：按页写入（2KB/页 = 1024个uint16_t）
uint8_t bsp_flash_write_page(uint32_t page_index, const uint16_t *data)
{
    uint32_t base_addr = 0xFFFFFFFFU;
    if (data == NULL)
        return 0;

    base_addr = bsp_flash_get_page_addr(page_index);
    if (base_addr == 0xFFFFFFFFU)
        return 0;

    fmc_unlock();
    fmc_page_erase(base_addr);

    // 逐半字编程一整页
    for (uint32_t i = 0; i < (FLASH_LOGICAL_PAGE_SIZE / 2U); i++)
    {
        fmc_halfword_program(base_addr + (i * 2U), data[i]);
    }

    fmc_lock();
    return 1;
}

// 新增：按页读取（2KB/页 = 1024个uint16_t）
void bsp_flash_read_page(uint32_t page_index, uint16_t *data)
{
    uint32_t base_addr = 0xFFFFFFFFU;
    if (data == NULL)
        return;

    base_addr = bsp_flash_get_page_addr(page_index);
    if (base_addr == 0xFFFFFFFFU)
        return;

    for (uint32_t i = 0; i < (FLASH_LOGICAL_PAGE_SIZE / 2U); i++)
    {
        data[i] = *(volatile uint16_t *)(base_addr + (i * 2U));
    }
}
