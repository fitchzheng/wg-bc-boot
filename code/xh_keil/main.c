#include "hc32_ll.h"
#include "section.h"
#include "ev_hc32f334_lqfp64.h"

#define LL_PERIPH_SEL (LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                       LL_PERIPH_EFM)

int main(void)
{
    LL_PERIPH_WE(LL_PERIPH_SEL);
    BSP_CLK_Init();
    SysTick_Init(1000);
    section_init();
    while (1)
    {
        run_task();
    }
}
