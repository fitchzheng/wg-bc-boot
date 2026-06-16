#include "can_ota.h"
#include "bsp_can.h"
#include "ymodem.h"
#include "section.h"
#include "flash.h"
#include <string.h>

#define CAN_OTA_CTRL_RX_ID              0x1C510000UL
#define CAN_OTA_CTRL_TX_ID              0x1C520000UL
#define CAN_OTA_DATA_ID_BASE            0x1C500000UL
#define CAN_OTA_DATA_ID_MASK            0x1FFF0000UL

#define CAN_OTA_CMD_START               0xA0U
#define CAN_OTA_CMD_START_CRC           0xA1U
#define CAN_OTA_CMD_QUERY               0xA2U
#define CAN_OTA_CMD_BLOCK_END           0xB1U
#define CAN_OTA_CMD_FINISH              0xD0U
#define CAN_OTA_CMD_ABORT               0xE0U

#define CAN_OTA_ACK_OK                  0xC0U
#define CAN_OTA_ACK_NAK                 0x15U
#define CAN_OTA_ACK_DONE                0xD1U

#define CAN_OTA_ERR_NONE                0U
#define CAN_OTA_ERR_STATE               1U
#define CAN_OTA_ERR_PARAM               2U
#define CAN_OTA_ERR_SEQ                 3U
#define CAN_OTA_ERR_CRC                 4U
#define CAN_OTA_ERR_FLASH               5U

typedef enum
{
    CAN_OTA_IDLE = 0,
    CAN_OTA_WAIT_START_CRC,
    CAN_OTA_RX_BLOCK,
    CAN_OTA_DONE,
} can_ota_state_t;

static can_ota_state_t can_ota_state = CAN_OTA_IDLE;
static uint8_t block_buf[CAN_OTA_BLOCK_SIZE];
static uint32_t block_frame_mask = 0U;
static uint32_t image_size = 0U;
static uint32_t image_crc32 = 0U;
static uint16_t expected_block = 0U;
static uint16_t total_blocks = 0U;
static uint16_t reset_delay_ms = 0U;

static uint32_t can_ota_read_le32(const uint8_t *data)
{
    return ((uint32_t)data[0]) |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static uint32_t can_ota_crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0U; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0U; bit < 8U; bit++)
        {
            crc = ((crc & 1U) != 0U) ? ((crc >> 1) ^ 0xEDB88320UL) : (crc >> 1);
        }
    }
    return crc;
}

static uint32_t can_ota_crc32(const uint8_t *data, uint32_t len)
{
    return can_ota_crc32_update(0xFFFFFFFFUL, data, len) ^ 0xFFFFFFFFUL;
}

static void can_ota_ack(uint8_t code, uint8_t err)
{
    uint8_t tx[8] = {0};

    tx[0] = code;
    tx[1] = err;
    tx[2] = (uint8_t)(expected_block & 0xFFU);
    tx[3] = (uint8_t)(expected_block >> 8);
    tx[4] = (uint8_t)(boot_iap_can_received() & 0xFFU);
    tx[5] = (uint8_t)((boot_iap_can_received() >> 8) & 0xFFU);
    tx[6] = (uint8_t)((boot_iap_can_received() >> 16) & 0xFFU);
    tx[7] = (uint8_t)((boot_iap_can_received() >> 24) & 0xFFU);
    (void)bsp_can_tx_ext(CAN_OTA_CTRL_TX_ID, tx);
}

static uint8_t can_ota_is_data_id(uint32_t id)
{
    return ((id & CAN_OTA_DATA_ID_MASK) == CAN_OTA_DATA_ID_BASE) ? 1U : 0U;
}

static void can_ota_reset_block_buffer(void)
{
    memset(&block_buf[0], 0xFF, sizeof(block_buf));
    block_frame_mask = 0U;
}

void can_ota_reset_session(void)
{
    can_ota_state = CAN_OTA_IDLE;
    image_size = 0U;
    image_crc32 = 0U;
    expected_block = 0U;
    total_blocks = 0U;
    reset_delay_ms = 0U;
    can_ota_reset_block_buffer();
}

