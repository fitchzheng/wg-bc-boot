#ifndef __BSP_USART_H__
#define __BSP_USART_H__

#include "hc32_ll.h"
#include "hc32_ll_usart.h"
#include "hc32_ll_dma.h"

//usart1 串口引脚配置
#define USART1_TX_PORT GPIO_PORT_B
#define USART1_TX_PIN  GPIO_PIN_06
#define USART1_TX_FUNC GPIO_FUNC_32

#define USART1_RX_PORT GPIO_PORT_B
#define USART1_RX_PIN  GPIO_PIN_07
#define USART1_RX_FUNC GPIO_FUNC_33

#define USART1_PORT    CM_USART1

//usart2 串口引脚配置
#define USART2_TX_PORT GPIO_PORT_C
#define USART2_TX_PIN  GPIO_PIN_10
#define USART2_TX_FUNC GPIO_FUNC_36

#define USART2_RX_PORT GPIO_PORT_C
#define USART2_RX_PIN  GPIO_PIN_11
#define USART2_RX_FUNC GPIO_FUNC_37

#define USART2_PORT    CM_USART2

//串口波特率
#define USART1_BAUD 9600
#define USART2_BAUD 9600

//串口校验位，以下三选一
// USART_PARITY_NONE  
// USART_PARITY_EVEN 
// USART_PARITY_ODD  
#define USART1_PARITY  USART_PARITY_NONE
#define USART2_PARITY  USART_PARITY_NONE


//串口接受超时时间，单位bits
#define USART1_TIMOUT_BITS 20 //总线空闲20个bit算超时，即一帧结束
#define USART2_TIMOUT_BITS 20 //空闲20个bit算超时



#define Usart1_TxBuff_Size  256
#define Usart1_RxBuff_Size  256
extern uint8_t USART1_Txbuff[Usart1_TxBuff_Size];
extern uint8_t USART1_Rxbuff[Usart1_RxBuff_Size];
extern volatile uint8_t usart0_rx_buffer[Usart1_RxBuff_Size];

#define Usart2_TxBuff_Size  256
#define Usart2_RxBuff_Size  256
extern uint8_t USART2_Txbuff[Usart2_TxBuff_Size];
extern uint8_t USART2_Rxbuff[Usart2_RxBuff_Size];
extern volatile uint8_t usart2_rx_buffer[Usart2_RxBuff_Size];

#define USART2_RX_BUFFER_SIZE Usart1_RxBuff_Size
#define USART2_TX_BUFFER_SIZE Usart1_TxBuff_Size

#define USART0_RX_BUFFER_SIZE Usart2_RxBuff_Size
#define USART0_TX_BUFFER_SIZE Usart2_TxBuff_Size

extern uint16_t  Usart1_Rx_Lenth ; //接收长度
extern uint16_t  Usart1_Rx_Evt ;

extern uint16_t  Usart2_Rx_Lenth ; //接收长度
extern uint16_t  Usart2_Rx_Evt ;

typedef enum {
    OUTPUT_USART0,
    OUTPUT_USART2
} usart_output_port_t;

extern usart_output_port_t g_output_port;

void bsp_usart_init(void);
void bsp_usart_putc(usart_output_port_t port, uint8_t ch);
void Uart1_SendData(uint8_t *pu8Data, uint16_t u16Len);
void Uart2_SendData(uint8_t *pu8Data, uint16_t u16Len);

uint8_t get_usart1_rx_state(void);
uint8_t get_usart2_rx_state(void);

void USART1_RxTimeout_config(void);
void USART2_RxTimeout_config(void);

#endif
