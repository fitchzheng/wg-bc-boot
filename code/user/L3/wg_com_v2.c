#include "wg_com_v2.h"
#include "bsp_usart.h"
#include "section.h"
#include "stdint.h"
#include "string.h"
#include "bsp_dma.h"

extern uint32_t systemtime;

static uint8_t host_addr = WG_COM_V2_HOST_ADDR;

static uint8_t wg_com_tx_buffer[WG_COM_V2_BUFFER_SIZE];
static uint32_t wg_com_tx_buffer_cnt = 0;

static uint8_t wg_com_rx_buffer[WG_COM_V2_BUFFER_SIZE];
static uint32_t wg_com_rx_buffer_cnt = 0;

#define WG_COM_V2_INIT_U16(value) \
    ((uint16_t)(((((uint16_t)(value)) & 0x00FFU) << 8) | ((((uint16_t)(value)) & 0xFF00U) >> 8)))

wg_com_v2_product_info_t wg_com_v2_product_info = {
    .ProtocolVersion = {WG_COM_V2_INIT_U16(('V' << 8) | '1'), WG_COM_V2_INIT_U16(('0' << 8) | '0')},
    .ProductType = {WG_COM_V2_INIT_U16(0), WG_COM_V2_INIT_U16(5)},
    .HardverVerzi = {WG_COM_V2_INIT_U16(('V' << 8) | '1'), WG_COM_V2_INIT_U16(('0' << 8) | '0')},
    .SoftVersion = {WG_COM_V2_INIT_U16(('V' << 8) | '2'), WG_COM_V2_INIT_U16(('0' << 8) | '0')},
    .SnSerial = {
        WG_COM_V2_INIT_U16(('W' << 8) | 'G'),
        WG_COM_V2_INIT_U16(('0' << 8) | '5'),
        WG_COM_V2_INIT_U16(('-' << 8) | '2'),
        WG_COM_V2_INIT_U16(('0' << 8) | '2'),
        WG_COM_V2_INIT_U16(('5' << 8) | '0'),
        WG_COM_V2_INIT_U16(('7' << 8) | '2'),
        WG_COM_V2_INIT_U16(('2' << 8) | '-'),
        WG_COM_V2_INIT_U16(('0' << 8) | '0'),
        WG_COM_V2_INIT_U16(('0' << 8) | '0'),
        WG_COM_V2_INIT_U16(('0' << 8) | '0')},
    .ProductName = {
        WG_COM_V2_INIT_U16(('W' << 8) | 'G'),
        WG_COM_V2_INIT_U16(('-' << 8) | 'B'),
        WG_COM_V2_INIT_U16(('C' << 8) | '1'),
        WG_COM_V2_INIT_U16(('2' << 8) | '0'),
        WG_COM_V2_INIT_U16(('0' << 8) | 'M'),
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' ')},
    .Address = WG_COM_V2_INIT_U16(1),
    .ApplicationScenarios = WG_COM_V2_INIT_U16(0),
    .CustomizationVersion = WG_COM_V2_INIT_U16(0),
    .MacAddress = {
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' '),
        WG_COM_V2_INIT_U16((' ' << 8) | ' ')},
    .BtName = WG_COM_V2_INIT_U16(0),
};

static const realtime_data_unit_map_t unit_map[] = {
    {&wg_com_v2_product_info.Address, 1.0f},
};

static const wg_com_v2_data_lmt_map_t lmt_map[] = {
    {&wg_com_v2_product_info.Address, 147, 1},
};

float get_unit_for_addr(void *p)
{
    for (size_t i = 0; i < sizeof(unit_map) / sizeof(unit_map[0]); ++i)
    {
        if (unit_map[i].addr == p)
            return unit_map[i].unit;
    }
    return 1.0f; // default unit 1.0f
}

