// ========================= ymodem.c =========================
#include "ymodem.h"
#include <string.h>
#include "bsp_usart.h"
#include "bsp_dma.h"
#include "flash.h"
#include "gpio.h"
#include "section.h"
#include "bsp_dma.h"
#include "wg_com_v2.h"
#include "can_ota.h"

//static uint32_t counter = 0;
static uint8_t Stack_Addr[4];

// 安全增加计数器（防止溢出）
static inline void safe_increment(uint32_t *counter, uint32_t max)
{
    if (*counter < max)
        (*counter)++;
}

enum
{
    YMODEM_STA_INIT,
    YMODEM_STA_IDLE,
    YMODEM_STA_CHK_FIRST,
    YMODEM_STA_WAIT_DATA,
    YMODEM_STA_PROCESS_DATA,
    YMODEM_STA_RECV_END,
    YMODEM_STA_RECV_CP_DATA,
    YMODEM_STA_RECV_CP_END,
    YMODEM_STA_RECV_WRITE_FLASH,
    YMODEM_STA_WAIT_REPLY,
    YMODEM_STA_END_DATA,
    YMODEM_STA_RESET,
};

uint32_t ymodem_state = YMODEM_STA_INIT;
uint32_t ymodem_state_next = YMODEM_STA_INIT;

uint8_t usart_link = 0;

// 声明 DMA 环形缓冲（使用 volatile 防止优化丢读）
extern volatile uint8_t usart2_rx_buffer[];
extern volatile uint8_t usart0_rx_buffer[];
extern uint32_t dma_transfer_number_get(uint32_t periph, uint8_t channel);
ymodem_soh_first_parsed_t ymodem_soh_first_parsed;
ymodem_frame_soh_data_t ymodem_frame_soh_data;
ymodem_frame_stx_t ymodem_frame_stx;
static uint16_t flash_page_4k = 0;
static uint32_t code_size_byte = 0;

#define IAP_META_MAGIC      0x4D504149UL
#define IAP_META_PENDING    0x444E4550UL
#define IAP_META_STARTED    0x54525453UL
#define IAP_META_VALID      0x444C4156UL
#define IAP_META_OFFSET     256U
#define FACTORY_BACKUP_MAGIC 0x42434647UL

typedef struct
{
    uint32_t magic;
    uint32_t status;
    uint16_t header_crc;
    uint16_t reserved;
} iap_meta_t;

// 空闲判定阈值：连续空闲 50ms 视为当前帧接收完成
#undef YMODEM_IDLE_GAP_MS
#define YMODEM_IDLE_GAP_MS 50U

// 回复发送队列（按 200ms 间隔发送）
static uint8_t reply_fifo[4];
static uint8_t reply_head[2], reply_tail[2], reply_len[2];
static uint16_t reply_wait_ms[2];

static uint8_t is_recv_cplt[2];

static inline uint8_t ymodem_reply_port_is_valid(uint8_t port_com)
{
    return (port_com < RX_MAX_COM) ? 1U : 0U;
}

static inline uint8_t ymodem_is_can_ota_link(void)
{
    return (usart_link == RX_CAN_OTA_COM) ? 1U : 0U;
}

static uint32_t swap_endian_u32(uint32_t in)
{
    return ((in & 0xFF000000) >> 24) | // Move byte 3 to byte 0
           ((in & 0x00FF0000) >> 8) |  // Move byte 2 to byte 1
           ((in & 0x0000FF00) << 8) |  // Move byte 1 to byte 2
           ((in & 0x000000FF) << 24);  // Move byte 0 to byte 3
}

// 入队一个待回复字节
static inline void ymodem_queue_reply(uint8_t c,uint8_t port_com)
{
    if (ymodem_reply_port_is_valid(port_com) == 0U)
    {
        return;
    }

    if (reply_len[port_com] < sizeof(reply_fifo))
    {
        reply_fifo[reply_tail[port_com]] = c;
        reply_tail[port_com] = (uint8_t)((reply_tail[port_com] + 1) % sizeof(reply_fifo));
        reply_len[port_com]++;
    }
}
static uint32_t ymodem_usart0_delay = 0;
// 实际发送一个待回复字节（printf），并启动下一次 50ms 间隔计时
static inline void ymodem_tx_reply_tick(uint8_t port_com)
{
    if (ymodem_reply_port_is_valid(port_com) == 0U)
    {
        return;
    }

    if (reply_len[port_com] == 0)
        return;

    if (reply_wait_ms[port_com] > 0)
    {
        reply_wait_ms[port_com]--;
        return;
    }

    // 发送队首
    uint8_t c = reply_fifo[reply_head[port_com]];
    reply_head[port_com] = (uint8_t)((reply_head[port_com] + 1) % sizeof(reply_fifo));
    reply_len[port_com]--;

    // 发送
    if (port_com == OUTPUT_USART0)
    {
        gpio_set_re(1);
        bsp_usart_putc(OUTPUT_USART0, c);
        ymodem_usart0_delay = 0;
        while(USART_GetStatus(CM_USART1, USART_FLAG_TX_CPLT) == 0)
        {
            safe_increment(&ymodem_usart0_delay,USART0_DELAY_CONT);
            if(ymodem_usart0_delay >= USART0_DELAY_CONT)
            {
                break;
            }
        }
        gpio_set_re(0);
    }
    else if (port_com == OUTPUT_USART2)
    {
        bsp_usart_putc(OUTPUT_USART2, c);
    }

    // 设置下一条回复的最小间隔 50ms
    reply_wait_ms[port_com] = 200;
}

// 新增：ACK/NAK 统一使用队列发送
static inline void ymodem_send_ack(uint8_t port_com)
{
    ymodem_queue_reply(YMODEM_ACK,port_com);
    // 根据需要重置重试/超时计数
    // ymodem_retry_cnt = 0;
    // ymodem_update_time_dn_cnt = 0;
}

static inline void ymodem_send_nak(uint8_t port_com)
{
    ymodem_queue_reply(YMODEM_NAK,port_com);
    // if (ymodem_retry_cnt < 0xFF) ymodem_retry_cnt++;
    // ymodem_update_time_dn_cnt = 0;
}

