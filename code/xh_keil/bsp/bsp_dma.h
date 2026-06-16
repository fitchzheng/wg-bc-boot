#ifndef __BSP_DMA_H__
#define __BSP_DMA_H__

#include "hc32_ll.h"

#define USART0_RX_DMA_CNT DMA_GetTransCount(CM_DMA,DMA_CH1)
#define USART2_RX_DMA_CNT DMA_GetTransCount(CM_DMA,DMA_CH3)
#define USARTX_RX_DMA_CNT(ch) DMA_GetTransCount(CM_DMA,ch)

typedef struct
{
    CM_DMA_TypeDef *DMAx;
    uint8_t Dma_Ch;
}DMA_INFO_Str;


//定义使用的DMA通道及相关配置
#define USART1_TX_DMA_UNIT          CM_DMA
#define USART1_TX_DMA_CH            DMA_CH0

#define USART1_RX_DMA_UNIT          CM_DMA
#define USART1_RX_DMA_CH            DMA_CH1

#define USART2_TX_DMA_UNIT          CM_DMA
#define USART2_TX_DMA_CH            DMA_CH2

#define USART2_RX_DMA_UNIT          CM_DMA
#define USART2_RX_DMA_CH            DMA_CH3

//DMA资源分配定义--Usart_Tx1
extern DMA_INFO_Str Dma_Usart_Tx1_Info ;

//DMA资源分配定义--Usart_Rx1
extern DMA_INFO_Str Dma_Usart_Rx1_Info;

//DMA资源分配定义--Usart_Tx1
extern DMA_INFO_Str Dma_Usart_Tx2_Info ;

//DMA资源分配定义--Usart_Rx1
extern DMA_INFO_Str Dma_Usart_Rx2_Info ;


void BSP_DMA_USART1TX_Init(void);
void BSP_DMA_USART1RX_Init(void);
void Dma_Set_TxLenth(uint8_t lenth);
void Dma_Set_TxPtr(uint8_t *ptr);
//void DMA_Ch_Cmd(CM_DMA_TypeDef *DMAx, uint8_t u8Ch, en_functional_state_t enNewState);
uint32_t DMA_Getllp_Addr(CM_DMA_TypeDef *DMAx, uint8_t u8Ch);
uint8_t Wait_DMA_Ready(CM_DMA_TypeDef *DMAx, uint8_t u8Ch,uint32_t ADDR);
uint8_t DMA_Get_Chen( uint8_t u8Ch);

void Dma_Enable(void);

void bsp_dma_init(void);
#endif

