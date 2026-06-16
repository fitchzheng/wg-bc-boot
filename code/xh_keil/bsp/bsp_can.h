#ifndef __BSP_CAN_H__
#define __BSP_CAN_H__

#include "hc32_ll.h"
#include "hc32_ll_mcan.h"

#define MCAN_UNIT_SEL                   (1U)
#define MCAN_PERIPH_CLK                 (FCG1_PERIPH_MCAN1)
#define MCAN_UNIT                       (CM_MCAN1)
#define MCAN_CLK_UNIT                   (CLK_MCAN1)
#define MCAN_CLK_SRC                    (CLK_MCANCLK_SYSCLK_DIV3)

#define MCAN_TX_PORT                    (GPIO_PORT_B)
#define MCAN_TX_PIN                     (GPIO_PIN_09)
#define MCAN_TX_PIN_FUNC                (GPIO_FUNC_54)
#define MCAN_RX_PORT                    (GPIO_PORT_B)
#define MCAN_RX_PIN                     (GPIO_PIN_08)
#define MCAN_RX_PIN_FUNC                (GPIO_FUNC_55)
#define MCAN_PHY_STBY_PORT              (GPIO_PORT_B)
#define MCAN_PHY_STBY_PIN               (GPIO_PIN_02)

#define MCAN_STD_FILTER_NUM             (1U)
#define MCAN_EXT_FILTER_NUM             (1U)
#define MCAN_RX_FIFO0_NUM               (0U)
#define MCAN_RX_FIFO0_WATERMARK         (0U)
#define MCAN_RX_FIFO0_DATA_FIELD_SIZE   MCAN_DATA_SIZE_8BYTE
#define MCAN_RX_FIFO1_NUM               (32U)
#define MCAN_RX_FIFO1_WATERMARK         (1U)
#define MCAN_RX_FIFO1_DATA_FIELD_SIZE   MCAN_DATA_SIZE_8BYTE
#define MCAN_RX_BUF_NUM                 (0U)
#define MCAN_RX_BUF_DATA_FIELD_SIZE     MCAN_DATA_SIZE_8BYTE
#define MCAN_TX_BUF_NUM                 (0U)
#define MCAN_TX_FIFO_NUM                (3U)
#define MCAN_TX_BUF_DATA_FIELD_SIZE     MCAN_DATA_SIZE_8BYTE
#define MCAN_TX_EVT_NUM                 (0U)

#define MCAN_STD_FILTER0                {.u32IdType = MCAN_STD_ID, .u32FilterType = MCAN_FILTER_RANGE, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_FIFO1, .u32FilterId1 = 0U, \
                                         .u32FilterId2 = 0x7FFUL,}
#define MCAN_EXT_FILTER0                {.u32IdType = MCAN_EXT_ID, .u32FilterType = MCAN_FILTER_RANGE, \
                                         .u32FilterConfig = MCAN_FILTER_TO_RX_FIFO1, .u32FilterId1 = 0U, \
                                         .u32FilterId2 = 0x1FFFFFFFUL,}

void bsp_can_init(void);
uint8_t bsp_can_tx_ext(uint32_t id, const uint8_t *data);
uint8_t bsp_can_rx_ext(uint32_t *id, uint8_t *data);

#endif