// ========== 补回缺失的静态全局变量与前置声明 ==========
static uint8_t recv_buffer[RX_MAX_COM][YMODEM_BUFFER_SIZE];
static uint16_t recv_index[RX_MAX_COM];

static uint8_t code_buffer[CODE_BUFFER_PAGE_COUNT][CODE_BUFFER_PAGE_SIZE];
static uint8_t page[FLASH_LOGICAL_PAGE_SIZE];
static uint8_t can_iap_page[FLASH_LOGICAL_PAGE_SIZE];
//static uint8_t code_bt_chunk_cnt = 0;
//static uint8_t expect_bt_pn = 0;
//static uint8_t expect_pkt_pn = 1;
//static uint32_t flash_index = 0;
// 新增：记录当前2KB页的前半/后半（0:前半 1:后半）
//static uint8_t code_half = 0;

//static uint8_t is_updating = 0;
//static uint8_t app_is_exist = 0;
static uint32_t app_stack = 0;
static uint32_t app_reset = 0;
static uint8_t  updating_flag = 0;
static uint8_t  update_started = 0;
static uint8_t  pending_can_fallback = 0;
static uint8_t  expect_pn = 1;
static uint8_t  expect_bt_pn = 0;
static uint8_t  last_pn = 0;
static uint8_t  last_bt_pn = 0;
static uint8_t  ymodem_flash_error = 0;
static uint8_t  recv_byte_is_ok = 0;
static uint32_t image_size_byte = 0;
static uint32_t image_bytes_received = 0;
static uint32_t image_crc32_calc = 0xFFFFFFFFUL;
static uint32_t image_crc32_expected = 0xFFFFFFFFUL;
static uint32_t can_iap_page_base = 0;
static uint32_t can_iap_page_used = 0;
static uint32_t can_iap_received = 0;
static uint8_t  can_iap_active = 0;
//static uint32_t ymodem_update_time_dn_cnt = 0;
//static uint8_t ymodem_retry_cnt = 0;

//static uint16_t pn_cnt = 0;
//static uint16_t btpn_cnt = 0;

// 前置声明，避免隐式声明
static void boot_jump_to_app(void);

// === CRC16 Modbus 实现（高位在前） ===
static uint16_t ymodem_crc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return (uint16_t)((crc >> 8) | (crc << 8));
}

