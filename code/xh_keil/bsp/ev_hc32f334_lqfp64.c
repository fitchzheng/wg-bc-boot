/**
 *******************************************************************************
 * @file  ev_hc32f334_lqfp64.c
 * @brief This file provides firmware functions for EV_HC32F334_LQFP64 BSP
 @verbatim
   Change Logs:
   Date             Author          Notes
   2024-01-15       CDT             First version
   2024-06-30       CDT             Enable cache in API BSP_CLK_Init()
                                    MISRAC fix in API BSP_XTAL32_Init()
                                    FCM interface add instance
                                    Comments fixed in API BSP_CLK_Init()
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2024, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "ev_hc32f334_lqfp64.h"

/**
 * @defgroup BSP BSP
 * @{
 */

/**
 * @defgroup EV_HC32F334_LQFP64 EV_HC32F334_LQFP64
 * @{
 */

/**
 * @defgroup EV_HC32F334_LQFP64_BASE EV_HC32F334_LQFP64 Base
 * @{
 */

#if (BSP_EV_HC32F334_LQFP64 == BSP_EV_HC32F3XX)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup EV_HC32F334_LQFP64_Local_Types EV_HC32F334_LQFP64 Local Types
 * @{
 */
typedef struct {
    uint8_t port;
    uint16_t pin;
} BSP_Port_Pin;

typedef struct {
    uint8_t      port;
    uint16_t     pin;
#if (DDL_ON == BSP_INT_KEY_ENABLE)
    uint32_t     ch;
    IRQn_Type    irq;
#endif
} BSP_Key_Config;
/**
 * @}
 */

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/**
* @defgroup EV_HC32F334_LQFP64_Local_Variables EV_HC32F334_LQFP64 Local Variables
* @{
*/
static const BSP_Port_Pin BSP_LED_PORT_PIN[BSP_LED_NUM] = {
    {BSP_LED_RED_PORT,  BSP_LED_RED_PIN},
    {BSP_LED_BLUE_PORT, BSP_LED_BLUE_PIN},
};

static const BSP_Key_Config BSP_KEY_PORT_PIN[BSP_KEY_NUM] = {
#if (DDL_ON == BSP_INT_KEY_ENABLE)
    {BSP_KEY1_PORT, BSP_KEY1_PIN, BSP_KEY1_INT_CH, BSP_KEY1_INT_IRQn},
    {BSP_KEY2_PORT, BSP_KEY2_PIN, BSP_KEY2_INT_CH, BSP_KEY2_INT_IRQn},
    {BSP_KEY3_PORT, BSP_KEY3_PIN, BSP_KEY3_INT_CH, BSP_KEY3_INT_IRQn},
#else
    {BSP_KEY1_PORT, BSP_KEY1_PIN},
    {BSP_KEY2_PORT, BSP_KEY2_PIN},
    {BSP_KEY3_PORT, BSP_KEY3_PIN},
#endif
};

static uint32_t m_u32GlobalKey = 0UL;
/**
 * @}
 */

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @defgroup EV_HC32F334_LQFP64_Global_Functions EV_HC32F334_LQFP64 Global Functions
 * @{
 */

#if (LL_I2C_ENABLE == DDL_ON)
/**
 * @brief  BSP I2C initialize
 * @param  [in] I2Cx                Pointer to I2C instance register base.
 *                                  This parameter can be a value of the following:
 *         @arg CM_I2Cx:            I2C instance register base.
 * @retval int32_t:
 *            - LL_OK:              Configure success
 *            - LL_ERR_INVD_PARAM:  Invalid parameter
 */
int32_t BSP_I2C_Init(CM_I2C_TypeDef *I2Cx)
{
    int32_t i32Ret;
    float32_t fErr;
    stc_i2c_init_t stcI2cInit;
    uint32_t I2cSrcClk;
    uint32_t I2cClkDiv;
    uint32_t I2cClkDivReg;

    I2cSrcClk = I2C_SRC_CLK;
    I2cClkDiv = I2cSrcClk / BSP_I2C_BAUDRATE / I2C_WIDTH_MAX_IMME;
    for (I2cClkDivReg = I2C_CLK_DIV1; I2cClkDivReg <= I2C_CLK_DIV128; I2cClkDivReg++) {
        if (I2cClkDiv < (1UL << I2cClkDivReg)) {
            break;
        }
    }

    (void)I2C_DeInit(I2Cx);
    (void)I2C_StructInit(&stcI2cInit);
    stcI2cInit.u32Baudrate = BSP_I2C_BAUDRATE;
    stcI2cInit.u32SclTime  = (uint32_t)((uint64_t)250UL * ((uint64_t)I2cSrcClk / ((uint64_t)1UL << I2cClkDivReg)) / (uint64_t)1000000000UL);  /* SCL time is about 250nS in EVB board */
    stcI2cInit.u32ClockDiv = I2cClkDivReg;
    i32Ret = I2C_Init(I2Cx, &stcI2cInit, &fErr);

    if (LL_OK == i32Ret) {
        I2C_BusWaitCmd(I2Cx, ENABLE);
        I2C_Cmd(I2Cx, ENABLE);
    }

    return i32Ret;
}

/**
 * @brief  BSP I2C De-initialize
 * @param  [in] I2Cx                Pointer to I2C instance register base.
 *                                  This parameter can be a value of the following:
 *         @arg CM_I2Cx:            I2C instance register base.
 * @retval None
 */
void BSP_I2C_DeInit(CM_I2C_TypeDef *I2Cx)
{
    (void)I2C_DeInit(I2Cx);
}

/**
 * @brief  BSP I2C write.
 * @param  [in] I2Cx                Pointer to I2C instance register base.
 *                                  This parameter can be a value of the following:
 *         @arg CM_I2Cx:            I2C instance register base.
 * @param  [in] u16DevAddr:         Device address.
 * @param  [in] pu8Reg:             Pointer to the register address or memory address.
 * @param  [in] u8RegLen:           Length of register address or memory address.
 * @param  [in] pu8Buf:             The pointer to the buffer contains the data to be write.
 * @param  [in] u32Len:             Buffer size in byte.
 * @retval int32_t:
 *            - LL_OK:              Success
 *            - LL_ERR:             Receive NACK
 *            - LL_ERR_TIMEOUT:     Timeout
 *            - LL_ERR_INVD_PARAM:  pu8Buf is NULL
 */
int32_t BSP_I2C_Write(CM_I2C_TypeDef *I2Cx, uint16_t u16DevAddr, const uint8_t *pu8Reg, uint8_t u8RegLen, const uint8_t *pu8Buf, uint32_t u32Len)
{
    int32_t i32Ret;

    I2C_SWResetCmd(I2Cx, ENABLE);
    I2C_SWResetCmd(I2Cx, DISABLE);
    i32Ret = I2C_Start(I2Cx, BSP_I2C_TIMEOUT);
    if (LL_OK == i32Ret) {
        i32Ret = I2C_TransAddr(I2Cx, u16DevAddr, I2C_DIR_TX, BSP_I2C_TIMEOUT);

        if (LL_OK == i32Ret) {
            i32Ret = I2C_TransData(I2Cx, pu8Reg, u8RegLen, BSP_I2C_TIMEOUT);
            if (LL_OK == i32Ret) {
                i32Ret = I2C_TransData(I2Cx, pu8Buf, u32Len, BSP_I2C_TIMEOUT);
            }
        }
    }
    (void)I2C_Stop(I2Cx, BSP_I2C_TIMEOUT);
    return i32Ret;
}

/**
 * @brief  BSP I2C read.
 * @param  [in] I2Cx                Pointer to I2C instance register base.
 *                                  This parameter can be a value of the following:
 *         @arg CM_I2Cx:            I2C instance register base.
 * @param  [in] u16DevAddr:         Device address.
 * @param  [in] pu8Reg:             Pointer to the register address or memory address.
 * @param  [in] u8RegLen:           Length of register address or memory address.
 * @param  [in] pu8Buf:             The pointer to the buffer contains the data to be read.
 * @param  [in] u32Len:             Buffer size in byte.
 * @retval int32_t:
 *            - LL_OK:              Success
 *            - LL_ERR:             Receive NACK
 *            - LL_ERR_TIMEOUT:     Timeout
 *            - LL_ERR_INVD_PARAM:  pu8Buf is NULL
 */
int32_t BSP_I2C_Read(CM_I2C_TypeDef *I2Cx, uint16_t u16DevAddr, const uint8_t *pu8Reg, uint8_t u8RegLen, uint8_t *pu8Buf, uint32_t u32Len)
{
    int32_t i32Ret;
    I2C_SWResetCmd(I2Cx, ENABLE);
    I2C_SWResetCmd(I2Cx, DISABLE);
    i32Ret = I2C_Start(I2Cx, BSP_I2C_TIMEOUT);
    if (LL_OK == i32Ret) {
        i32Ret = I2C_TransAddr(I2Cx, u16DevAddr, I2C_DIR_TX, BSP_I2C_TIMEOUT);

        if (LL_OK == i32Ret) {
            i32Ret = I2C_TransData(I2Cx, pu8Reg, u8RegLen, BSP_I2C_TIMEOUT);
            if (LL_OK == i32Ret) {
                i32Ret = I2C_Restart(I2Cx, BSP_I2C_TIMEOUT);
                if (LL_OK == i32Ret) {
                    if (1UL == u32Len) {
                        I2C_AckConfig(I2Cx, I2C_NACK);
                    }

                    i32Ret = I2C_TransAddr(I2Cx, u16DevAddr, I2C_DIR_RX, BSP_I2C_TIMEOUT);
                    if (LL_OK == i32Ret) {
                        i32Ret = I2C_MasterReceiveDataAndStop(I2Cx, pu8Buf, u32Len, BSP_I2C_TIMEOUT);
                    }
                    I2C_AckConfig(I2Cx, I2C_ACK);
                }
            }
        }
    }

    if (LL_OK != i32Ret) {
        (void)I2C_Stop(I2Cx, BSP_I2C_TIMEOUT);
    }

    return i32Ret;
}

/**
 * @brief  BSP 24CXX status get.
 * @param  [in] I2Cx                Pointer to I2C instance register base.
 *                                  This parameter can be a value of the following:
 *         @arg CM_I2Cx:            I2C instance register base.
 * @param  [in] u16DevAddr:         Device address.
 * @retval int32_t:
 *            - LL_OK:              Idle
 *            - LL_ERR:             Receive NACK
 *            - LL_ERR_TIMEOUT:     Timeout
 *            - LL_ERR_INVD_PARAM:  pu8Buf is NULL
 */
int32_t BSP_I2C_GetDevStatus(CM_I2C_TypeDef *I2Cx, uint16_t u16DevAddr)
{
    int32_t i32Ret;

    i32Ret = I2C_Start(I2Cx, BSP_I2C_TIMEOUT);
    if (LL_OK == i32Ret) {
        i32Ret = I2C_TransAddr(I2Cx, u16DevAddr, I2C_DIR_TX, BSP_I2C_TIMEOUT);

        if (LL_OK == i32Ret) {
            if (SET == I2C_GetStatus(I2Cx, I2C_FLAG_ACKR)) {
                i32Ret = LL_ERR;
            }
        }
    }
    (void)I2C_Stop(I2Cx, BSP_I2C_TIMEOUT);
    return i32Ret;
}
#endif /* LL_I2C_ENABLE */

/**
 * @brief  BSP clock initialize.
 *         SET board system clock to PLLH@120MHz
 *         Flash: 2 wait
 *         PCLK0: 120MHz
 *         PCLK1: 60MHz
 *         PCLK2: 60MHz
 *         PCLK3: 60MHz
 *         PCLK4: 60MHz
 *         HCLK:  120MHz
 * @param  None
 * @retval None
 */
__WEAKDEF void BSP_CLK_Init(void)
{
    stc_clock_xtal_init_t stcXtalInit;
    stc_clock_pll_init_t stcPLLHInit;

    /* PCLK0, HCLK  Max 120MHz */
    /* PCLK1, PCLK2, PCLK3, PCLK4 Max 60MHz */
    CLK_SetClockDiv(CLK_BUS_CLK_ALL,
                    (CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 | CLK_PCLK2_DIV2 |
                     CLK_PCLK3_DIV2 | CLK_PCLK4_DIV2 | CLK_HCLK_DIV1));

    GPIO_AnalogCmd(BSP_XTAL_PORT, BSP_XTAL_IN_PIN | BSP_XTAL_OUT_PIN, ENABLE);
    (void)CLK_XtalStructInit(&stcXtalInit);
    /* Config Xtal and enable Xtal */
    stcXtalInit.u8Mode   = CLK_XTAL_MD_OSC;
    stcXtalInit.u8Drv    = CLK_XTAL_DRV_ULOW;
    stcXtalInit.u8State  = CLK_XTAL_ON;
    stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
    (void)CLK_XtalInit(&stcXtalInit);

    (void)CLK_PLLStructInit(&stcPLLHInit);
    /* VCO = (8/1)*60 = 480MHz*/
    stcPLLHInit.u8PLLState = CLK_PLL_ON;
    stcPLLHInit.PLLCFGR = 0UL;
    stcPLLHInit.PLLCFGR_f.PLLM = 1UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLN = 60UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLP = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLQ = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLR = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    (void)CLK_PLLInit(&stcPLLHInit);

    /* 2 cycles for 100 ~ 120MHz */
    (void)EFM_SetWaitCycle(EFM_WAIT_CYCLE2);
    /* 2 cycles for 100 ~ 120MHz */
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT2);
    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_PLL);

    /* Reset cache ram */
    EFM_CacheRamReset(ENABLE);
    EFM_CacheRamReset(DISABLE);
    /* Enable cache */
    EFM_PrefetchCmd(ENABLE);
    EFM_DCacheCmd(ENABLE);
    EFM_ICacheCmd(ENABLE);
}

