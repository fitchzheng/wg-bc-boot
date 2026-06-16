#include "section.h"
#include "bsp_init.h"
#include "bsp_dma.h"
#include "bsp_gpio.h"
#include "bsp_usart.h"

void bsp_init(void)
{
    bsp_gpio_init();
    bsp_usart_init();
    bsp_dma_init();
}

REG_INIT_TP(bsp_init);