const wg_com_v2_data_lmt_map_t *get_lmt_for_addr(void *p)
{
    for (size_t i = 0; i < sizeof(lmt_map) / sizeof(lmt_map[0]); ++i)
    {
        if (lmt_map[i].addr == p)
            return &lmt_map[i];
    }
    return NULL;
}

// CRC check calculation
static uint16_t ModBusCRC16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF; // Initial value of CRC register

    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001; // XOR polynomial
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return (crc >> 8) | (crc << 8); // Swap bytes for little-endian
}

uint16_t get_uint16(uint8_t *p_data)
{
    return (uint16_t)(p_data[1] | (p_data[0] << 8));
}

void set_uint16(uint8_t *p_data, uint16_t data)
{
    p_data[0] = (uint8_t)((data >> 8) & 0x00FF);
    p_data[1] = (uint8_t)(data & 0x00FF);
}

// 地址区域注册表
static const addr_region_t addr_regions[] = {
    DEFINE_ADDR_REGION(WG_COM_V2_PRUCUCT_INFO_ADDR, wg_com_v2_product_info),
};

// 查找匹配的地址区域
static const addr_region_t *find_addr_region(uint16_t addr, uint16_t count)
{
    for (size_t i = 0; i < sizeof(addr_regions) / sizeof(addr_regions[0]); i++)
    {
        if (addr >= addr_regions[i].start_addr &&
            (addr + count - 1) <= addr_regions[i].end_addr)
        {
            return &addr_regions[i];
        }
    }
    return NULL;
}

// 各区域的具体读写实现
static uint8_t unified_read(uint16_t addr, uint16_t count, uint8_t *data)
{
    const addr_region_t *region = find_addr_region(addr, count);
    if (region == NULL)
        return 0;

    uint16_t offset = addr - region->start_addr;
    memcpy(data, (uint8_t *)region->data_ptr + offset * 2, count * 2);
    return 1;
}

static uint8_t unified_write(uint16_t addr, uint16_t count, const uint8_t *data)
{
    const addr_region_t *region = find_addr_region(addr, count);
    if (region == NULL)
        return 0;

    uint16_t offset = addr - region->start_addr;
    memcpy((uint8_t *)region->data_ptr + offset * 2, data, count * 2);
    return 1;
}

// 命令处理函数
static void handle_read_command(void)
{
    uint16_t start_addr = get_uint16(&wg_com_rx_buffer[2]);
    uint16_t reg_count = get_uint16(&wg_com_rx_buffer[4]);

    wg_com_tx_buffer[0] = host_addr;          // 从机地址
    wg_com_tx_buffer[1] = WG_COM_V2_CMD_READ; // 功能码

    if (unified_read(start_addr, reg_count, &wg_com_tx_buffer[3]))
    {
        wg_com_tx_buffer[2] = reg_count * 2; // 字节数
        uint16_t crc = ModBusCRC16(wg_com_tx_buffer, 3 + reg_count * 2);
        set_uint16(&wg_com_tx_buffer[3 + reg_count * 2], crc);
        wg_com_tx_buffer_cnt = 5 + reg_count * 2;
        return;
    }

    // 错误处理
    wg_com_tx_buffer[1] |= 0x80; // 设置错误标志
    wg_com_tx_buffer[2] = 0x02;  // 非法数据地址
    uint16_t crc = ModBusCRC16(wg_com_tx_buffer, 3);
    set_uint16(&wg_com_tx_buffer[3], crc);
    wg_com_tx_buffer_cnt = 5;
}

static void handle_write_data_command(void)
{
    uint16_t reg_addr = get_uint16(&wg_com_rx_buffer[2]);
    uint16_t reg_value = get_uint16(&wg_com_rx_buffer[4]);

    memcpy(wg_com_tx_buffer, wg_com_rx_buffer, 6); // 回显

    uint8_t data[2];
    set_uint16(data, reg_value);

    if (unified_write(reg_addr, 1, data))
    {
        uint16_t crc = ModBusCRC16(wg_com_tx_buffer, 6);
        set_uint16(&wg_com_tx_buffer[6], crc);
        wg_com_tx_buffer_cnt = 8;
        return;
    }

    // 错误处理
    wg_com_tx_buffer[1] |= 0x80;
    wg_com_tx_buffer[2] = 0x02;
    uint16_t crc = ModBusCRC16(wg_com_tx_buffer, 3);
    set_uint16(&wg_com_tx_buffer[3], crc);
    wg_com_tx_buffer_cnt = 5;
}

