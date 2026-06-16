#include "bsp_dma.h"
#include "bsp_usart.h"

stc_dma_llp_descriptor_t Usart1_tx_List = {0}; //串口1发送DMA链表
stc_dma_llp_descriptor_t Usart1_Rx_List = {0}; //串口1接收DMA链表

stc_dma_llp_descriptor_t Usart2_tx_List = {0}; //串口2发送DMA链表
stc_dma_llp_descriptor_t Usart2_Rx_List = {0}; //串口2接收DMA链表


uint32_t Temp_Null_Array[2] = {0}; //占位用，无实际意义，用于DMA的传输停止




//DMA资源分配定义--Usart_Tx1
DMA_INFO_Str Dma_Usart_Tx1_Info = 
{
    USART1_TX_DMA_UNIT         ,
    USART1_TX_DMA_CH           ,
};

//DMA资源分配定义--Usart_Rx1
DMA_INFO_Str Dma_Usart_Rx1_Info = 
{
    USART1_RX_DMA_UNIT           ,
    USART1_RX_DMA_CH             ,
};

//DMA资源分配定义--Usart_Tx1
DMA_INFO_Str Dma_Usart_Tx2_Info = 
{
    USART2_TX_DMA_UNIT         ,
    USART2_TX_DMA_CH           ,
};

//DMA资源分配定义--Usart_Rx1
DMA_INFO_Str Dma_Usart_Rx2_Info = 
{
    USART2_RX_DMA_UNIT           ,
    USART2_RX_DMA_CH             ,
};
//串口接收链表
void DMA_Usart_Rx1_List_Init(void)
{
    //使能中断、使能链表传输、更新链表后等待触发、数据宽度8bit、无重复、无不连续传输、目的地址递增，源地址固定
    Usart1_Rx_List.CHCTLx = DMA_INT_ENABLE | DMA_LLP_ENABLE | DMA_LLP_WAIT | DMA_DATAWIDTH_8BIT \
                           |DMA_RPT_NONE | DMA_NON_SEQ_NONE | DMA_DEST_ADDR_INC | DMA_SRC_ADDR_FIX;  
    Usart1_Rx_List.SARx = (uint32_t)(&USART1_PORT->RDR); //源地址
    Usart1_Rx_List.DARx = (uint32_t)usart0_rx_buffer; //目的地址
    Usart1_Rx_List.DTCTLx = ((Usart1_RxBuff_Size<<16) | 1); //blocksize:1，stranscnt：Usart1_RxBuff_Size
    Usart1_Rx_List.RPTx = 0; //源地址和目的地址都不重复
    Usart1_Rx_List.SNSEQCTLx = 0; //源地址非连续传输
    Usart1_Rx_List.DNSEQCTLx = 0; //目的地址非连续传输
    Usart1_Rx_List.LLPx = (uint32_t)(&Usart1_Rx_List); //下一个链表地址，指回首地址
}

void DMA_Usart_Rx2_List_Init(void)
{
    //使能中断、使能链表传输、更新链表后等待触发、数据宽度8bit、无重复、无不连续传输、目的地址递增，源地址固定
    Usart2_Rx_List.CHCTLx = DMA_INT_ENABLE | DMA_LLP_ENABLE | DMA_LLP_WAIT | DMA_DATAWIDTH_8BIT \
                           |DMA_RPT_NONE | DMA_NON_SEQ_NONE | DMA_DEST_ADDR_INC | DMA_SRC_ADDR_FIX;  
    Usart2_Rx_List.SARx = (uint32_t)(&USART2_PORT->RDR); //源地址
    Usart2_Rx_List.DARx = (uint32_t)usart2_rx_buffer; //目的地址
    Usart2_Rx_List.DTCTLx = ((Usart2_RxBuff_Size<<16) | 1); //blocksize:1，stranscnt：Usart1_RxBuff_Size
    Usart2_Rx_List.RPTx = 0; //源地址和目的地址都不重复
    Usart2_Rx_List.SNSEQCTLx = 0; //源地址非连续传输
    Usart2_Rx_List.DNSEQCTLx = 0; //目的地址非连续传输
    Usart2_Rx_List.LLPx = (uint32_t)(&Usart2_Rx_List); //下一个链表地址，指回首地址
}

