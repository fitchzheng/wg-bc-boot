#include "bsp_can.h"
#include <string.h>

static void McanInitConfig(void);

#define MCAN_TX_PENDING_MASK            ((1UL << MCAN_TX_FIFO_NUM) - 1UL)
#define MCAN_SOFT_FAULT_RESTART_TICKS   (300U)
#define MCAN_TX_STALL_RESTART_TICKS     (300U)

static uint16_t mcan_soft_fault_ticks = 0U;
static uint16_t mcan_tx_stall_ticks = 0U;

static void McanCommClockConfig(void)
{
    CLK_SetCANClockSrc(MCAN_CLK_UNIT, MCAN_CLK_SRC);
}

static void McanPinConfig(void)
{
    GPIO_SetFunc(MCAN_TX_PORT, MCAN_TX_PIN, MCAN_TX_PIN_FUNC);
    GPIO_SetFunc(MCAN_RX_PORT, MCAN_RX_PIN, MCAN_RX_PIN_FUNC);
}

static void McanPhyEnable(void)
{
    stc_gpio_init_t gpio;

    (void)GPIO_StructInit(&gpio);
    gpio.u16PinState = PIN_STAT_RST;
    gpio.u16PinDir = PIN_DIR_OUT;
    GPIO_Init(MCAN_PHY_STBY_PORT, MCAN_PHY_STBY_PIN, &gpio);
    GPIO_ResetPins(MCAN_PHY_STBY_PORT, MCAN_PHY_STBY_PIN);
    GPIO_OutputCmd(MCAN_PHY_STBY_PORT, MCAN_PHY_STBY_PIN, ENABLE);
}

static void McanRestart(void)
{
    MCAN_AbortTxRequest(MCAN_UNIT, MCAN_TX_PENDING_MASK);
    (void)MCAN_Stop(MCAN_UNIT);
    (void)MCAN_DeInit(MCAN_UNIT);
    McanCommClockConfig();
    MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_ALL);
    McanInitConfig();
    McanPinConfig();
    McanPhyEnable();
    MCAN_Start(MCAN_UNIT);
    mcan_soft_fault_ticks = 0U;
    mcan_tx_stall_ticks = 0U;
}

static void McanRecoverIfNeeded(void)
{
    stc_mcan_protocol_status_t status = {0};
    stc_mcan_error_counter_t err = {0};
    uint8_t hard_fault = 0U;
    uint8_t soft_fault = 0U;
    uint8_t tx_pending = 0U;

    (void)MCAN_GetProtocolStatus(MCAN_UNIT, &status);
    (void)MCAN_GetErrorCounter(MCAN_UNIT, &err);

    tx_pending = (MCAN_CheckTxBufferPending(MCAN_UNIT, MCAN_TX_PENDING_MASK) == LL_OK) ? 1U : 0U;

    if ((status.u8BusOffFlag != 0U) ||
        (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_BUS_OFF) == SET) ||
        (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RAM_ACCESS_FAILURE) == SET) ||
        (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_BIT_ERR_UNCORRECTED) == SET))
    {
        hard_fault = 1U;
    }

    if ((status.u8ErrorPassiveFlag != 0U) ||
        (status.u8WarningFlag != 0U) ||
        (err.u8TxErrorCount >= 96U) ||
        (err.u8RxErrorCount >= 96U) ||
        (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_ERR_PASSIVE) == SET) ||
        (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_ERR_WARNING) == SET) ||
        (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_ARB_PHASE_ERROR) == SET) ||
        (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_DATA_PHASE_ERROR) == SET))
    {
        soft_fault = 1U;
    }

    if (hard_fault != 0U)
    {
        McanRestart();
        return;
    }

    if (tx_pending != 0U)
    {
        if ((MCAN_GetTxFifoFreeLevel(MCAN_UNIT) == 0U) ||
            (status.u8LastErrorCode == MCAN_PROTOCOL_ACK_ERR) ||
            (status.u8ComState == MCAN_COM_STATE_TX) ||
            (err.u8TxErrorCount != 0U))
        {
            if (mcan_tx_stall_ticks < 0xFFFFU)
            {
                mcan_tx_stall_ticks++;
            }
        }
    }
    else
    {
        mcan_tx_stall_ticks = 0U;
    }

    if (soft_fault != 0U)
    {
        if (mcan_soft_fault_ticks < 0xFFFFU)
        {
            mcan_soft_fault_ticks++;
        }
    }
    else if (tx_pending == 0U)
    {
        mcan_soft_fault_ticks = 0U;
    }

    if ((mcan_tx_stall_ticks >= MCAN_TX_STALL_RESTART_TICKS) ||
        (mcan_soft_fault_ticks >= MCAN_SOFT_FAULT_RESTART_TICKS))
    {
        McanRestart();
        return;
    }

    if ((MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_MSG_LOST) == SET) ||
        (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_FULL) == SET))
    {
        MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_MSG_LOST | MCAN_FLAG_RX_FIFO1_FULL);
    }
}