/**
 * @brief  BSP Xtal32 initialize.
 * @param  None
 * @retval int32_t:
 *         - LL_OK: XTAL32 enable successfully
 *         - LL_ERR_TIMEOUT: XTAL32 enable timeout.
 */
__WEAKDEF int32_t BSP_XTAL32_Init(void)
{
    stc_clock_xtal32_init_t stcXtal32Init;
    stc_fcm_init_t stcFcmInit;
    uint32_t u32TimeOut = 0UL;
    uint32_t u32Time = HCLK_VALUE / 5UL;

    if (CLK_XTAL32_ON == READ_REG8(CM_CMU->XTAL32CR)) {
        /* Disable xtal32 */
        (void)CLK_Xtal32Cmd(DISABLE);
    }

    /* Xtal32 config */
    (void)CLK_Xtal32StructInit(&stcXtal32Init);
    stcXtal32Init.u8State  = CLK_XTAL32_ON;
    stcXtal32Init.u8Drv    = CLK_XTAL32_DRV_MID;
    stcXtal32Init.u8Filter = CLK_XTAL32_FILTER_ALL_MD;
    GPIO_AnalogCmd(BSP_XTAL32_PORT, BSP_XTAL32_IN_PIN | BSP_XTAL32_OUT_PIN, ENABLE);
    (void)CLK_Xtal32Init(&stcXtal32Init);

    /* FCM config */
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_FCM, ENABLE);
    (void)FCM_StructInit(&stcFcmInit);
    stcFcmInit.u32RefClock       = FCM_REF_CLK_MRC;
    stcFcmInit.u32RefClockDiv    = FCM_REF_CLK_DIV8192;
    stcFcmInit.u32RefClockEdge   = FCM_REF_CLK_RISING;
    stcFcmInit.u32TargetClock    = FCM_TARGET_CLK_XTAL32;
    stcFcmInit.u32TargetClockDiv = FCM_TARGET_CLK_DIV1;
    stcFcmInit.u16LowerLimit     = (uint16_t)((XTAL32_VALUE / (MRC_VALUE / 8192U)) * 96UL / 100UL);
    stcFcmInit.u16UpperLimit     = (uint16_t)((XTAL32_VALUE / (MRC_VALUE / 8192U)) * 104UL / 100UL);
    (void)FCM_Init(CM_FCM, &stcFcmInit);
    /* Enable FCM, to ensure xtal32 stable */
    FCM_Cmd(CM_FCM, ENABLE);
    for (;;) {
        if (SET == FCM_GetStatus(CM_FCM, FCM_FLAG_END)) {
            FCM_ClearStatus(CM_FCM, FCM_FLAG_END);
            if (SET == FCM_GetStatus(CM_FCM, FCM_FLAG_ERR | FCM_FLAG_OVF)) {
                FCM_ClearStatus(CM_FCM, FCM_FLAG_ERR | FCM_FLAG_OVF);
            } else {
                (void)FCM_DeInit(CM_FCM);
                FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_FCM, DISABLE);
                return LL_OK;
            }
        }
        u32TimeOut++;
        if (u32TimeOut > u32Time) {
            (void)FCM_DeInit(CM_FCM);
            FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_FCM, DISABLE);
            return LL_ERR_TIMEOUT;
        }
    }
}

