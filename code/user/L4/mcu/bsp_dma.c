#include "bsp_dma.h"
#include "gd32f30x_dma.h"
#include "stdio.h"
#include "bsp_usart.h"

void bsp_dma_init(void)
{
    dma_parameter_struct dma0_parameter;

    rcu_periph_clock_enable(RCU_DMA0);

    dma_deinit(DMA0, DMA_CH2);

    dma_circulation_enable(DMA0, DMA_CH2);

    dma0_parameter.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma0_parameter.number = USART2_RX_BUFFER_SIZE;
    dma0_parameter.priority = DMA_PRIORITY_LOW;

    dma0_parameter.periph_addr = (uint32_t)(&USART_DATA(USART2));
    dma0_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma0_parameter.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;

    dma0_parameter.memory_addr = (uint32_t)(&usart2_rx_buffer[0]);
    dma0_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma0_parameter.memory_width = DMA_MEMORY_WIDTH_8BIT;

    dma_init(DMA0, DMA_CH2, &dma0_parameter);

    dma_channel_enable(DMA0, DMA_CH2);

    dma_deinit(DMA0, DMA_CH4);

    dma_circulation_enable(DMA0, DMA_CH4);

    dma0_parameter.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma0_parameter.number = USART2_RX_BUFFER_SIZE;
    dma0_parameter.priority = DMA_PRIORITY_LOW;

    dma0_parameter.periph_addr = (uint32_t)(&USART_DATA(USART0));
    dma0_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma0_parameter.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;

    dma0_parameter.memory_addr = (uint32_t)(&usart0_rx_buffer[0]);
    dma0_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma0_parameter.memory_width = DMA_MEMORY_WIDTH_8BIT;

    dma_init(DMA0, DMA_CH4, &dma0_parameter);

    dma_channel_enable(DMA0, DMA_CH4);
}
