#include "bsp_usart.h"
#include "stdio.h"
#include "stdint.h"
#include "gd32f30x.h"

volatile uint8_t usart2_rx_buffer[USART2_RX_BUFFER_SIZE];
volatile uint8_t usart2_tx_buffer[USART2_TX_BUFFER_SIZE];

volatile uint8_t usart0_rx_buffer[USART0_RX_BUFFER_SIZE];
volatile uint8_t usart0_tx_buffer[USART0_TX_BUFFER_SIZE];

void bsp_usart_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_USART2);
    rcu_periph_clock_enable(RCU_AF);
    gpio_pin_remap_config(GPIO_USART2_PARTIAL_REMAP, ENABLE);
    gpio_init(GPIOC, GPIO_MODE_AF_PP, GPIO_OSPEED_MAX, GPIO_PIN_10);
    gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_MAX, GPIO_PIN_11);
    USART_BAUD(USART2) = (BAUD_INTEGER << 4) + BAUD_FRACTION;
    USART_CTL0(USART2) = 0;
    USART_CTL0(USART2) |= 1 << 13;
    USART_CTL0(USART2) |= 1 << 2;
    USART_CTL0(USART2) |= 1 << 3;
    USART_CTL2(USART2) = 0x00000040;
    USART_STAT0(USART2) = 0;

    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_USART0);
    gpio_pin_remap_config(GPIO_USART0_REMAP, ENABLE);
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_MAX, GPIO_PIN_6);
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_MAX, GPIO_PIN_7);
    USART_BAUD(USART0) = (USART0_BAUD_INTEGER << 4) + USART0_BAUD_FRACTION;
    USART_CTL0(USART0) = 0;
    USART_CTL0(USART0) |= 1 << 13;
    USART_CTL0(USART0) |= 1 << 2;
    USART_CTL0(USART0) |= 1 << 3;
    USART_CTL2(USART0) = 0x00000040;
    USART_STAT0(USART0) = 0;
}

usart_output_port_t g_output_port = OUTPUT_USART2;

int fputc(int ch, FILE *f)
{
    switch (g_output_port)
    {
    case OUTPUT_USART0:
        USART_DATA(USART0) = (uint8_t)ch;
        while (RESET == usart_flag_get(USART0, USART_FLAG_TC))
            ;                                    // 等待发送完成
        usart_flag_clear(USART0, USART_FLAG_TC); // 手动清除TC标志
        break;
    case OUTPUT_USART2:
        USART_DATA(USART2) = (uint8_t)ch;
        while (RESET == usart_flag_get(USART2, USART_FLAG_TBE))
            ;
        break;
    default:
        break;
    }
    return ch;
}
