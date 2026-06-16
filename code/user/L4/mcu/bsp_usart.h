#ifndef __BSP_USART_H
#define __BSP_USART_H

#include "gd32f30x.h"

#define BAUD_RATE 9600 // 115200 baud rate
#define BAUD_INTEGER (3750000 / BAUD_RATE)
#define BAUD_FRACTION ((uint32_t)(3750000.0f / (float)BAUD_RATE * 16) % 0x0000000F)

#define USART0_BAUD_RATE 9600 // 115200 baud rate
#define USART0_BAUD_INTEGER (7500000 / BAUD_RATE)
#define USART0_BAUD_FRACTION ((uint32_t)(7500000.0f / (float)BAUD_RATE * 16) % 0x0000000F)

#define USART2_RX_BUFFER_SIZE 256
#define USART2_TX_BUFFER_SIZE 256

#define USART0_RX_BUFFER_SIZE 256
#define USART0_TX_BUFFER_SIZE 256

extern volatile uint8_t usart2_rx_buffer[USART2_RX_BUFFER_SIZE];
extern volatile uint8_t usart2_tx_buffer[USART2_TX_BUFFER_SIZE];

extern volatile uint8_t usart0_rx_buffer[USART2_RX_BUFFER_SIZE];
extern volatile uint8_t usart0_tx_buffer[USART2_TX_BUFFER_SIZE];

void bsp_usart_init(void);

typedef enum
{
    OUTPUT_USART0,
    OUTPUT_USART2
} usart_output_port_t;

extern usart_output_port_t g_output_port;

#endif