static uint32_t ymodem_read_le32(const uint8_t *data)
{
    return ((uint32_t)data[0]) |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static uint32_t ymodem_crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if ((crc & 1U) != 0U)
            {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static void ymodem_reset_transfer_state(void)
{
    expect_pn = 1;
    expect_bt_pn = 0;
    last_pn = 0;
    last_bt_pn = 0;
    recv_byte_is_ok = 0;
    ymodem_flash_error = 0;
    image_size_byte = code_size_byte;
    image_bytes_received = 0;
    image_crc32_calc = 0xFFFFFFFFUL;
    image_crc32_expected = ymodem_read_le32(&ymodem_soh_first_parsed.padding[YMODEM_INFO_IMAGE_CRC32_OFFSET]);
    memset(&code_buffer[0][0], 0xFF, sizeof(code_buffer));
}

static void ymodem_advance_expected_packet(void)
{
    last_pn = expect_pn;
    last_bt_pn = expect_bt_pn;

    expect_bt_pn++;
    if (expect_bt_pn > YMODEM_BT_BTPN_MAX)
    {
        expect_bt_pn = 0;
        expect_pn++;
    }
}

static uint8_t ymodem_is_duplicate_packet(void)
{
    return ((ymodem_frame_stx.pn == last_pn) &&
            (ymodem_frame_stx.bt_pn == last_bt_pn)) ? 1 : 0;
}

static uint8_t ymodem_verify_flash_page(uint32_t zone_index, uint32_t offset, const uint8_t *expected)
{
    int32_t page_index = flash_get_page_index(zone_index, offset);
    const uint8_t *flash_data;

    if (page_index < 0)
    {
        return 0;
    }

    flash_data = (const uint8_t *)(FLASH_USER_BASE_ADDR + ((uint32_t)page_index * FLASH_LOGICAL_PAGE_SIZE));
    return (memcmp(flash_data, expected, FLASH_LOGICAL_PAGE_SIZE) == 0) ? 1 : 0;
}

static uint8_t ymodem_verify_app_crc32(void)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t remain = image_size_byte;
    uint32_t offset = 0;

    while (remain > 0U)
    {
        uint32_t chunk = (remain > FLASH_LOGICAL_PAGE_SIZE) ? FLASH_LOGICAL_PAGE_SIZE : remain;
        if (flash_read(FLASH_APP, offset, (uint16_t *)&page) == 0)
        {
            return 0;
        }
        crc = ymodem_crc32_update(crc, &page[0], chunk);
        remain -= chunk;
        offset += FLASH_LOGICAL_PAGE_SIZE;
    }

    crc ^= 0xFFFFFFFFUL;
    return (crc == image_crc32_expected) ? 1 : 0;
}

static uint8_t ymodem_received_crc32_is_ok(void)
{
    return ((image_crc32_calc ^ 0xFFFFFFFFUL) == image_crc32_expected) ? 1 : 0;
}

static uint8_t ymodem_load_meta_image_info(void)
{
    code_size_byte = swap_endian_u32(ymodem_soh_first_parsed.file_size);
    image_size_byte = code_size_byte;
    image_crc32_expected = ymodem_read_le32(&ymodem_soh_first_parsed.padding[YMODEM_INFO_IMAGE_CRC32_OFFSET]);
    image_crc32_calc = 0xFFFFFFFFUL;
    image_bytes_received = 0U;

    if ((image_size_byte == 0U) ||
        (image_size_byte > flash_zone[FLASH_APP].size) ||
        (image_crc32_expected == 0U) ||
        (image_crc32_expected == 0xFFFFFFFFUL))
    {
        return 0U;
    }

    return 1U;
}

static iap_meta_t *ymodem_get_meta(void)
{
    return (iap_meta_t *)&page[IAP_META_OFFSET];
}

static uint8_t ymodem_meta_header_is_valid(void)
{
    iap_meta_t *meta = ymodem_get_meta();
    uint16_t crc_data = ymodem_crc16((const uint8_t *)&ymodem_soh_first_parsed, sizeof(ymodem_soh_first_parsed) - 2);

    if ((meta->magic != IAP_META_MAGIC) ||
        (meta->header_crc != crc_data))
    {
        return 0;
    }

    if ((meta->status != IAP_META_PENDING) &&
        (meta->status != IAP_META_STARTED) &&
        (meta->status != IAP_META_VALID))
    {
        return 0;
    }

    return 1;
}

static uint8_t ymodem_meta_link_is_valid(void)
{
    return ((usart_link <= OUTPUT_USART2) || (usart_link == RX_CAN_OTA_COM)) ? 1U : 0U;
}

static uint8_t ymodem_legacy_start_header_is_valid(void)
{
    uint16_t crc_data;
    uint32_t legacy_size;
    uint32_t legacy_crc32;

    if ((ymodem_soh_first_parsed.header != YMODEM_SOH) ||
        (ymodem_soh_first_parsed.pn != 0U) ||
        (ymodem_soh_first_parsed.xpn != 0xFFU) ||
        (usart_link > OUTPUT_USART2))
    {
        return 0U;
    }

    crc_data = ymodem_crc16((const uint8_t *)&ymodem_soh_first_parsed, sizeof(ymodem_soh_first_parsed) - 2);
    if (crc_data != (((uint16_t)ymodem_soh_first_parsed.crc_high << 8) | ymodem_soh_first_parsed.crc_low))
    {
        return 0U;
    }

    legacy_size = swap_endian_u32(ymodem_soh_first_parsed.file_size);
    legacy_crc32 = ymodem_read_le32(&ymodem_soh_first_parsed.padding[YMODEM_INFO_IMAGE_CRC32_OFFSET]);
    if ((legacy_size == 0U) ||
        (legacy_size > flash_zone[FLASH_APP].size) ||
        (legacy_crc32 == 0U) ||
        (legacy_crc32 == 0xFFFFFFFFUL))
    {
        return 0U;
    }

    return 1U;
}

static uint8_t ymodem_write_meta_status(uint32_t status)
{
    iap_meta_t meta;
    uint16_t crc_data = ymodem_crc16((const uint8_t *)&ymodem_soh_first_parsed, sizeof(ymodem_soh_first_parsed) - 2);

    if (usart_link <= OUTPUT_USART2)
    {
        ymodem_soh_first_parsed.link_id = usart_link;
    }
    crc_data = ymodem_crc16((const uint8_t *)&ymodem_soh_first_parsed, sizeof(ymodem_soh_first_parsed) - 2);

    memset(&page, 0xFF, sizeof(page));
    memcpy(&page[0], &ymodem_soh_first_parsed, sizeof(ymodem_soh_first_parsed));
    meta.magic = IAP_META_MAGIC;
    meta.status = status;
    meta.header_crc = crc_data;
    meta.reserved = 0xFFFFU;
    memcpy(&page[IAP_META_OFFSET], &meta, sizeof(meta));
    if (flash_write(FLASH_UPDATE, 0, (uint16_t *)&page) == 0)
    {
        return 0;
    }
    return ymodem_verify_flash_page(FLASH_UPDATE, 0, &page[0]);
}

static uint8_t ymodem_clear_meta(void)
{
    memset(&page, 0xFF, sizeof(page));
      if (flash_write(FLASH_UPDATE, 0, (uint16_t *)&page) == 0U)
    {
        return 0U;
    }
    return ymodem_verify_flash_page(FLASH_UPDATE, 0, &page[0]);
}

static uint8_t ymodem_update_page_has_data(void)
{
    for (uint32_t i = 0U; i < FLASH_LOGICAL_PAGE_SIZE; i++)
    {
        if (page[i] != 0xFFU)
        {
            return 1U;
        }
    }

    return 0U;
}

static uint8_t ymodem_update_page_is_factory_backup(void)
{
    uint32_t magic = 0xFFFFFFFFUL;

    memcpy(&magic, &page[0], sizeof(magic));
    return (magic == FACTORY_BACKUP_MAGIC) ? 1U : 0U;
}

static inline void ymodem_reset_rx(uint8_t rx_com)
{
    recv_index[rx_com] = 0;
    memset(recv_buffer[rx_com], 0, sizeof(recv_buffer[rx_com]));
}

static void ymodem_chk_stack(void)
{
    app_stack = *(uint32_t *)APP_START_ADDR;
//    app_is_exist = ((app_stack & 0x2FFE0000U) == 0x20000000U) ? 1 : 0;
}

static uint8_t ymodem_app_vector_is_valid(void)
{
    app_stack = *(uint32_t *)APP_START_ADDR;
    app_reset = *(uint32_t *)(APP_START_ADDR + 4);

    return ((app_stack > 0x1FFFC000) &&
            (app_stack < (0x1FFFC000 + 0x8000)) &&
            ((app_reset & 0x00000001U) != 0U) &&
            ((app_reset & 0xFFFFFFFEU) >= APP_START_ADDR) &&
            ((app_reset & 0xFFFFFFFEU) <= APP_END_ADDR)) ? 1U : 0U;
}

int ymodem_recv_byte(uint8_t byte,uint8_t rx_com)
{
    if (recv_index[rx_com] >= sizeof(recv_buffer[rx_com]))
    {
        recv_index[rx_com] = 0;
        return -1;
    }
    recv_buffer[rx_com][recv_index[rx_com]++] = byte;
    return (int)recv_index[rx_com];
}

#define EXAMPLE_PERIPH_WP (LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                       LL_PERIPH_EFM)
static void boot_jump_to_app(void)
{

    if (ymodem_app_vector_is_valid() != 0U)
    {
        USART_DeInit(USART1_PORT);
        USART_DeInit(USART2_PORT);
        FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_USART1, DISABLE);
        FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_USART2, DISABLE);
        DMA_Cmd(CM_DMA, DISABLE);
        DMA_ChCmd(USART1_RX_DMA_UNIT, USART1_RX_DMA_CH, DISABLE); //使能DMA通道
        DMA_ChCmd(USART2_RX_DMA_UNIT, USART2_RX_DMA_CH, DISABLE); //使能DMA通道
        GPIO_SetFunc(USART1_TX_PORT,USART1_TX_PIN , USART1_TX_FUNC);
        GPIO_SetFunc(USART1_RX_PORT,USART1_RX_PIN , USART1_RX_FUNC);
        GPIO_SetFunc(USART2_TX_PORT,USART2_TX_PIN , USART2_TX_FUNC);
        GPIO_SetFunc(USART2_RX_PORT,USART2_RX_PIN , USART2_RX_FUNC);
        SysTick->CTRL  = 0UL;
        SysTick->LOAD  = 0UL;
        SysTick->VAL   = 0UL;
        LL_PERIPH_WP(EXAMPLE_PERIPH_WP);
        __set_MSP(app_stack);
        void (*app_reset_handler)(void) = (void (*)(void))app_reset;
        app_reset_handler();
    }
    else
    {
        // APP is missing or invalid: stay in BOOT and accept direct CAN OTA.
        boot_iap_can_prepare_restart();
    }
}