/**
 * @brief  LED initialize.
 * @param  None
 * @retval None
 */
void BSP_LED_Init(void)
{
    uint8_t i;
    stc_gpio_init_t stcGpioInit;

    /* Initialize GPIO structure */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDir   = PIN_DIR_OUT;
    stcGpioInit.u16PinState = PIN_STAT_SET;
    /* Initialize RGB LED pin */
    for (i = 0U; i < BSP_LED_NUM; i++) {
        (void)GPIO_Init(BSP_LED_PORT_PIN[i].port, BSP_LED_PORT_PIN[i].pin, &stcGpioInit);
    }
}

/**
 * @brief  Turn on LEDs.
 * @param  [in] u8Led                   LED @ref EV_HC32F334_LQFP64_LED_Sel
 * @retval None
 */
void BSP_LED_On(uint8_t u8Led)
{
    uint8_t i;

    for (i = 0U; i < BSP_LED_NUM; i++) {
        if (0U != ((u8Led >> i) & 1U)) {
            GPIO_ResetPins(BSP_LED_PORT_PIN[i].port, BSP_LED_PORT_PIN[i].pin);
        }
    }
}

/**
 * @brief  Turn off LEDs.
 * @param  [in] u8Led                   LED @ref EV_HC32F334_LQFP64_LED_Sel
 * @retval None
 */
