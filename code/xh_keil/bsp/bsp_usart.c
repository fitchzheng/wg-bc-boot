#include "bsp_usart.h"
#include "bsp_dma.h"
#include "stdio.h"

uint8_t USART1_Txbuff[Usart1_TxBuff_Size];
//uint8_t USART1_Rxbuff[Usart1_RxBuff_Size];
volatile uint8_t usart0_rx_buffer[Usart1_RxBuff_Size];

uint8_t USART2_Txbuff[Usart2_TxBuff_Size];
//uint8_t USART2_Rxbuff[Usart2_RxBuff_Size];
volatile uint8_t usart2_rx_buffer[Usart2_RxBuff_Size];

uint16_t  Usart1_Rx_Lenth = 0; //接收长度
uint16_t  Usart1_Rx_Evt = 0;

uint16_t  Usart2_Rx_Lenth = 0; //接收长度
uint16_t  Usart2_Rx_Evt = 0;


uint8_t get_usart1_rx_state(void)
{
    if(USART_GetStatus(USART1_PORT,USART_FLAG_RX_TIMEOUT) == SET) //判断是否接收完一帧数据
    {
        TMR0_Stop(CM_TMR0_1, TMR0_CH_A);
        Usart1_Rx_Lenth =Usart1_RxBuff_Size - DMA_GetTransCount(USART1_RX_DMA_UNIT,USART1_RX_DMA_CH);
        DMA_ChCmd(USART1_RX_DMA_UNIT, USART1_RX_DMA_CH, DISABLE); //关闭DMA通道
        DMA_SetDestAddr(USART1_RX_DMA_UNIT,USART1_RX_DMA_CH,(uint32_t)&usart0_rx_buffer[0]);
        DMA_SetTransCount(USART1_RX_DMA_UNIT, USART1_RX_DMA_CH, Usart1_RxBuff_Size); //设置DMA传输长度
        DMA_ChCmd(USART1_RX_DMA_UNIT, USART1_RX_DMA_CH, ENABLE); //关闭DMA通道
        USART_ClearStatus(USART1_PORT, USART_FLAG_RX_TIMEOUT);
        return SET;
    }
    return RESET;
}



uint8_t get_usart2_rx_state(void)
{
    if(USART_GetStatus(USART2_PORT,USART_FLAG_RX_TIMEOUT) == SET) //判断是否接收完一帧数据
    {
        TMR0_Stop(CM_TMR0_1, TMR0_CH_B);
        Usart2_Rx_Lenth =Usart2_RxBuff_Size - DMA_GetTransCount(USART2_RX_DMA_UNIT,USART2_RX_DMA_CH);
        DMA_ChCmd(USART2_RX_DMA_UNIT, USART2_RX_DMA_CH, DISABLE); //关闭DMA通道
        DMA_SetDestAddr(USART2_RX_DMA_UNIT,USART2_RX_DMA_CH,(uint32_t)&usart2_rx_buffer[0]);
        DMA_SetTransCount(USART2_RX_DMA_UNIT, USART2_RX_DMA_CH, Usart1_RxBuff_Size); //设置DMA传输长度
        DMA_ChCmd(USART2_RX_DMA_UNIT, USART2_RX_DMA_CH, ENABLE); //关闭DMA通道
        USART_ClearStatus(USART2_PORT, USART_FLAG_RX_TIMEOUT);
        return SET;
    }
    return RESET;
}



//根据波特率和超时位数，计算TIMER0的比较值
//默认主频200M，PCLK1100M,timer0对应的计数源为100M的8分频（100M/8）,对应的时间就是 8 / 100M，波特率每1个bit，时间为 1/baudrate
//因此，每一个bit对应的TIMER0计数值为 1/baudrate / (8 / 100M) = 100M / (8 * baudrate)
//假设波特率是115200，则对应的计数值为 100M / (8 * 115200) = 108.51
uint16_t Get_Timeout_Value(uint32_t Baudrate,uint8_t TimeoutBits)
{
    uint16_t CompareVal = 0;
    uint32_t u32Div = 100000000UL / (8 * Baudrate);
    CompareVal = (uint16_t)(u32Div * TimeoutBits);
    if(CompareVal > 0xFFF0)
    {
        CompareVal = 0xFFF0;
    }
    else if(CompareVal < 2)
    {
        CompareVal = 2; //最小值为2
    }
    return CompareVal;
}


//timer0_1A 初始化，用于USART1的接收超时
void Timer0_1_A_Init(void)
{
    uint16_t u16CompareValue;
    stc_tmr0_init_t stcTmr0Init;

    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR0_1, ENABLE);

    /* Initialize TMR0 base function. */
    stcTmr0Init.u32ClockSrc = TMR0_CLK_SRC_INTERN_CLK; //使用PCLK1作为时钟
    stcTmr0Init.u32ClockDiv = TMR0_CLK_DIV8;  
    stcTmr0Init.u32Func     = TMR0_FUNC_CMP;  
   
    u16CompareValue = Get_Timeout_Value(USART1_BAUD,USART1_TIMOUT_BITS); //空闲20个bit算超时
    
    stcTmr0Init.u16CompareValue = u16CompareValue;
    (void)TMR0_Init(CM_TMR0_1, TMR0_CH_A, &stcTmr0Init);

    TMR0_HWStartCondCmd(CM_TMR0_1, TMR0_CH_A, ENABLE);
    TMR0_HWClearCondCmd(CM_TMR0_1, TMR0_CH_A, ENABLE);

}