void ymodem_init(void)
{
    iap_meta_t meta;
    uint8_t saved_link;
    ymodem_reset_rx(OUTPUT_USART0);
    ymodem_reset_rx(OUTPUT_USART2);
    ymodem_chk_stack();
    flash_read(FLASH_UPDATE, 0, (uint16_t *)&page);
    memcpy(&ymodem_soh_first_parsed, &page, sizeof(ymodem_soh_first_parsed));
    memcpy(&meta, ymodem_get_meta(), sizeof(meta));
    usart_link = ymodem_soh_first_parsed.link_id;
    saved_link = usart_link;

    if ((ymodem_meta_link_is_valid() != 0U) &&
        (ymodem_meta_header_is_valid()))
    {
        if (meta.status == IAP_META_VALID)
        {
            if ((ymodem_load_meta_image_info() != 0U) &&
                (ymodem_verify_app_crc32() != 0U))
            {
                boot_jump_to_app();
            }
            else
            {
                ymodem_clear_meta();
                ymodem_reset_rx(OUTPUT_USART0);
                ymodem_reset_rx(OUTPUT_USART2);
                boot_iap_can_prepare_restart();
            }
            return;
        }

        ymodem_clear_meta();
        ymodem_reset_rx(OUTPUT_USART0);
        ymodem_reset_rx(OUTPUT_USART2);
        updating_flag = 0;
        update_started = 0;
        pending_can_fallback = 0;

        if (saved_link == RX_CAN_OTA_COM)
        {
            boot_iap_can_prepare_restart();
        }
        else
        {
            usart_link = 0;
            ymodem_state = YMODEM_STA_WAIT_DATA;
        }
        return;
    }

    if (ymodem_update_page_has_data() != 0U)
    {
        if (ymodem_update_page_is_factory_backup() != 0U)
        {
            usart_link = 0;
            updating_flag = 0;
            update_started = 0;
            pending_can_fallback = 0;
            boot_jump_to_app();
            return;
        }

        if (ymodem_legacy_start_header_is_valid() != 0U)
        {
            ymodem_reset_rx(OUTPUT_USART0);
            ymodem_reset_rx(OUTPUT_USART2);
            updating_flag = 1;
            update_started = 0;
            pending_can_fallback = 0;
            ymodem_state = YMODEM_STA_CHK_FIRST;
            return;
        }

        ymodem_clear_meta();
        ymodem_reset_rx(OUTPUT_USART0);
        ymodem_reset_rx(OUTPUT_USART2);
        boot_iap_can_prepare_restart();
        update_started = 0;
        pending_can_fallback = 0;
        return;
    }

    usart_link = 0;
    updating_flag = 0;
    update_started = 0;
    pending_can_fallback = 0;
    boot_jump_to_app();
}

void ymodem_process_usart_dma_input(const ymodem_usart_dma_port_t *port)
{
    uint32_t cndtr = USARTX_RX_DMA_CNT(port->dma_channel);
    uint16_t wr_idx = (uint16_t)(port->buffer_size - cndtr);

    if(wr_idx >= port->buffer_size)
    {
        wr_idx -= port->buffer_size;
    }

    while ((*(port->rd_idx)) != wr_idx)
    {
        uint8_t byte = port->dma_buffer[(*(port->rd_idx))];
        (*(port->rd_idx))++;
        if ((*(port->rd_idx)) >= port->buffer_size)
            (*(port->rd_idx)) = 0;

        (*(port->overflow)) = ymodem_recv_byte(byte,port->USARTx);
        (*(port->idle_ms))  = YMODEM_IDLE_GAP_MS;
    }

    if ((*(port->overflow)) == -1)
    {
        ymodem_send_nak(port->USARTx);
        ymodem_reset_rx(port->USARTx);
        (*(port->overflow)) = 0;
        (*(port->idle_ms)) = 0;
    }

    if (*(port->idle_ms))
    {
        (*(port->idle_ms))--;
    }
    
    if (recv_index[port->USARTx] != 0 && (*(port->idle_ms)) == 0)
    {
        is_recv_cplt[port->USARTx] = 1;
    }
    
    // 统一在这里按节流间隔发送待回复字节
    ymodem_tx_reply_tick(port->USARTx);
}

void ymodem_recv_task(void)
{
    // USART2部分
    static uint16_t rd_idx2 = 0;
    static int overflow2 = 0;
    static uint16_t idle2_ms = 0;

    // USART0部分
    static uint16_t rd_idx0 = 0;
    static int overflow0 = 0;
    static uint16_t idle0_ms = 0;

    static const ymodem_usart_dma_port_t usart2_port = {
        .dma_buffer = usart2_rx_buffer,
        .buffer_size = USART2_RX_BUFFER_SIZE,
        .dma_channel = DMA_CH3,
        .rd_idx = &rd_idx2,
        .overflow = &overflow2,
        .idle_ms = &idle2_ms,
        .tag = "usart2",
        .USARTx = OUTPUT_USART2};
    
    static const ymodem_usart_dma_port_t usart0_port = {
        .dma_buffer = usart0_rx_buffer,
        .buffer_size = USART0_RX_BUFFER_SIZE,
        .dma_channel = DMA_CH1,
        .rd_idx = &rd_idx0,
        .overflow = &overflow0,
        .idle_ms = &idle0_ms,
        .tag = "usart0",
        .USARTx = OUTPUT_USART0};

    ymodem_process_usart_dma_input(&usart2_port);
    ymodem_process_usart_dma_input(&usart0_port);
}