void BSP_LED_Off(uint8_t u8Led)
{
    uint8_t i;

    for (i = 0U; i < BSP_LED_NUM; i++) {
        if (0U != ((u8Led >> i) & 1U)) {
            GPIO_SetPins(BSP_LED_PORT_PIN[i].port, BSP_LED_PORT_PIN[i].pin);
        }
    }
}

/**
 * @brief  Toggle LEDs.
 * @param  [in] u8Led                   LED @ref EV_HC32F334_LQFP64_LED_Sel
 * @retval None
 */
void BSP_LED_Toggle(uint8_t u8Led)
{
    uint8_t i;

    for (i = 0U; i < BSP_LED_NUM; i++) {
        if (0U != ((u8Led >> i) & 1U)) {
            GPIO_TogglePins(BSP_LED_PORT_PIN[i].port, BSP_LED_PORT_PIN[i].pin);
        }
    }
}

#if (LL_PRINT_ENABLE == DDL_ON)
/**
 * @brief  BSP printf device, clock and port pre-initialize.
 * @param  [in] vpDevice                Pointer to print device
 * @param  [in] u32Baudrate             Print device communication baudrate
 * @retval int32_t:
 *           - LL_OK:                   Initialize successfully.
 *           - LL_ERR:                  Initialize unsuccessfully.
 *           - LL_ERR_INVD_PARAM:       The u32Baudrate value is 0.
 */
