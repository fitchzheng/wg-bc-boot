#include "bsp_gpio.h"

const bsp_gpio_parm_t bsp_gpio_parm_table[] = {
    GPIO_REG_PARM(AUXOFF, GPIOC, 13, OUT_PP),
    GPIO_REG_PARM(RED, GPIOB, 12, OUT_PP),
    GPIO_REG_PARM(Green, GPIOB, 15, OUT_PP),
    GPIO_REG_PARM(BLED1, GPIOC, 8, OUT_PP),
    GPIO_REG_PARM(BLED2, GPIOC, 9, OUT_PP),
    GPIO_REG_PARM(LEDS, GPIOA, 12, OUT_PP),
    GPIO_REG_PARM(PG_EN, GPIOA, 15, IPD),
    GPIO_REG_PARM(DB2, GPIOC, 12, OUT_PP),
    GPIO_REG_PARM(DB1, GPIOD, 2, OUT_PP),
    GPIO_REG_PARM(DA2, GPIOB, 3, OUT_PP),
    GPIO_REG_PARM(DA1, GPIOB, 4, OUT_PP),
    GPIO_REG_PARM(RE, GPIOB, 5, OUT_PP),
};

void bsp_gpio_init(void)
{
    uint8_t gpio_num = 0;
    gpio_num = sizeof(bsp_gpio_parm_table) / sizeof(bsp_gpio_parm_t);
    uint8_t i;

    rcu_periph_clock_enable(RCU_AF);
    gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP, ENABLE);

    for (i = 0; i < gpio_num; i++)
    {
        rcu_periph_clock_enable(bsp_gpio_parm_table[i].rcu_periph);
        gpio_init(bsp_gpio_parm_table[i].gpio_periph,
                  bsp_gpio_parm_table[i].mode,
                  bsp_gpio_parm_table[i].speed,
                  bsp_gpio_parm_table[i].pin);
    }
}

void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val)
{
    if (val)
    {
        gpio_bit_set(bsp_gpio_parm_table[num].gpio_periph, bsp_gpio_parm_table[num].pin);
    }
    else
    {
        gpio_bit_reset(bsp_gpio_parm_table[num].gpio_periph, bsp_gpio_parm_table[num].pin);
    }
}

void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val)
{
    *val = gpio_input_bit_get(bsp_gpio_parm_table[num].gpio_periph, bsp_gpio_parm_table[num].pin);
}
