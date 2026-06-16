#include "gpio.h"
#include "bsp_gpio.h"

//void gpio_set_auxoff(uint8_t val)
//{
//    bsp_gpio_set_bit(AUXOFF, val);
//}

void gpio_set_red(uint8_t val)
{
    bsp_gpio_set_bit(RED, val);
}

void gpio_set_green(uint8_t val)
{
    bsp_gpio_set_bit(Green, val);
}

void gpio_set_bled1(uint8_t val)
{
    bsp_gpio_set_bit(BLED1, val);
}

void gpio_set_bled2(uint8_t val)
{
    bsp_gpio_set_bit(BLED2, val);
}

void gpio_set_leds(uint8_t val)
{
    bsp_gpio_set_bit(LEDS, val);
}

//uint8_t gpio_get_pg_en(void)
//{
//    uint8_t temp = 0;
//    bsp_gpio_get_bit(PG_EN, &temp);
//    return temp;
//}

//void gpio_set_db2(uint8_t val)
//{
//    bsp_gpio_set_bit(DB2, val);
//}

//void gpio_set_db1(uint8_t val)
//{
//    bsp_gpio_set_bit(DB1, val);
//}

//void gpio_set_da2(uint8_t val)
//{
//    bsp_gpio_set_bit(DA2, val);
//}

//void gpio_set_da1(uint8_t val)
//{
//    bsp_gpio_set_bit(DA1, val);
//}

void gpio_set_re(uint8_t val)
{
    bsp_gpio_set_bit(RE, val);
}