int32_t BSP_PRINTF_Preinit(void *vpDevice, uint32_t u32Baudrate)
{
    uint32_t u32Div;
    float32_t f32Error;
    stc_usart_uart_init_t stcUartInit;
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    (void)vpDevice;

    if (0UL != u32Baudrate) {
        /* Set TX port function */
        GPIO_SetFunc(BSP_PRINTF_PORT, BSP_PRINTF_PIN, BSP_PRINTF_PORT_FUNC);

        /* Enable clock  */
        FCG_Fcg3PeriphClockCmd(BSP_PRINTF_DEVICE_FCG, ENABLE);

        /* Configure UART */
        (void)USART_UART_StructInit(&stcUartInit);
        stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
        (void)USART_UART_Init(BSP_PRINTF_DEVICE, &stcUartInit, NULL);

        for (u32Div = 0UL; u32Div <= USART_CLK_DIV64; u32Div++) {
            USART_SetClockDiv(BSP_PRINTF_DEVICE, u32Div);
            i32Ret = USART_SetBaudrate(BSP_PRINTF_DEVICE, u32Baudrate, &f32Error);
            if ((LL_OK == i32Ret) && \
                ((-BSP_PRINTF_BAUDRATE_ERR_MAX <= f32Error) && (f32Error <= BSP_PRINTF_BAUDRATE_ERR_MAX))) {
                USART_FuncCmd(BSP_PRINTF_DEVICE, USART_TX, ENABLE);
                break;
            } else {
                i32Ret = LL_ERR;
            }
        }
    }

    return i32Ret;
}
#endif