static void McanInitConfig(void)
{
    stc_mcan_init_t init;
    stc_mcan_filter_t std_filter[MCAN_STD_FILTER_NUM] = {MCAN_STD_FILTER0};
    stc_mcan_filter_t ext_filter[MCAN_EXT_FILTER_NUM] = {MCAN_EXT_FILTER0};

    (void)MCAN_StructInit(&init);
    init.u32Mode = MCAN_MD_NORMAL;
    init.u32FrameFormat = MCAN_FRAME_CLASSIC;
    init.stcBitTime.u32NominalPrescaler = 8U;
    init.stcBitTime.u32NominalTimeSeg1 = 16U;
    init.stcBitTime.u32NominalTimeSeg2 = 4U;
    init.stcBitTime.u32NominalSyncJumpWidth = 4U;
    init.stcMsgRam.u32AddrOffset = 0U;
    init.stcMsgRam.u32StdFilterNum = MCAN_STD_FILTER_NUM;
    init.stcMsgRam.u32ExtFilterNum = MCAN_EXT_FILTER_NUM;
    init.stcMsgRam.u32RxFifo0Num = MCAN_RX_FIFO0_NUM;
    init.stcMsgRam.u32RxFifo0DataSize = MCAN_RX_FIFO0_DATA_FIELD_SIZE;
    init.stcMsgRam.u32RxFifo1Num = MCAN_RX_FIFO1_NUM;
    init.stcMsgRam.u32RxFifo1DataSize = MCAN_RX_FIFO1_DATA_FIELD_SIZE;
    init.stcMsgRam.u32RxBufferNum = MCAN_RX_BUF_NUM;
    init.stcMsgRam.u32RxBufferDataSize = MCAN_RX_BUF_DATA_FIELD_SIZE;
    init.stcMsgRam.u32TxBufferNum = MCAN_TX_BUF_NUM;
    init.stcMsgRam.u32TxFifoQueueNum = MCAN_TX_FIFO_NUM;
    init.stcMsgRam.u32TxFifoQueueMode = MCAN_TX_FIFO_MD;
    init.stcMsgRam.u32TxDataSize = MCAN_TX_BUF_DATA_FIELD_SIZE;
    init.stcMsgRam.u32TxEventNum = MCAN_TX_EVT_NUM;
    init.stcFilter.pstcStdFilterList = std_filter;
    init.stcFilter.pstcExtFilterList = ext_filter;
    init.stcFilter.u32StdFilterConfigNum = init.stcMsgRam.u32StdFilterNum;
    init.stcFilter.u32ExtFilterConfigNum = init.stcMsgRam.u32ExtFilterNum;

    FCG_Fcg1PeriphClockCmd(MCAN_PERIPH_CLK, ENABLE);
    (void)MCAN_Init(MCAN_UNIT, &init);
    MCAN_RxFifoOperationModeConfig(MCAN_UNIT, MCAN_RX_FIFO1, MCAN_RX_FIFO_BLOCKING);
}

void bsp_can_init(void)
{
    McanCommClockConfig();
    McanInitConfig();
    McanPinConfig();
    McanPhyEnable();
    MCAN_Start(MCAN_UNIT);
}

uint8_t bsp_can_tx_ext(uint32_t id, const uint8_t *data)
{
    stc_mcan_tx_msg_t msg = {
        .ID = id,
        .IDE = 1U,
        .DLC = MCAN_DLC8,
    };

    McanRecoverIfNeeded();

    if (data != NULL)
    {
        memcpy(&msg.au8Data[0], data, 8U);
    }

    if (MCAN_AddMsgToTxFifoQueue(MCAN_UNIT, &msg) == LL_OK)
    {
        return 1U;
    }

    McanRestart();
    return (MCAN_AddMsgToTxFifoQueue(MCAN_UNIT, &msg) == LL_OK) ? 1U : 0U;
}

uint8_t bsp_can_rx_ext(uint32_t *id, uint8_t *data)
{
    stc_mcan_rx_msg_t msg = {0};

    if ((id == NULL) || (data == NULL))
    {
        return 0U;
    }

    McanRecoverIfNeeded();

    if (MCAN_GetStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_NEW_MSG) != SET)
    {
        return 0U;
    }

    MCAN_ClearStatus(MCAN_UNIT, MCAN_FLAG_RX_FIFO1_NEW_MSG);
    if (MCAN_GetRxMsg(MCAN_UNIT, MCAN_RX_FIFO1, &msg) != LL_OK)
    {
        return 0U;
    }

    if ((msg.IDE != 1U) || (msg.DLC != MCAN_DLC8))
    {
        return 0U;
    }

    *id = msg.ID;
    memcpy(data, &msg.au8Data[0], 8U);
    return 1U;
}