// timer0_1A 初始化，用于USART1的接收超时
void Timer0_1_B_Init(void)
{
    uint16_t u16CompareValue;
    stc_tmr0_init_t stcTmr0Init;

    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR0_1, ENABLE);

    /* Initialize TMR0 base function. */
    stcTmr0Init.u32ClockSrc = TMR0_CLK_SRC_INTERN_CLK; //使用PCLK1作为时钟
    stcTmr0Init.u32ClockDiv = TMR0_CLK_DIV8;  
    stcTmr0Init.u32Func     = TMR0_FUNC_CMP;  
   
    u16CompareValue = Get_Timeout_Value(USART2_BAUD,USART2_TIMOUT_BITS); //空闲20个bit算超时
    
    stcTmr0Init.u16CompareValue = u16CompareValue;
    (void)TMR0_Init(CM_TMR0_1, TMR0_CH_B, &stcTmr0Init);

    TMR0_HWStartCondCmd(CM_TMR0_1, TMR0_CH_B, ENABLE);
    TMR0_HWClearCondCmd(CM_TMR0_1, TMR0_CH_B, ENABLE);

}

//使用到的端口初始化
void BSP_Port_Init(void)
{
    GPIO_SetFunc(USART1_TX_PORT,USART1_TX_PIN , USART1_TX_FUNC);
    GPIO_SetFunc(USART1_RX_PORT,USART1_RX_PIN , USART1_RX_FUNC);
    GPIO_SetFunc(USART2_TX_PORT,USART2_TX_PIN , USART2_TX_FUNC);
    GPIO_SetFunc(USART2_RX_PORT,USART2_RX_PIN , USART2_RX_FUNC);
}

//串口初始化
void BSP_Usart1_Init(void)
{
    stc_usart_uart_init_t stcUartInit;
    FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_USART1, ENABLE);
//    stc_irq_signin_config_t stcIrqSigninConfig;
    
    (void)USART_UART_StructInit(&stcUartInit);

    stcUartInit.u32ClockDiv = USART_CLK_DIV64;
    stcUartInit.u32CKOutput = USART_CK_OUTPUT_ENABLE;
    stcUartInit.u32Baudrate = USART1_BAUD;
    stcUartInit.u32Parity = USART1_PARITY;
    stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    if (LL_OK != USART_UART_Init(USART1_PORT, &stcUartInit, NULL)) {
        for (;;) {
        }
    }

     /* Register RX timeout IRQ handler. */
//    stcIrqSigninConfig.enIRQn = USART1_RX_TIMEOUT_IRQn;
//    stcIrqSigninConfig.enIntSrc = USART1_INT_SRC;
//    stcIrqSigninConfig.pfnCallback = &USART1_RxTimeout_IrqCallback;
//    (void)INTC_IrqSignIn(&stcIrqSigninConfig);
//    NVIC_ClearPendingIRQ(stcIrqSigninConfig.enIRQn);
//    NVIC_SetPriority(stcIrqSigninConfig.enIRQn, USART1_INT_PRIORITY);
//    NVIC_EnableIRQ(stcIrqSigninConfig.enIRQn);


    USART_FuncCmd(USART1_PORT, USART_TX|USART_RX|USART_RX_TIMEOUT|USART_INT_RX_TIMEOUT, ENABLE); //使能发送
}


void BSP_Usart2_Init(void)
{
    stc_usart_uart_init_t stcUartInit;
    FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_USART2, ENABLE);
//    stc_irq_signin_config_t stcIrqSigninConfig;
    
    (void)USART_UART_StructInit(&stcUartInit);

    stcUartInit.u32ClockDiv = USART_CLK_DIV4;
    stcUartInit.u32CKOutput = USART_CK_OUTPUT_ENABLE;
    stcUartInit.u32Baudrate = USART2_BAUD;
    stcUartInit.u32Parity = USART2_PARITY;
    stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    if (LL_OK != USART_UART_Init(USART2_PORT, &stcUartInit, NULL)) {
        for (;;) {
        }
    }

     /* Register RX timeout IRQ handler. */
//    stcIrqSigninConfig.enIRQn = USART2_RX_TIMEOUT_IRQn;
//    stcIrqSigninConfig.enIntSrc = USART2_INT_SRC;
//    stcIrqSigninConfig.pfnCallback = &USART2_RxTimeout_IrqCallback;
//    (void)INTC_IrqSignIn(&stcIrqSigninConfig);
//    NVIC_ClearPendingIRQ(stcIrqSigninConfig.enIRQn);
//    NVIC_SetPriority(stcIrqSigninConfig.enIRQn, USART2_INT_PRIORITY);
//    NVIC_EnableIRQ(stcIrqSigninConfig.enIRQn);


    USART_FuncCmd(USART2_PORT, USART_TX|USART_RX|USART_RX_TIMEOUT|USART_INT_RX_TIMEOUT, ENABLE); //使能发送
}