#if (DDL_ON == BSP_INT_KEY_ENABLE)
/**
 * @brief  KEY1 interrupt callback function
 * @param  None
 * @retval None
 */
void EXTINT03_SWINT19_Handler(void)
{
    for (;;) {
        DDL_DelayMS(BSP_KEY_DELAY_MS);
        if (PIN_RESET == GPIO_ReadInputPins(BSP_KEY_PORT_PIN[0].port, BSP_KEY_PORT_PIN[0].pin)) {
            m_u32GlobalKey |= BSP_KEY_1;
        } else {
            EXTINT_ClearExtIntStatus(BSP_KEY_PORT_PIN[0].ch);
            break;
        }
    }
    BSP_KEY_KEY1_IrqCallback();
}

/**
 * @brief  KEY2 interrupt callback function
 * @param  None
 * @retval None
 */
void EXTINT00_SWINT16_Handler(void)
{
    for (;;) {
        DDL_DelayMS(BSP_KEY_DELAY_MS);
        if (PIN_RESET == GPIO_ReadInputPins(BSP_KEY_PORT_PIN[1].port, BSP_KEY_PORT_PIN[1].pin)) {
            m_u32GlobalKey |= BSP_KEY_2;
        } else {
            EXTINT_ClearExtIntStatus(BSP_KEY_PORT_PIN[1].ch);
            break;
        }
    }
    BSP_KEY_KEY2_IrqCallback();
}

/**
 * @brief  KEY3 interrupt callback function
 * @param  None
 * @retval None
 */
void EXTINT09_SWINT25_Handler(void)
{
    for (;;) {
        DDL_DelayMS(BSP_KEY_DELAY_MS);
        if (PIN_RESET == GPIO_ReadInputPins(BSP_KEY_PORT_PIN[2].port, BSP_KEY_PORT_PIN[2].pin)) {
            m_u32GlobalKey |= BSP_KEY_3;
        } else {
            EXTINT_ClearExtIntStatus(BSP_KEY_PORT_PIN[2].ch);
            break;
        }
    }
    BSP_KEY_KEY3_IrqCallback();
}

/**
 * @brief  User callback function for BSP KEY1.
 * @param  None
 * @retval None
 */
__WEAKDEF void BSP_KEY_KEY1_IrqCallback(void)
{
    /* This function should be implemented by the user application. */
}

