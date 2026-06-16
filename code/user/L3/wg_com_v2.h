#ifndef __WG_COM_V2_H
#define __WG_COM_V2_H

#include "stdint.h"
#include "my_math.h"
#include "bsp_usart.h"
#define LMT_MAP_SIZE (sizeof(lmt_map) / sizeof(lmt_map[0]))

#define WG_COM_V2_GET_DATA(user_data, wg_com_v2_data)                                          \
    do                                                                                         \
    {                                                                                          \
        uint16_t litend_uint16 = get_uint16((uint8_t *)&(wg_com_v2_data));                     \
        float unit = get_unit_for_addr((void *)&(wg_com_v2_data));                             \
        const wg_com_v2_data_lmt_map_t *lmt_map = get_lmt_for_addr((void *)&(wg_com_v2_data)); \
        if ((uint16_t)((user_data) / unit) != litend_uint16)                                   \
        {                                                                                      \
            if (lmt_map != NULL)                                                               \
            {                                                                                  \
                UP_DN_LMT(litend_uint16, lmt_map->up_lmt, lmt_map->dn_lmt);                    \
                set_uint16((uint8_t *)&(wg_com_v2_data), litend_uint16);                       \
            }                                                                                  \
            (user_data) = litend_uint16 * unit;                                                \
        }                                                                                      \
    } while (0)

#define WG_COM_V2_SET_DATA(user_data, wg_com_v2_data)                                          \
    do                                                                                         \
    {                                                                                          \
        float unit = get_unit_for_addr((void *)&(wg_com_v2_data));                             \
        const wg_com_v2_data_lmt_map_t *lmt_map = get_lmt_for_addr((void *)&(wg_com_v2_data)); \
        uint16_t act_data_temp = (uint16_t)((user_data) / unit + ((unit*5)/10.0f));            \
        if (lmt_map != NULL)                                                                   \
        {                                                                                      \
            UP_DN_LMT(act_data_temp, lmt_map->up_lmt, lmt_map->dn_lmt);                        \
        }                                                                                      \
        set_uint16((uint8_t *)&(wg_com_v2_data), act_data_temp);                               \
    } while (0)

#define WG_COM_V2_BUFFER_SIZE 256

#define WG_COM_V2_BROADCAST_ADDR 0xFF
#define WG_COM_V2_HOST_ADDR 0x01

#define WG_COM_V2_CMD_READ 0x03
#define WG_COM_V2_CMD_WRITE_DATA 0x06
#define WG_COM_V2_CMD_WRITE_STR 0x10

#define MODBUS_MIN_FRAME_LEN 5 // 地址(1) + 功能码(1) + 起始寄存器(2) + CRC(2)

#define USART0_DELAY_CONT   0xffff

#define DEFINE_ADDR_REGION(offset, var)             \
    {                                               \
        .start_addr = (offset),                     \
        .end_addr = (offset) + sizeof(var) / 2 - 1, \
        .data_ptr = &(var)}

typedef struct
{
    void *addr;
    float unit;
} realtime_data_unit_map_t;

typedef struct
{
    void *addr;
    uint16_t up_lmt;
    uint16_t dn_lmt;
} wg_com_v2_data_lmt_map_t;

// 地址区域描述结构体
typedef struct
{
    uint16_t start_addr;
    uint16_t end_addr;
    void *data_ptr; // 数据结构起始地址
} addr_region_t;

#pragma pack(1)

#define WG_COM_V2_PRUCUCT_INFO_ADDR 0x0
typedef struct
{
    uint16_t ProtocolVersion[2];   // 协议版本
    uint16_t ProductType[2];       // 产品类型
    uint16_t HardverVerzi[2];      // 硬件版本
    uint16_t SoftVersion[2];       // 软件版本
    uint16_t SnSerial[10];         // SN序列号
    uint16_t ProductName[10];      // 产品名称
    uint16_t Address;              // 地址
    uint16_t ApplicationScenarios; // 应用场景
    uint16_t CustomizationVersion; // 协议定制
    uint16_t MacAddress[10];       // mac地址
    uint16_t BtName;               // 设置蓝牙名称
} wg_com_v2_product_info_t;

#pragma pack()

float get_unit_for_addr(void *p);

const wg_com_v2_data_lmt_map_t *get_lmt_for_addr(void *p);

uint16_t get_uint16(uint8_t *p_data);
void set_uint16(uint8_t *p_data, uint16_t data);

extern wg_com_v2_product_info_t wg_com_v2_product_info;

typedef struct
{
    volatile uint8_t *dma_buffer;
    uint32_t buffer_size;
    uint32_t dma_channel;
    uint32_t *rx_cnt;
    uint8_t *is_rx_flag;
    uint32_t *timeout;
    const char *tag;
    usart_output_port_t USARTx;
} usart_dma_port_t;

#endif