REG_TASK(1, ymodem_recv_task)

static void ymodem_chk_first(void)
{
    uint16_t crc_data = ymodem_crc16((const uint8_t *)&ymodem_soh_first_parsed, sizeof(ymodem_soh_first_parsed) - 2);
    if (crc_data == ((uint16_t)ymodem_soh_first_parsed.crc_high << 8 | ymodem_soh_first_parsed.crc_low))
    {
        // CRC 校验通过，准备接收数据
        code_size_byte = swap_endian_u32(ymodem_soh_first_parsed.file_size);
        if ((code_size_byte == 0U) ||
            (code_size_byte > flash_zone[FLASH_APP].size))
        {
            ymodem_state_next = YMODEM_STA_WAIT_DATA;
            ymodem_state = YMODEM_STA_WAIT_REPLY;
            ymodem_send_nak(usart_link);
            return;
        }
        ymodem_reset_transfer_state();
        if ((image_crc32_expected == 0UL) ||
            (image_crc32_expected == 0xFFFFFFFFUL))
        {
            ymodem_state_next = YMODEM_STA_WAIT_DATA;
            ymodem_state = YMODEM_STA_WAIT_REPLY;
            ymodem_send_nak(usart_link);
            return;
        }
        ymodem_send_ack(usart_link);
        ymodem_queue_reply(YMODEM_C,usart_link);
        ymodem_state_next = YMODEM_STA_WAIT_DATA;
        ymodem_state = YMODEM_STA_WAIT_REPLY;
//        is_updating = 1;
        // 记录升级端口
    }
    else
    {
        boot_jump_to_app();
    }
}

static void ymodem_wait_reply(uint8_t rx_com)
{
    if (ymodem_reply_port_is_valid(rx_com) == 0U)
    {
        ymodem_state = ymodem_state_next;
        return;
    }

    if (reply_len[rx_com] == 0)
    // 等待回复状态
    if (reply_len[rx_com] == 0)
    {
        ymodem_state = ymodem_state_next;
    }
}

static uint8_t ymodem_is_start_frame_ready(uint8_t port_com)
{
    ymodem_soh_first_parsed_t *first;

    if (port_com >= RX_MAX_COM)
    {
        return 0U;
    }

    if ((recv_buffer[port_com][0] != YMODEM_SOH) ||
        (recv_index[port_com] != YMODEM_START_TOTAL_LEN))
    {
        return 0U;
    }

    first = (ymodem_soh_first_parsed_t *)&recv_buffer[port_com][0];
    return (first->file_size != 0U) ? 1U : 0U;
}

static void ymodem_accept_start_frame(uint8_t port_com)
{
    memcpy(&ymodem_soh_first_parsed, &recv_buffer[port_com][0], sizeof(ymodem_soh_first_parsed));
    ymodem_state = YMODEM_STA_CHK_FIRST;
    updating_flag = 1U;
    update_started = 0U;
    pending_can_fallback = 0U;
    usart_link = port_com;
}

static void ymodem_wait_data(void)
{
    
    if(updating_flag == 0)
    {
        for(uint16_t i = 0;i < 2;i++)
        {
            if (is_recv_cplt[i])
            {
                is_recv_cplt[i] = 0;
                if (recv_buffer[i][0] == YMODEM_SOH && recv_index[i] == YMODEM_START_TOTAL_LEN)
                {
                    ymodem_soh_first_parsed_t *p_ymodem_soh_first_parsed_temp;
                    p_ymodem_soh_first_parsed_temp = (ymodem_soh_first_parsed_t *)&recv_buffer[i];
                    if (p_ymodem_soh_first_parsed_temp->file_size != 0)
                    {
                        ymodem_accept_start_frame((uint8_t)i);
                    }
                }
                recv_index[i] = 0;
            }
        }
    }
    else
    {
        // 等待数据状态
        for(uint16_t i = 0;i < 2;i++)
        {
            if ((is_recv_cplt[i])&&(usart_link == i))
            {
                uint8_t frame_handled = 0;
                is_recv_cplt[i] = 0;
                if (recv_buffer[i][0] == YMODEM_SOH && recv_index[i] == YMODEM_START_TOTAL_LEN)
                {
                    frame_handled = 1;
                    ymodem_soh_first_parsed_t *p_ymodem_soh_first_parsed_temp;
                    p_ymodem_soh_first_parsed_temp = (ymodem_soh_first_parsed_t *)&recv_buffer[i];
                    if (p_ymodem_soh_first_parsed_temp->file_size != 0)
                    {
                        memcpy(&ymodem_soh_first_parsed, &recv_buffer[i][0], sizeof(ymodem_soh_first_parsed));
                        ymodem_state = YMODEM_STA_CHK_FIRST;
                    }
                    else
                    {
                        memcpy(&ymodem_frame_soh_data, &recv_buffer[i][0], sizeof(ymodem_frame_soh_data));
                        ymodem_state = YMODEM_STA_END_DATA;
                    }
                }
                else if (recv_buffer[i][0] == YMODEM_STX && recv_index[i] == YMODEM_STX_BT_TOTAL_LEN)
                {
                    frame_handled = 1;
                    memcpy(&ymodem_frame_stx, &recv_buffer[i], sizeof(ymodem_frame_stx));
                    ymodem_state = YMODEM_STA_PROCESS_DATA;
                }
                else if (recv_buffer[i][0] == YMODEM_EOT && recv_index[i] == 1)
                {
                    frame_handled = 1;
                    ymodem_state = YMODEM_STA_RECV_END;
                }
                if ((frame_handled == 0) && (recv_index[i] != 0))
                {
                    ymodem_send_nak(usart_link);
                }
                recv_index[i] = 0;
            }
            else if (is_recv_cplt[i])
            {
                if ((ymodem_is_can_ota_link() != 0U) &&
                    (ymodem_is_start_frame_ready((uint8_t)i) != 0U))
                {
                    is_recv_cplt[i] = 0;
                    boot_iap_can_abort();
                    ymodem_accept_start_frame((uint8_t)i);
                    recv_index[i] = 0;
                    continue;
                }
                is_recv_cplt[i] = 0;
                recv_index[i] = 0;
            }
        }
    }
}