void can_ota_prepare_boot_recovery(void)
{
    bsp_can_init();
    can_ota_reset_session();
}

static void can_ota_handle_start(const uint8_t *data)
{
    uint32_t size;

    if ((data[1] != 1U) ||
        (data[6] != CAN_OTA_FRAMES_PER_BLOCK) ||
        (data[7] != 0U))
    {
        can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_PARAM);
        return;
    }

    size = can_ota_read_le32(&data[2]);
    if ((size == 0U) || (size > flash_zone[FLASH_APP].size))
    {
        can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_PARAM);
        return;
    }

    if (can_ota_state == CAN_OTA_RX_BLOCK)
    {
        if (size == image_size)
        {
            can_ota_ack(CAN_OTA_ACK_OK, CAN_OTA_ERR_NONE);
        }
        else
        {
            can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_STATE);
        }
        return;
    }

    boot_iap_can_abort();
    can_ota_reset_session();
    image_size = size;
    total_blocks = (uint16_t)((image_size + CAN_OTA_BLOCK_SIZE - 1U) / CAN_OTA_BLOCK_SIZE);
    can_ota_state = CAN_OTA_WAIT_START_CRC;
    can_ota_ack(CAN_OTA_ACK_OK, CAN_OTA_ERR_NONE);
}

static void can_ota_handle_start_crc(const uint8_t *data)
{
    uint16_t host_blocks;
    uint32_t host_crc;

    host_crc = can_ota_read_le32(&data[1]);
    host_blocks = (uint16_t)data[5] | ((uint16_t)data[6] << 8);

    if (can_ota_state == CAN_OTA_RX_BLOCK)
    {
        if ((host_crc == image_crc32) &&
            (host_blocks == total_blocks) &&
            (data[7] == 0U))
        {
            can_ota_ack(CAN_OTA_ACK_OK, CAN_OTA_ERR_NONE);
        }
        else
        {
            can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_STATE);
        }
        return;
    }

    if (can_ota_state != CAN_OTA_WAIT_START_CRC)
    {
        can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_STATE);
        return;
    }

    image_crc32 = host_crc;
    if ((image_crc32 == 0U) ||
        (image_crc32 == 0xFFFFFFFFUL) ||
        (host_blocks != total_blocks) ||
        (boot_iap_can_begin(image_size, image_crc32) == 0U))
    {
        can_ota_reset_session();
        can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_PARAM);
        return;
    }

    expected_block = 0U;
    can_ota_state = CAN_OTA_RX_BLOCK;
    can_ota_reset_block_buffer();
    can_ota_ack(CAN_OTA_ACK_OK, CAN_OTA_ERR_NONE);
}

static void can_ota_handle_query(const uint8_t *data)
{
    uint32_t host_crc = can_ota_read_le32(&data[1]);
    uint16_t host_blocks = (uint16_t)data[5] | ((uint16_t)data[6] << 8);

    if (can_ota_state != CAN_OTA_RX_BLOCK)
    {
        can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_STATE);
        return;
    }

    if ((host_crc != image_crc32) ||
        (host_blocks != total_blocks) ||
        (data[7] != CAN_OTA_FRAMES_PER_BLOCK))
    {
        can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_PARAM);
        return;
    }

    can_ota_reset_block_buffer();
    can_ota_ack(CAN_OTA_ACK_OK, CAN_OTA_ERR_NONE);
}

static void can_ota_handle_data(uint32_t id, const uint8_t *data)
{
    uint16_t block = (uint16_t)((id >> 5) & 0x7FFU);
    uint8_t frame = (uint8_t)(id & 0x1FU);

    if (can_ota_state != CAN_OTA_RX_BLOCK)
    {
        return;
    }

    if ((block != expected_block) || (frame >= CAN_OTA_FRAMES_PER_BLOCK))
    {
        return;
    }

    memcpy(&block_buf[(uint16_t)frame * 8U], data, 8U);
    block_frame_mask |= (1UL << frame);
}

static uint32_t can_ota_expected_mask(uint8_t frame_count)
{
    return (frame_count >= 32U) ? 0xFFFFFFFFUL : ((1UL << frame_count) - 1UL);
}