static void handle_write_str_command(void)
{
    uint16_t start_addr = get_uint16(&wg_com_rx_buffer[2]);
    uint16_t reg_count = get_uint16(&wg_com_rx_buffer[4]);
    uint8_t byte_count = wg_com_rx_buffer[6];

    if (byte_count != reg_count * 2)
    {
        wg_com_tx_buffer[0] = host_addr;
        wg_com_tx_buffer[1] = WG_COM_V2_CMD_WRITE_STR | 0x80;
        wg_com_tx_buffer[2] = 0x03; // 非法数据值
        uint16_t crc = ModBusCRC16(wg_com_tx_buffer, 3);
        set_uint16(&wg_com_tx_buffer[3], crc);
        wg_com_tx_buffer_cnt = 5;
        return;
    }

    memcpy(wg_com_tx_buffer, wg_com_rx_buffer, 6); // 回显

    if (unified_write(start_addr, reg_count, &wg_com_rx_buffer[7]))
    {
        uint16_t crc = ModBusCRC16(wg_com_tx_buffer, 6);
        set_uint16(&wg_com_tx_buffer[6], crc);
        wg_com_tx_buffer_cnt = 8;
        return;
    }

    // 错误处理
    wg_com_tx_buffer[1] |= 0x80;
    wg_com_tx_buffer[2] = 0x02;
    uint16_t crc = ModBusCRC16(wg_com_tx_buffer, 3);
    set_uint16(&wg_com_tx_buffer[3], crc);
    wg_com_tx_buffer_cnt = 5;
}

static void process_command(void)
{
    memset(wg_com_tx_buffer, 0, sizeof(wg_com_tx_buffer));
    wg_com_tx_buffer_cnt = 0;

    uint8_t cmd = wg_com_rx_buffer[1];
    switch (cmd)
    {
    case WG_COM_V2_CMD_READ:
        handle_read_command();
        break;
    case WG_COM_V2_CMD_WRITE_DATA:
        handle_write_data_command();
        break;
    case WG_COM_V2_CMD_WRITE_STR:
        handle_write_str_command();
        break;
    default:
        break;
    }
}

#include "gpio.h"