uint32_t recv_byte = 0;
static void ymodem_process_data(void)
{
    uint16_t crc_data = ymodem_crc16((const uint8_t *)&ymodem_frame_stx, sizeof(ymodem_frame_stx) - 2);
    if ((crc_data == ((uint16_t)ymodem_frame_stx.crc_high << 8 | ymodem_frame_stx.crc_low)) &&
        (ymodem_frame_stx.pn == (uint8_t)(~ymodem_frame_stx.xpn)) &&
        (ymodem_frame_stx.bt_pn < 8) &&
        (ymodem_frame_stx.pn <= 128))
    {
        uint32_t packet_start = ((uint32_t)ymodem_frame_stx.pn - 1U) * 1024U +
                                ((uint32_t)ymodem_frame_stx.bt_pn * 128U);
        uint32_t packet_end = packet_start + 128U;
        uint32_t payload_len = 128U;

        if ((ymodem_frame_stx.pn != expect_pn) ||
            (ymodem_frame_stx.bt_pn != expect_bt_pn))
        {
            ymodem_state = YMODEM_STA_WAIT_REPLY;
            ymodem_state_next = YMODEM_STA_WAIT_DATA;
            if (ymodem_is_duplicate_packet())
            {
                ymodem_send_ack(usart_link);
            }
            else
            {
                ymodem_send_nak(usart_link);
            }
            return;
        }

        if (packet_start >= image_size_byte)
        {
            ymodem_state = YMODEM_STA_WAIT_REPLY;
            ymodem_state_next = YMODEM_STA_WAIT_DATA;
            ymodem_send_nak(usart_link);
            return;
        }

        if (packet_end >= image_size_byte)
        {
            payload_len = image_size_byte - packet_start;
            recv_byte_is_ok = 1;
        }
        else
        {
            recv_byte_is_ok = 0;
        }

        recv_byte = packet_end;
        memcpy(&code_buffer[((ymodem_frame_stx.pn + 3) % 4) * 8 + (ymodem_frame_stx.bt_pn)][0],
               &ymodem_frame_stx.data[0],
               payload_len);
        image_crc32_calc = ymodem_crc32_update(image_crc32_calc, &ymodem_frame_stx.data[0], payload_len);
        image_bytes_received += payload_len;
        ymodem_advance_expected_packet();

        if ((((ymodem_frame_stx.pn % 4U) == 0U) &&
             (ymodem_frame_stx.bt_pn == 7U)) ||
            (recv_byte_is_ok == 1))
        {
            ymodem_state_next = YMODEM_STA_RECV_WRITE_FLASH;
            flash_page_4k = (ymodem_frame_stx.pn + 3U) / 4U - 1U;
        }
        else
        {
            ymodem_state_next = YMODEM_STA_WAIT_DATA;
        }

        ymodem_state = YMODEM_STA_WAIT_REPLY;
        ymodem_send_ack(usart_link);
    }
    else
    {
        ymodem_state = YMODEM_STA_WAIT_REPLY;
        ymodem_state_next = YMODEM_STA_WAIT_DATA;
        ymodem_send_nak(usart_link);
    }
}

static void ymodem_write_flash(void)
{
    if (update_started == 0)
    {
        if (ymodem_write_meta_status(IAP_META_STARTED) == 0)
        {
            ymodem_flash_error = 1;
            ymodem_state = YMODEM_STA_WAIT_DATA;
            return;
        }
        update_started = 1;
        pending_can_fallback = 0;
    }

    if(flash_page_4k == 0)
    {
        for(uint16_t i = 0;i < 4;i++)
        {
            Stack_Addr[i] = code_buffer[0][i];
            code_buffer[0][i] = 0xff;
        }
    }
    //flash_write(FLASH_BACKUP_APP, flash_page_4k * FLASH_LOGICAL_PAGE_SIZE, (uint16_t *)&code_buffer[0][0]);
    if ((flash_write(FLASH_APP, flash_page_4k * FLASH_LOGICAL_PAGE_SIZE, (uint16_t *)&code_buffer[0][0]) == 0) ||
        (ymodem_verify_flash_page(FLASH_APP, flash_page_4k * FLASH_LOGICAL_PAGE_SIZE, &code_buffer[0][0]) == 0))
    {
        ymodem_flash_error = 1;
    }
    ymodem_state = YMODEM_STA_WAIT_DATA;
    memset(&code_buffer[0][0], 0xFF, sizeof(code_buffer));
}

static void ymodem_recv_end(void)
{
    ymodem_state_next = YMODEM_STA_WAIT_DATA;
    ymodem_state = YMODEM_STA_WAIT_REPLY;
    ymodem_queue_reply(YMODEM_C,usart_link);
}