//串口2发送DMA初始化
void BSP_DMA_USART1TX_Init(void)
{
    stc_dma_init_t stcDmaInit;
//    stc_dma_llp_init_t stcDmaLlpInit;

//    Dma_Usart_Tx1_List_Init();
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_DMA, ENABLE);
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);

   
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32IntEn = DMA_INT_DISABLE;
    stcDmaInit.u32BlockSize = 1UL;
    stcDmaInit.u32TransCount = 1;
    stcDmaInit.u32DataWidth = DMA_DATAWIDTH_8BIT;
    stcDmaInit.u32DestAddr = (uint32_t)(&USART1_PORT->TDR);
    stcDmaInit.u32SrcAddr =  0;
    stcDmaInit.u32SrcAddrInc = DMA_SRC_ADDR_INC;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_FIX;
    DMA_Init(USART1_TX_DMA_UNIT, USART1_TX_DMA_CH, &stcDmaInit);
    
    AOS_SetTriggerEventSrc(AOS_DMA_0, EVT_SRC_USART1_TCI); //触发源为 串口发送完成，每发送完一个字节触发一次 
}

//串口2发送DMA初始化
void BSP_DMA_USART2TX_Init(void)
{
    stc_dma_init_t stcDmaInit;
//    stc_dma_llp_init_t stcDmaLlpInit;

//    Dma_Usart_Tx1_List_Init();
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_DMA, ENABLE);
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);

   
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32IntEn = DMA_INT_DISABLE;
    stcDmaInit.u32BlockSize = 1UL;
    stcDmaInit.u32TransCount = 1;
    stcDmaInit.u32DataWidth = DMA_DATAWIDTH_8BIT;
    stcDmaInit.u32DestAddr = (uint32_t)(&USART2_PORT->TDR);
    stcDmaInit.u32SrcAddr =  0;
    stcDmaInit.u32SrcAddrInc = DMA_SRC_ADDR_INC;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_FIX;
    DMA_Init(USART2_TX_DMA_UNIT, USART2_TX_DMA_CH, &stcDmaInit);
    
    AOS_SetTriggerEventSrc(AOS_DMA_2, EVT_SRC_USART2_TCI); //触发源为 串口发送完成，每发送完一个字节触发一次 
}


//串口接收DMA初始化
void BSP_DMA_USART1RX_Init(void)
{
    stc_dma_init_t stcDmaInit;
    stc_dma_llp_init_t stcDmaLlpInit;

    DMA_Usart_Rx1_List_Init();

    /* DMA&AOS FCG enable */
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_DMA, ENABLE);
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);

    /* USART_RX_DMA 注意与链表配置保持一致 */
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32IntEn = DMA_INT_DISABLE;
    stcDmaInit.u32BlockSize = 1UL;
    stcDmaInit.u32TransCount = Usart1_RxBuff_Size;
    stcDmaInit.u32DataWidth = DMA_DATAWIDTH_8BIT;
    stcDmaInit.u32DestAddr = Usart1_Rx_List.DARx;
    stcDmaInit.u32SrcAddr =  Usart1_Rx_List.SARx;
    stcDmaInit.u32SrcAddrInc = DMA_SRC_ADDR_FIX;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_INC;
    DMA_Init(USART1_RX_DMA_UNIT, USART1_RX_DMA_CH, &stcDmaInit);

    (void)DMA_LlpStructInit(&stcDmaLlpInit);

    stcDmaLlpInit.u32State = DMA_LLP_ENABLE;
    stcDmaLlpInit.u32Mode  = DMA_LLP_WAIT;
    stcDmaLlpInit.u32Addr  = (uint32_t)&Usart1_Rx_List;   

    (void)DMA_LlpInit(USART1_RX_DMA_UNIT, USART1_RX_DMA_CH, &stcDmaLlpInit);
    AOS_SetTriggerEventSrc(AOS_DMA_1, EVT_SRC_USART1_RI);
}