static void can_ota_handle_block_end(const uint8_t *data)
{
    uint16_t block;
    uint8_t valid_len;
    uint8_t expected_frames;
    uint32_t expected_crc;
    uint32_t actual_crc;
    uint32_t offset;
    uint32_t remain;
    uint32_t block_len;

    if (can_ota_state != CAN_OTA_RX_BLOCK)
    {
        can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_STATE);
        return;
    }

    block = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
    valid_len = data[3];
    expected_crc = can_ota_read_le32(&data[4]);
    offset = (uint32_t)expected_block * CAN_OTA_BLOCK_SIZE;
    remain = image_size - offset;
    block_len = (remain > CAN_OTA_BLOCK_SIZE) ? CAN_OTA_BLOCK_SIZE : remain;
    expected_frames = (uint8_t)((block_len + 7U) / 8U);

    if ((block != expected_block) ||
        ((valid_len == 0U ? CAN_OTA_BLOCK_SIZE : valid_len) != block_len) ||
        ((block_frame_mask & can_ota_expected_mask(expected_frames)) != can_ota_expected_mask(expected_frames)))
    {
        can_ota_reset_block_buffer();
        can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_SEQ);
        return;
    }

    actual_crc = can_ota_crc32(&block_buf[0], block_len);
    if (actual_crc != expected_crc)
    {
        can_ota_reset_block_buffer();
        can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_CRC);
        return;
    }

    if (boot_iap_can_write(offset, &block_buf[0], block_len) == 0U)
    {
        can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_FLASH);
        return;
    }

    expected_block++;
    can_ota_reset_block_buffer();
    can_ota_ack(CAN_OTA_ACK_OK, CAN_OTA_ERR_NONE);
}

static void can_ota_handle_finish(const uint8_t *data)
{
    uint32_t finish_crc = can_ota_read_le32(&data[1]);

    if ((can_ota_state != CAN_OTA_RX_BLOCK) ||
        (expected_block != total_blocks) ||
        (finish_crc != image_crc32) ||
        (boot_iap_can_finish() == 0U))
    {
        boot_iap_can_abort();
        can_ota_reset_session();
        can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_FLASH);
        return;
    }

    can_ota_state = CAN_OTA_DONE;
    can_ota_ack(CAN_OTA_ACK_DONE, CAN_OTA_ERR_NONE);
    reset_delay_ms = 200U;
}

static void can_ota_handle_ctrl(const uint8_t *data)
{
    switch (data[0])
    {
    case CAN_OTA_CMD_START:
        can_ota_handle_start(data);
        break;
    case CAN_OTA_CMD_START_CRC:
        can_ota_handle_start_crc(data);
        break;
    case CAN_OTA_CMD_QUERY:
        can_ota_handle_query(data);
        break;
    case CAN_OTA_CMD_BLOCK_END:
        can_ota_handle_block_end(data);
        break;
    case CAN_OTA_CMD_FINISH:
        can_ota_handle_finish(data);
        break;
    case CAN_OTA_CMD_ABORT:
        boot_iap_can_abort();
        can_ota_reset_session();
        can_ota_ack(CAN_OTA_ACK_OK, CAN_OTA_ERR_NONE);
        break;
    default:
        can_ota_ack(CAN_OTA_ACK_NAK, CAN_OTA_ERR_PARAM);
        break;
    }
}

static void can_ota_init(void)
{
    bsp_can_init();
    can_ota_reset_session();
}

REG_INIT(can_ota_init)

void can_ota_task(void)
{
    uint32_t id;
    uint8_t data[8];

    if (reset_delay_ms > 0U)
    {
        reset_delay_ms--;
        if (reset_delay_ms == 0U)
        {
            NVIC_SystemReset();
        }
    }

    while (bsp_can_rx_ext(&id, &data[0]) != 0U)
    {
        if (id == CAN_OTA_CTRL_RX_ID)
        {
            can_ota_handle_ctrl(&data[0]);
        }
        else if (can_ota_is_data_id(id) != 0U)
        {
            can_ota_handle_data(id, &data[0]);
        }
    }
}

REG_TASK(1, can_ota_task)
