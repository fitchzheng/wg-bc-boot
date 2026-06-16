#include "bsp_flash.h"

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
   
    EFM_REG_Unlock();
    EFM_FWMC_Cmd(ENABLE);
    EFM_SingleSectorOperateCmd(page_index, ENABLE);
    EFM_SectorErase(addr);
    EFM_SingleSectorOperateCmd(page_index, DISABLE);
    EFM_REG_Lock();
    
    return 1;
}

// 新增：按页写入（4KB/页 = 1024个uint32_t）
//注意，HC32F334只支持WORD写入，即32bit/4BYTE，不支持半字写入
uint8_t bsp_flash_write_page(uint32_t page_index, const uint32_t *data)
{
    uint32_t base_addr = 0xFFFFFFFFU;
    if (data == NULL)
        return 0;

    base_addr = bsp_flash_get_page_addr(page_index);
    if (base_addr == 0xFFFFFFFFU)
        return 0;

    EFM_REG_Unlock();

    EFM_FWMC_Cmd(ENABLE);
    
    (void)EFM_SingleSectorOperateCmd(page_index, ENABLE);

    EFM_SectorErase(base_addr);

    // 整字编程一整页
    for (uint32_t i = 0; i < (FLASH_LOGICAL_PAGE_SIZE / 4U); i++)
    {
        EFM_ProgramWord(base_addr + (i * 4U), data[i]);
    }

    (void)EFM_SingleSectorOperateCmd(page_index, DISABLE);
    EFM_REG_Lock();
    return 1;
}


// 新增：按页读取（4KB/页 = 1024个uint32_t）
//HC32F334的读取可以不用对齐地址
void bsp_flash_read_page(uint32_t page_index, uint32_t *data)
{
    uint32_t base_addr = 0xFFFFFFFFU;
    if (data == NULL)
        return;

    base_addr = bsp_flash_get_page_addr(page_index);
    if (base_addr == 0xFFFFFFFFU)
        return;

    for (uint32_t i = 0; i < (FLASH_LOGICAL_PAGE_SIZE / 4U); i++)
    {
        data[i] = *(volatile uint32_t *)(base_addr + (i * 4U));
    }
}
