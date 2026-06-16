/**
 *******************************************************************************
 * @file  ev_hc32f334_lqfp64.h
 * @brief This file contains all the functions prototypes of the
 *        EV_HC32F334_LQFP64 BSP driver library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2024-01-15       CDT             First version
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
#ifndef __EV_HC32F334_LQFP64_H__
#define __EV_HC32F334_LQFP64_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_aos.h"
#include "hc32_ll_clk.h"
#include "hc32_ll_dma.h"
#include "hc32_ll_efm.h"
#include "hc32_ll_fcg.h"
#include "hc32_ll_fcm.h"
#include "hc32_ll_gpio.h"
#include "hc32_ll_i2c.h"
#include "hc32_ll_interrupts.h"
#include "hc32_ll_pwc.h"
#include "hc32_ll_spi.h"
#include "hc32_ll_sram.h"
#include "hc32_ll_usart.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup BSP
 * @{
 */

/**
 * @addtogroup EV_HC32F334_LQFP64
 * @{
 */

/**
 * @addtogroup EV_HC32F334_LQFP64_BASE
 * @{
 */

#if (BSP_EV_HC32F334_LQFP64 == BSP_EV_HC32F3XX)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup EV_HC32F334_LQFP64_Global_Macros EV_HC32F334_LQFP64 Global Macros
 * @{
 */

/**
 * @defgroup EV_HC32F334_LQFP64_I2C_Configuration EV_HC32F334_LQFP64 I2C Configuration
 * @{
 */
#define BSP_I2C_BAUDRATE        (100000UL)
#define BSP_I2C_TIMEOUT         (0x40000U)
/**
 * @}
 */

/**
 * @defgroup EV_HC32F334_LQFP64_LED_Sel EV_HC32F334_LQFP64 LED definition
 * @{
 */
#define LED_RED                 (0x01U)
#define LED_BLUE                (0x02U)
#define LED_ALL                 (LED_RED | LED_BLUE)
/**
 * @}
 */

/**
 * @defgroup EV_HC32F334_LQFP64_KEY_Sel EV_HC32F334_LQFP64 KEY definition
 * @{
 */
#define BSP_KEY_1               (0x01UL)    /*!< BSP KEY 1 */
#define BSP_KEY_2               (0x02UL)    /*!< BSP KEY 2 */
#define BSP_KEY_3               (0x04UL)    /*!< BSP KEY 3 */
/**
 * @}
 */

/**
 * @defgroup EV_HC32F334_LQFP64_KEY_MD_Sel EV_HC32F334_LQFP64 KEY work mode definition
 * @{
 */
#define BSP_KEY_MD_GPIO         (0x00U)
#define BSP_KEY_MD_EXTINT       (0x01U)
/**
 * @}
 */

/**
 * @defgroup EV_HC32F334_LQFP64_LED_Number EV_HC32F334_LQFP64 LED Number
 * @{
 */
#define BSP_LED_NUM             (2U)
/**
 * @}
 */

/**
 * @defgroup EV_HC32F334_LQFP64_LED_PortPin_Sel EV_HC32F334_LQFP64 LED port/pin definition
 * @{
 */
#define BSP_LED_RED_PORT        (GPIO_PORT_F)
#define BSP_LED_RED_PIN         (GPIO_PIN_02)
#define BSP_LED_BLUE_PORT       (GPIO_PORT_C)
#define BSP_LED_BLUE_PIN        (GPIO_PIN_13)
/**
 * @}
 */



/**
 * @defgroup EV_HC32F334_LQFP64_KEY_Number EV_HC32F334_LQFP64 KEY Number
 * @{
 */
#define BSP_KEY_NUM             (3U)
/**
 * @}
 */

/**
 * @defgroup EV_HC32F334_LQFP64_KEY_PortPin_Sel EV_HC32F334_LQFP64 KEY port/pin definition
 * @{
 */
#define BSP_KEY1_PORT           (GPIO_PORT_C)
#define BSP_KEY1_PIN            (GPIO_PIN_03)
#define BSP_KEY2_PORT           (GPIO_PORT_C)
#define BSP_KEY2_PIN            (GPIO_PIN_00)
#define BSP_KEY3_PORT           (GPIO_PORT_B)
#define BSP_KEY3_PIN            (GPIO_PIN_09)
/**
 * @}
 */

/**
 * @defgroup EV_HC32F334_LQFP64_KEY_INT_PortPin_Sel EV_HC32F334_LQFP64 KEY interrupt definition
 * @{
 */
#define BSP_KEY1_INT_CH         (EXTINT_CH03)
#define BSP_KEY1_INT_IRQn       (EXTINT_PORT_EIRQ3_IRQn)
#define BSP_KEY1_INT_WAKEUP     (INTC_STOP_WKUP_EXTINT_CH3)
#define BSP_KEY1_INT_EVT        (EVT_SRC_PORT_EIRQ3)

#define BSP_KEY2_INT_CH         (EXTINT_CH00)
#define BSP_KEY2_INT_IRQn       (EXTINT_PORT_EIRQ0_IRQn)
#define BSP_KEY3_INT_CH         (EXTINT_CH09)
#define BSP_KEY3_INT_IRQn       (EXTINT_PORT_EIRQ9_IRQn)
/* KEY Dithering Elimination */
#define BSP_KEY_DELAY_MS        (10UL)
/**
 * @}
 */

/**
 * @defgroup EV_HC32F334_LQFP64_PRINT_CONFIG EV_HC32F334_LQFP64 PRINT Configure definition
 * @{
 */
#define BSP_PRINTF_DEVICE               (CM_USART2)
#define BSP_PRINTF_DEVICE_FCG           (FCG3_PERIPH_USART2)

#define BSP_PRINTF_BAUDRATE             (115200UL)
#define BSP_PRINTF_BAUDRATE_ERR_MAX     (0.025F)

#define BSP_PRINTF_PORT                 (GPIO_PORT_C)
#define BSP_PRINTF_PIN                  (GPIO_PIN_10)
#define BSP_PRINTF_PORT_FUNC            (GPIO_FUNC_36)
/**
 * @}
 */

/**
 * @defgroup EV_HC32F334_LQFP64_XTAL_CONFIG EV_HC32F334_LQFP64 XTAL port/pin definition
 * @{
 */
#define BSP_XTAL_PORT                   (GPIO_PORT_F)
#define BSP_XTAL_IN_PIN                 (GPIO_PIN_00)
#define BSP_XTAL_OUT_PIN                (GPIO_PIN_01)
/**
 * @}
 */

/**
 * @defgroup EV_HC32F334_LQFP64_XTAL32_CONFIG EV_HC32F334_LQFP64 XTAL32 port/pin definition
 * @{
 */
#define BSP_XTAL32_PORT                 (GPIO_PORT_C)
#define BSP_XTAL32_IN_PIN               (GPIO_PIN_14)
#define BSP_XTAL32_OUT_PIN              (GPIO_PIN_15)
/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
  Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup EV_HC32F334_LQFP64_Global_Functions
 * @{
 */
int32_t BSP_XTAL32_Init(void);
void BSP_CLK_Init(void);

void BSP_LED_Init(void);
void BSP_LED_On(uint8_t u8Led);
void BSP_LED_Off(uint8_t u8Led);
void BSP_LED_Toggle(uint8_t u8Led);

void BSP_KEY_Init(uint8_t u8Mode);
en_flag_status_t BSP_KEY_GetStatus(uint32_t u32Key, uint8_t u8Mode);

#if (LL_PRINT_ENABLE == DDL_ON)
int32_t BSP_PRINTF_Preinit(void *vpDevice, uint32_t u32Baudrate);
#endif

/* User Callbacks: User has to implement these functions in his code if they're needed.
   These functions are called when BSP_CLK_Init is initialized to BSP_KEY_MD_EXTINT mode. */
#if (DDL_ON == BSP_INT_KEY_ENABLE)
void BSP_KEY_KEY1_IrqCallback(void);
void BSP_KEY_KEY2_IrqCallback(void);
void BSP_KEY_KEY3_IrqCallback(void);
#endif

#if (LL_I2C_ENABLE == DDL_ON)
int32_t BSP_I2C_Init(CM_I2C_TypeDef *I2Cx);
void BSP_I2C_DeInit(CM_I2C_TypeDef *I2Cx);
int32_t BSP_I2C_Write(CM_I2C_TypeDef *I2Cx, uint16_t u16DevAddr, const uint8_t *pu8Reg, uint8_t u8RegLen, const uint8_t *pu8Buf, uint32_t u32Len);
int32_t BSP_I2C_Read(CM_I2C_TypeDef *I2Cx, uint16_t u16DevAddr, const uint8_t *pu8Reg, uint8_t u8RegLen, uint8_t *pu8Buf, uint32_t u32Len);
int32_t BSP_I2C_GetDevStatus(CM_I2C_TypeDef *I2Cx, uint16_t u16DevAddr);
#endif /* LL_I2C_ENABLE */

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

#ifdef __cplusplus
}
#endif

#endif /* __EV_HC32F334_LQFP64_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