//串口接收DMA初始化
void BSP_DMA_USART2RX_Init(void)
{
    stc_dma_init_t stcDmaInit;
    stc_dma_llp_init_t stcDmaLlpInit;

    DMA_Usart_Rx2_List_Init();

    /* DMA&AOS FCG enable */
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_DMA, ENABLE);
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);

    /* USART_RX_DMA 注意与链表配置保持一致 */
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32IntEn = DMA_INT_DISABLE;
    stcDmaInit.u32BlockSize = 1UL;
    stcDmaInit.u32TransCount = Usart2_RxBuff_Size;
    stcDmaInit.u32DataWidth = DMA_DATAWIDTH_8BIT;
    stcDmaInit.u32DestAddr = Usart2_Rx_List.DARx;
    stcDmaInit.u32SrcAddr =  Usart2_Rx_List.SARx;
    stcDmaInit.u32SrcAddrInc = DMA_SRC_ADDR_FIX;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_INC;
    DMA_Init(USART2_RX_DMA_UNIT, USART2_RX_DMA_CH, &stcDmaInit);

    (void)DMA_LlpStructInit(&stcDmaLlpInit);

    stcDmaLlpInit.u32State = DMA_LLP_ENABLE;
    stcDmaLlpInit.u32Mode  = DMA_LLP_WAIT;
    stcDmaLlpInit.u32Addr  = (uint32_t)&Usart2_Rx_List;   

    (void)DMA_LlpInit(USART2_RX_DMA_UNIT, USART2_RX_DMA_CH, &stcDmaLlpInit);
    AOS_SetTriggerEventSrc(AOS_DMA_3, EVT_SRC_USART2_RI);
}
//所有用到的DMA单元和通道使能
void Dma_Enable(void)
{
    DMA_Cmd(CM_DMA, ENABLE);
    DMA_ChCmd(USART1_RX_DMA_UNIT, USART1_RX_DMA_CH, ENABLE); //使能DMA通道
    DMA_ChCmd(USART2_RX_DMA_UNIT, USART2_RX_DMA_CH, ENABLE); //使能DMA通道

}

//static void Dma_Delay(uint32_t u32Delay)
//{
//    volatile uint32_t i;
//    for (i = 0; i < u32Delay; i++)
//    {
//        // 空循环，等待延时
//    }
//}

//uint8_t Wait_DMA_Ready(CM_DMA_TypeDef *DMAx, uint8_t u8Ch,uint32_t ADDR)
//{
//    /* 设置超时计数器初始值 */
//    uint32_t Timeout = 0xfff0;

//    /* 循环检查DMA LLP地址是否到达目标地址 */
//    while (DMA_GetDestAddr(DMAx, u8Ch) == ADDR)
//    {
//        /* 每次循环等待10个时钟周期 */
//        Dma_Delay(10);
//        
//        /* 超时检测与处理 */
//        if (Timeout-- == 0)
//        {
//            return 0;
//        }
//    }

//    return 1;
//}

uint8_t DMA_Get_Chen( uint8_t u8Ch)
{
    uint32_t CHEN_REG = 0;
    CHEN_REG = READ_REG32(CM_DMA->CHEN);
    if(CHEN_REG & (1<<u8Ch))
    {
        return 1;
    }
    return 0;
}





void bsp_dma_init(void)
{
    BSP_DMA_USART1TX_Init();    //串口1发送   DMA初始化
    BSP_DMA_USART1RX_Init();    //串口1接收   DMA初始化
    BSP_DMA_USART2TX_Init();    //串口1发送   DMA初始化
    BSP_DMA_USART2RX_Init();    //串口1接收   DMA初始化
    
    Dma_Enable();               //使能DMA
}