static void ymodem_end_data(void)
{
    uint16_t crc_data = ymodem_crc16((const uint8_t *)&ymodem_frame_soh_data, sizeof(ymodem_frame_soh_data) - 2);

    if ((ymodem_frame_soh_data.pn == 0x00) &&
        (ymodem_frame_soh_data.xpn == 0xFF) &&
        (ymodem_frame_soh_data.header == YMODEM_SOH) &&
        (crc_data == ((uint16_t)ymodem_frame_soh_data.crc_high << 8 | ymodem_frame_soh_data.crc_low)) &&
        (recv_byte_is_ok == 1) &&
        (ymodem_flash_error == 0) &&
        (image_bytes_received == image_size_byte) &&
        (ymodem_received_crc32_is_ok() != 0))
    {
        ymodem_state_next = YMODEM_STA_RESET;
        ymodem_state = YMODEM_STA_WAIT_REPLY;
        code_size_byte = swap_endian_u32(ymodem_soh_first_parsed.file_size);

        flash_read(FLASH_APP, 0, (uint16_t *)&code_buffer[0][0]);
        for (uint32_t i = 0; i < sizeof(Stack_Addr); i++)
        {
            code_buffer[0][i] = Stack_Addr[i];
        }
        if ((flash_write(FLASH_APP,0, (uint16_t *)&code_buffer[0][0]) != 0) &&
            (ymodem_verify_flash_page(FLASH_APP, 0, &code_buffer[0][0]) != 0) &&
            (ymodem_verify_app_crc32() != 0))
        {
            if (ymodem_write_meta_status(IAP_META_VALID) != 0)
            {
                ymodem_queue_reply(RYM_CODE_O,usart_link);
            }
            else
            {
                ymodem_state_next = YMODEM_STA_WAIT_DATA;
                ymodem_send_nak(usart_link);
            }
        }
        else
        {
            ymodem_state_next = YMODEM_STA_WAIT_DATA;
            ymodem_send_nak(usart_link);
        }

    }
    else
    {
        ymodem_state_next = YMODEM_STA_RESET;
        ymodem_state = YMODEM_STA_WAIT_DATA;
        ymodem_send_nak(usart_link);
    }
}

static void boot_iap_can_prepare_header(uint32_t image_size, uint32_t image_crc32)
{
    const char product[] = "WG-BC1500M";
    uint16_t crc_data;

    memset(&ymodem_soh_first_parsed, 0, sizeof(ymodem_soh_first_parsed));
    ymodem_soh_first_parsed.header = YMODEM_SOH;
    ymodem_soh_first_parsed.pn = 0x00U;
    ymodem_soh_first_parsed.xpn = 0xFFU;
    memcpy(ymodem_soh_first_parsed.product_name, product, sizeof(product) - 1U);
    ymodem_soh_first_parsed.file_size = swap_endian_u32(image_size);
    ymodem_soh_first_parsed.baud = 0xFFFFU;
    ymodem_soh_first_parsed.passthrough = 0xFFU;
    ymodem_soh_first_parsed.upgrade_mode = 0xFFU;
    ymodem_soh_first_parsed.hw_ver = 0xFFFFFFFFUL;
    ymodem_soh_first_parsed.link_id = RX_CAN_OTA_COM;
    ymodem_soh_first_parsed.padding[YMODEM_INFO_IMAGE_CRC32_OFFSET + 0U] = (uint8_t)(image_crc32 & 0xFFU);
    ymodem_soh_first_parsed.padding[YMODEM_INFO_IMAGE_CRC32_OFFSET + 1U] = (uint8_t)((image_crc32 >> 8) & 0xFFU);
    ymodem_soh_first_parsed.padding[YMODEM_INFO_IMAGE_CRC32_OFFSET + 2U] = (uint8_t)((image_crc32 >> 16) & 0xFFU);
    ymodem_soh_first_parsed.padding[YMODEM_INFO_IMAGE_CRC32_OFFSET + 3U] = (uint8_t)((image_crc32 >> 24) & 0xFFU);
    crc_data = ymodem_crc16((const uint8_t *)&ymodem_soh_first_parsed, sizeof(ymodem_soh_first_parsed) - 2);
    ymodem_soh_first_parsed.crc_high = (uint8_t)(crc_data >> 8);
    ymodem_soh_first_parsed.crc_low = (uint8_t)(crc_data & 0xFFU);
}

static uint8_t boot_iap_can_flush_page(void)
{
    if (can_iap_page_used == 0U)
    {
        return 1U;
    }

    if (update_started == 0U)
    {
        if (ymodem_write_meta_status(IAP_META_STARTED) == 0U)
        {
            return 0U;
        }
        update_started = 1U;
        pending_can_fallback = 0U;
    }

    if (can_iap_page_base == 0U)
    {
        for (uint16_t i = 0U; i < sizeof(Stack_Addr); i++)
        {
            Stack_Addr[i] = can_iap_page[i];
            can_iap_page[i] = 0xFFU;
        }
    }

    if ((flash_write(FLASH_APP, can_iap_page_base, (uint16_t *)&can_iap_page[0]) == 0U) ||
        (ymodem_verify_flash_page(FLASH_APP, can_iap_page_base, &can_iap_page[0]) == 0U))
    {
        return 0U;
    }

    can_iap_page_base += FLASH_LOGICAL_PAGE_SIZE;
    can_iap_page_used = 0U;
    memset(&can_iap_page[0], 0xFF, sizeof(can_iap_page));
    return 1U;
}

uint8_t boot_iap_can_begin(uint32_t image_size, uint32_t image_crc32)
{
    if ((image_size == 0U) ||
        (image_size > flash_zone[FLASH_APP].size) ||
        (image_crc32 == 0U) ||
        (image_crc32 == 0xFFFFFFFFUL))
    {
        return 0U;
    }
	
   if (ymodem_clear_meta() == 0U)
    {
        return 0U;
    }

    usart_link = RX_CAN_OTA_COM;
    boot_iap_can_prepare_header(image_size, image_crc32);
    image_size_byte = image_size;
    image_bytes_received = 0U;
    image_crc32_calc = 0xFFFFFFFFUL;
    image_crc32_expected = image_crc32;
    can_iap_page_base = 0U;
    can_iap_page_used = 0U;
    can_iap_received = 0U;
    can_iap_active = 1U;
    updating_flag = 1U;
    update_started = 0U;
    pending_can_fallback = 1U;
    recv_byte_is_ok = 0U;
    ymodem_flash_error = 0U;
    memset(&Stack_Addr[0], 0xFF, sizeof(Stack_Addr));
    memset(&can_iap_page[0], 0xFF, sizeof(can_iap_page));

    if (ymodem_write_meta_status(IAP_META_PENDING) == 0U)
    {
        can_iap_active = 0U;
        return 0U;
    }

    return 1U;
}