// 安全增加计数器（防止溢出）
static inline void safe_increment(uint32_t *counter, uint32_t max)
{
    if (*counter < max)
        (*counter)++;
}
static uint32_t usart0_delay = 0;
void process_usart_dma_input(const usart_dma_port_t *port)
{
    if((USART_GetStatus(CM_USART1, USART_FLAG_FRAME_ERR) == 1) && (port->USARTx == OUTPUT_USART0))
    {
        USART_SetFirstBit(CM_USART1, USART_CR1_CFE);
        CM_USART1->CR1 |= USART_CR1_CFE;
    }
    while (*(port->rx_cnt) != USARTX_RX_DMA_CNT(port->dma_channel))
    {
        uint8_t rx_data = port->dma_buffer[port->buffer_size - *(port->rx_cnt)];
        (*(port->rx_cnt))--;
        (*(port->rx_cnt)) = (*(port->rx_cnt) == 0) ? port->buffer_size : (*(port->rx_cnt));

        wg_com_rx_buffer[wg_com_rx_buffer_cnt++] = rx_data;
        if (wg_com_rx_buffer_cnt >= WG_COM_V2_BUFFER_SIZE)
        {
            wg_com_rx_buffer_cnt = 0;
        }

        *(port->is_rx_flag) = 1; 
        *(port->timeout) = systemtime;
    }

    if (((*(port->timeout)+20) < systemtime) && (*(port->is_rx_flag)))
    {
        *(port->is_rx_flag) = 0;

        if (wg_com_rx_buffer_cnt < MODBUS_MIN_FRAME_LEN)
        {
            memset(wg_com_rx_buffer, 0, sizeof(wg_com_rx_buffer));
            wg_com_rx_buffer_cnt = 0;
            return;
        }

        uint16_t *rx_crc = (uint16_t *)&wg_com_rx_buffer[wg_com_rx_buffer_cnt - 2];
        uint16_t cal_crc = ModBusCRC16(wg_com_rx_buffer, wg_com_rx_buffer_cnt - 2);
        set_uint16((uint8_t *)&cal_crc, cal_crc);
        WG_COM_V2_GET_DATA(host_addr, wg_com_v2_product_info.Address); 
        if ((*rx_crc == cal_crc) &&
            ((wg_com_rx_buffer[0] == host_addr) ||
             (wg_com_rx_buffer[0] == WG_COM_V2_BROADCAST_ADDR)))
        {
            process_command();
                   
            if (wg_com_tx_buffer_cnt != 0)
            {
                if(port->USARTx == OUTPUT_USART0)
                {
                    gpio_set_re(1);
                    for (uint32_t i = 0; i < wg_com_tx_buffer_cnt; i++)
                    {
                        bsp_usart_putc(port->USARTx, wg_com_tx_buffer[i]);
                    }
                    usart0_delay = 0;
                    while(USART_GetStatus(CM_USART1, USART_FLAG_TX_CPLT) == 0)
                    {
                        safe_increment(&usart0_delay,USART0_DELAY_CONT);
                        if(usart0_delay >= USART0_DELAY_CONT)
                        {
                            break;
                        }
                    }
                    gpio_set_re(0);
                }
                else
                {
                    for (uint32_t i = 0; i < wg_com_tx_buffer_cnt; i++)
                    {
                        bsp_usart_putc(port->USARTx, wg_com_tx_buffer[i]);
                    }
                }
            }
        }

        memset(wg_com_rx_buffer, 0, sizeof(wg_com_rx_buffer));
        wg_com_rx_buffer_cnt = 0;
    }
}

void wg_com_v2_run(void)
{
    static uint32_t rx2_data_cnt = USART2_RX_BUFFER_SIZE;
    static uint8_t is_rx2 = 0;
    static uint32_t timeout2 = 0;

    static uint32_t rx0_data_cnt = USART0_RX_BUFFER_SIZE;
    static uint8_t is_rx0 = 0;
    static uint32_t timeout0 = 0;

    static const usart_dma_port_t usart2_port = {
        .dma_buffer = usart2_rx_buffer,
        .buffer_size = USART2_RX_BUFFER_SIZE,
#ifdef HC32F334
        .dma_channel = DMA_CH3,
#else
        .dma_channel = DMA_CH2,
#endif
        .rx_cnt = &rx2_data_cnt,
        .is_rx_flag = &is_rx2,
        .timeout = &timeout2,
        .tag = "usart2",
        .USARTx = OUTPUT_USART2};

    
    static const usart_dma_port_t usart0_port = {
        .dma_buffer = usart0_rx_buffer,
        .buffer_size = USART0_RX_BUFFER_SIZE,
#ifdef HC32F334
        .dma_channel = DMA_CH1,
#else
        .dma_channel = DMA_CH4,
#endif
        .rx_cnt = &rx0_data_cnt,
        .is_rx_flag = &is_rx0,
        .timeout = &timeout0,
        .tag = "usart0",
        .USARTx = OUTPUT_USART0};
 
    process_usart_dma_input(&usart2_port);
    process_usart_dma_input(&usart0_port);
}

REG_TASK(1, wg_com_v2_run)