/**
 * @brief  User callback function for BSP KEY2.
 * @param  None
 * @retval None
 */
__WEAKDEF void BSP_KEY_KEY2_IrqCallback(void)
{
    /* This function should be implemented by the user application. */
}

/**
 * @brief  User callback function for BSP KEY3.
 * @param  None
 * @retval None
 */
__WEAKDEF void BSP_KEY_KEY3_IrqCallback(void)
{
    /* This function should be implemented by the user application. */
}
#endif

/**
 * @brief  BSP key initialize
 * @param  [in] u8Mode Key work mode @ref EV_HC32F334_LQFP64_KEY_MD_Sel
 * @retval None
 */
void BSP_KEY_Init(uint8_t u8Mode)
{
    uint8_t i;
    stc_gpio_init_t stcGpioInit;
#if (DDL_ON == BSP_INT_KEY_ENABLE)
    stc_extint_init_t stcExtIntInit;
#endif

    (void)GPIO_StructInit(&stcGpioInit);
    if (BSP_KEY_MD_GPIO == u8Mode) {
        for (i = 0U; i < BSP_KEY_NUM; i++) {
            (void)GPIO_Init(BSP_KEY_PORT_PIN[i].port, BSP_KEY_PORT_PIN[i].pin, &stcGpioInit);
        }
#if (DDL_ON == BSP_INT_KEY_ENABLE)
    } else if (BSP_KEY_MD_EXTINT == u8Mode) {
        stcGpioInit.u16ExtInt = PIN_EXTINT_ON;
        for (i = 0U; i < BSP_KEY_NUM; i++) {
            (void)GPIO_Init(BSP_KEY_PORT_PIN[i].port, BSP_KEY_PORT_PIN[i].pin, &stcGpioInit);
        }
        /* Extint config */
        (void)EXTINT_StructInit(&stcExtIntInit);
        stcExtIntInit.u32Edge = EXTINT_TRIG_FALLING;
        for (i = 0U; i < BSP_KEY_NUM; i++) {
            EXTINT_ClearExtIntStatus(BSP_KEY_PORT_PIN[i].ch);
            (void)EXTINT_Init(BSP_KEY_PORT_PIN[i].ch, &stcExtIntInit);
        }
        /* NVIC config */
        for (i = 0U; i < BSP_KEY_NUM; i++) {
            NVIC_ClearPendingIRQ(BSP_KEY_PORT_PIN[i].irq);
            NVIC_SetPriority(BSP_KEY_PORT_PIN[i].irq, DDL_IRQ_PRIO_DEFAULT);
            NVIC_EnableIRQ(BSP_KEY_PORT_PIN[i].irq);
        }
    } else {
        ;   /* avoid MISRAC 2012-15.7 */
#endif
    }
}

/**
 * @brief  Get BSP key status
 * @param  [in] u32Key chose one macro from below @ref EV_HC32F334_LQFP64_KEY_Sel
 * @param  [in] u8Mode Key work mode @ref EV_HC32F334_LQFP64_KEY_MD_Sel
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t BSP_KEY_GetStatus(uint32_t u32Key, uint8_t u8Mode)
{
    en_flag_status_t enStatus = RESET;

    if (BSP_KEY_MD_GPIO == u8Mode) {
        DDL_DelayMS(BSP_KEY_DELAY_MS);
        u32Key = __CLZ(__RBIT(u32Key));
        if (PIN_RESET == GPIO_ReadInputPins(BSP_KEY_PORT_PIN[u32Key].port, BSP_KEY_PORT_PIN[u32Key].pin)) {
            enStatus = SET;
        }
    } else if (BSP_KEY_MD_EXTINT == u8Mode) {
        if (0UL != (m_u32GlobalKey & u32Key)) {
            enStatus = SET;
            m_u32GlobalKey &= ~u32Key;
        }
    } else {
        ;   /* avoid MISRAC 2012-15.7 */
    }

    return enStatus;
}

/**
 * @}
 */

#endif /* BSP_EV_HC32F334_LQFP64 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