uint8_t boot_iap_can_write(uint32_t offset, const uint8_t *data, uint32_t len)
{
    uint32_t src = 0U;

    if ((can_iap_active == 0U) ||
        (data == NULL) ||
        (offset != can_iap_received) ||
        ((offset + len) > image_size_byte))
    {
        return 0U;
    }

    while (src < len)
    {
        uint32_t room = FLASH_LOGICAL_PAGE_SIZE - can_iap_page_used;
        uint32_t chunk = (len - src > room) ? room : (len - src);

        memcpy(&can_iap_page[can_iap_page_used], &data[src], chunk);
        image_crc32_calc = ymodem_crc32_update(image_crc32_calc, &data[src], chunk);
        image_bytes_received += chunk;
        can_iap_received += chunk;
        can_iap_page_used += chunk;
        src += chunk;

        if (can_iap_page_used >= FLASH_LOGICAL_PAGE_SIZE)
        {
            if (boot_iap_can_flush_page() == 0U)
            {
                ymodem_flash_error = 1U;
                return 0U;
            }
        }
    }

    if (can_iap_received >= image_size_byte)
    {
        recv_byte_is_ok = 1U;
    }

    return 1U;
}

uint8_t boot_iap_can_finish(void)
{
    if ((can_iap_active == 0U) ||
        (ymodem_flash_error != 0U) ||
        (image_bytes_received != image_size_byte) ||
        (ymodem_received_crc32_is_ok() == 0U))
    {
        return 0U;
    }

    if (boot_iap_can_flush_page() == 0U)
    {
        return 0U;
    }

    if (flash_read(FLASH_APP, 0, (uint16_t *)&can_iap_page[0]) == 0U)
    {
        return 0U;
    }

    for (uint32_t i = 0U; i < sizeof(Stack_Addr); i++)
    {
        can_iap_page[i] = Stack_Addr[i];
    }

    if ((flash_write(FLASH_APP, 0, (uint16_t *)&can_iap_page[0]) == 0U) ||
        (ymodem_verify_flash_page(FLASH_APP, 0, &can_iap_page[0]) == 0U) ||
        (ymodem_verify_app_crc32() == 0U) ||
        (ymodem_write_meta_status(IAP_META_VALID) == 0U))
    {
        return 0U;
    }

    can_iap_active = 0U;
    return 1U;
}

void boot_iap_can_abort(void)
{
    can_ota_reset_session();
    ymodem_clear_meta();
    usart_link = 0U;
    updating_flag = 0U;
    update_started = 0U;
    recv_byte_is_ok = 0U;
    ymodem_flash_error = 0U;
    image_size_byte = 0U;
    image_bytes_received = 0U;
    image_crc32_calc = 0xFFFFFFFFUL;
    image_crc32_expected = 0U;
    can_iap_active = 0U;
    can_iap_page_base = 0U;
    can_iap_page_used = 0U;
    can_iap_received = 0U;
    pending_can_fallback = 0U;
    memset(&Stack_Addr[0], 0xFF, sizeof(Stack_Addr));
    memset(&can_iap_page[0], 0xFF, sizeof(can_iap_page));
    ymodem_state = YMODEM_STA_WAIT_DATA;
    ymodem_state_next = YMODEM_STA_WAIT_DATA;
}

void boot_iap_can_prepare_restart(void)
{
    can_ota_prepare_boot_recovery();
    usart_link = RX_CAN_OTA_COM;
    updating_flag = 0U;
    update_started = 0U;
    recv_byte_is_ok = 0U;
    ymodem_flash_error = 0U;
    image_size_byte = 0U;
    image_bytes_received = 0U;
    image_crc32_calc = 0xFFFFFFFFUL;
    image_crc32_expected = 0U;
    can_iap_active = 0U;
    can_iap_page_base = 0U;
    can_iap_page_used = 0U;
    can_iap_received = 0U;
    pending_can_fallback = 0U;
    memset(&Stack_Addr[0], 0xFF, sizeof(Stack_Addr));
    memset(&can_iap_page[0], 0xFF, sizeof(can_iap_page));
    ymodem_state = YMODEM_STA_WAIT_DATA;
    ymodem_state_next = YMODEM_STA_WAIT_DATA;
}

uint32_t boot_iap_can_received(void)
{
    return can_iap_received;
}

static void ymodem_process_current_frame(void)
{
    switch (ymodem_state)
    {
    case YMODEM_STA_INIT:
        ymodem_init();
        break;
    case YMODEM_STA_IDLE:
        break;
    case YMODEM_STA_CHK_FIRST:
        ymodem_chk_first();
        break;
    case YMODEM_STA_PROCESS_DATA:
        ymodem_process_data();
        break;
    case YMODEM_STA_RECV_END:
        ymodem_recv_end();
        break;
    case YMODEM_STA_RECV_CP_DATA:
        break;
    case YMODEM_STA_RECV_CP_END:
        break;
    case YMODEM_STA_WAIT_REPLY:
        ymodem_wait_reply(usart_link);
        break;
    case YMODEM_STA_WAIT_DATA:
        ymodem_wait_data();
        break;
    case YMODEM_STA_RECV_WRITE_FLASH:
        ymodem_write_flash();
        break;
    case YMODEM_STA_END_DATA:
        ymodem_end_data();
        break;
    case YMODEM_STA_RESET:
        NVIC_SystemReset();
        break;
    }
}

REG_TASK(1, ymodem_process_current_frame)

static uint8_t ymodem_state_flag = 0;
    
uint32_t get_ymodem_state_flag(void)
{
    return ymodem_state_flag;
}

void Upgrade_timeout_state(void)
{
    static uint16_t timeout = 0;
    if(ymodem_state == YMODEM_STA_WAIT_DATA)
    {
        if(++timeout >= 50)
        {
            timeout = 0;
            ymodem_state_flag = 0;
            if ((pending_can_fallback == 1) &&
                (update_started == 0))
            {
                if ((ymodem_is_can_ota_link() == 0U) ||
                    (can_iap_active == 0U))
                {
                    boot_iap_can_abort();
                }
            }
            else if (update_started == 0)
            {
                if (ymodem_is_can_ota_link() == 0U)
                {
                    updating_flag = 0;
                }
            }
            else if (updating_flag != 0)
            {
                if (ymodem_is_can_ota_link() == 0U)
                {
                    ymodem_send_nak(usart_link);
                }
            }
        }
    }
    else
    {
        timeout = 0;
        ymodem_state_flag = 1;

    }
}

REG_TASK(100, Upgrade_timeout_state)