//串口1发送数据
void Uart1_SendData(uint8_t *pu8Data, uint16_t u16Len)
{
    uint32_t Timeout = 0xfffffff0;
    while(DMA_Get_Chen(USART1_TX_DMA_CH) == 1)
    {
        Timeout--;
        if(Timeout == 0)
        {
            //加入超时处理
            DMA_ChCmd(USART1_TX_DMA_UNIT, USART1_TX_DMA_CH,DISABLE); //超时强制关闭DMA
        }
    }
    DMA_ChCmd(USART1_TX_DMA_UNIT, USART1_TX_DMA_CH,DISABLE);
    DMA_SetTransCount(USART1_TX_DMA_UNIT, USART1_TX_DMA_CH, u16Len);           // 设置DMA传输长度
    DMA_SetSrcAddr(USART1_TX_DMA_UNIT, USART1_TX_DMA_CH, (uint32_t)(pu8Data)); // 设置DMA源地址（跳过第一个字节）
//    DMA_SetDestAddr(USART_TX_DMA_UNIT, USART_TX_DMA_CH, (uint32_t)(&USART1_PORT->TDR));  // 设置DMA目的地址
    DMA_ChCmd(USART1_TX_DMA_UNIT, USART1_TX_DMA_CH,ENABLE); //使能DMA
    DMA_MxChSWTrigger(USART1_TX_DMA_UNIT, 1<<USART1_TX_DMA_CH); //软件启动DMA传输
}

//串口2发送数据
void Uart2_SendData(uint8_t *pu8Data, uint16_t u16Len)
{
    uint32_t Timeout = 0xfffffff0;
    while(DMA_Get_Chen(USART2_TX_DMA_CH) == 1)
    {
        Timeout--;
        if(Timeout == 0)
        {
            //加入超时处理
            DMA_ChCmd(USART2_TX_DMA_UNIT, USART2_TX_DMA_CH,DISABLE); //超时强制关闭DMA
        }
    }
    DMA_ChCmd(USART2_TX_DMA_UNIT, USART2_TX_DMA_CH,DISABLE);
    DMA_SetTransCount(USART2_TX_DMA_UNIT, USART2_TX_DMA_CH, u16Len);           // 设置DMA传输长度
    DMA_SetSrcAddr(USART2_TX_DMA_UNIT, USART2_TX_DMA_CH, (uint32_t)(pu8Data)); // 设置DMA源地址（跳过第一个字节）
//    DMA_SetDestAddr(USART_TX_DMA_UNIT, USART_TX_DMA_CH, (uint32_t)(&USART1_PORT->TDR));  // 设置DMA目的地址
    DMA_ChCmd(USART2_TX_DMA_UNIT, USART2_TX_DMA_CH,ENABLE); //使能DMA
    DMA_MxChSWTrigger(USART2_TX_DMA_UNIT, 1<<USART2_TX_DMA_CH); //软件启动DMA传输
}

void bsp_usart_init(void)
{
    BSP_Port_Init();
    BSP_Usart1_Init();
    BSP_Usart2_Init();
    Timer0_1_A_Init();
    Timer0_1_B_Init();
}

void bsp_usart_putc(usart_output_port_t port, uint8_t ch)
{
    switch (port)
    {
    case OUTPUT_USART0:
        while(USART_GetStatus(USART1_PORT, USART_FLAG_TX_EMPTY) == 0);
        USART1_PORT->TDR = ch;
        while(USART_GetStatus(USART1_PORT, USART_FLAG_TX_EMPTY) == 0);
        break;
    case OUTPUT_USART2:
        while(USART_GetStatus(USART2_PORT, USART_FLAG_TX_EMPTY) == 0);
        USART2_PORT->TDR = ch;
        while(USART_GetStatus(USART2_PORT, USART_FLAG_TX_EMPTY) == 0);
        break;
    default:
        break;
    }
}


usart_output_port_t g_output_port = OUTPUT_USART0;

int fputc(int ch, FILE *f)
{
    switch (g_output_port) 
    {
    case OUTPUT_USART0:
         while(USART_GetStatus(USART1_PORT, USART_FLAG_TX_EMPTY) == 0);
         USART1_PORT->TDR = (uint8_t)ch;
         while(USART_GetStatus(USART1_PORT, USART_FLAG_TX_EMPTY) == 0);
         break;
    case OUTPUT_USART2:
         while(USART_GetStatus(USART2_PORT, USART_FLAG_TX_EMPTY) == 0);
         USART2_PORT->TDR = (uint8_t)ch;
         while(USART_GetStatus(USART2_PORT, USART_FLAG_TX_EMPTY) == 0);
         break;
    default:
         break;
    }
    return 0;
}

