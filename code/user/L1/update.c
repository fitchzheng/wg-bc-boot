#include "ymodem.h"
#include "bsp_usart.h"

// 假设 usart2_rx_buffer 相关定义如下（如有不同请自行调整）
extern uint8_t usart2_rx_buffer[];
extern volatile uint16_t usart2_rx_head;
extern volatile uint16_t usart2_rx_tail;
extern volatile uint16_t usart2_rx_count;
#define USART2_RX_BUFFER_SIZE 512

int ymodem_port_send(const uint8_t *buf, size_t len)
{
}

int ymodem_port_recv(uint8_t *buf, size_t len, uint32_t timeout_ms)
{
}
