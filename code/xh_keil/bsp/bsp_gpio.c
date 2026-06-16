#include "bsp_gpio.h"  

const bsp_gpio_parm_t bsp_gpio_parm_table[] = {
    GPIO_REG_PARM(BUCK_PWMH_A , GPIO_PORT_B, 12, OUT_PP, 0),
    GPIO_REG_PARM(BUCK_PWML_A , GPIO_PORT_B, 13, OUT_PP, 0),
    GPIO_REG_PARM(BOOST_PWML_A, GPIO_PORT_B, 14, OUT_PP, 0),
    GPIO_REG_PARM(BOOST_PWMH_A, GPIO_PORT_B, 15, OUT_PP, 0),
    GPIO_REG_PARM(LEDS        , GPIO_PORT_A, 12, OUT_PP, 1),
    GPIO_REG_PARM(Green       , GPIO_PORT_C, 07, OUT_PP, 1),
    GPIO_REG_PARM(BLED1       , GPIO_PORT_C, 08, OUT_PP, 1),
    GPIO_REG_PARM(BLED2       , GPIO_PORT_C, 09, OUT_PP, 1),
    GPIO_REG_PARM(BOOST_PWMH_B, GPIO_PORT_A, 08, OUT_PP, 0),
    GPIO_REG_PARM(BOOST_PWML_B, GPIO_PORT_A, 09, OUT_PP, 0),
    GPIO_REG_PARM(BUCK_PWMH_B , GPIO_PORT_A, 10, OUT_PP, 0),
    GPIO_REG_PARM(BUCK_PWML_B , GPIO_PORT_A, 11, OUT_PP, 0),
    GPIO_REG_PARM(RED         , GPIO_PORT_C, 06, OUT_PP, 1),
    GPIO_REG_PARM(RE          , GPIO_PORT_B, 05, OUT_PP, 0),
};

void bsp_gpio_init(void)
{
    uint8_t gpio_num = 0;
    gpio_num = sizeof(bsp_gpio_parm_table) / sizeof(bsp_gpio_parm_t);
    uint8_t i;

    stc_gpio_init_t stcGpioInit;
    (void)GPIO_StructInit(&stcGpioInit);

    //rcu_periph_clock_enable(RCU_AF);
    //gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP, ENABLE);
    GPIO_SetDebugPort(GPIO_PIN_DEBUG_JTAG,DISABLE);
    for (i = 0; i < gpio_num; i++)
    {
        //rcu_periph_clock_enable(bsp_gpio_parm_table[i].rcu_periph);

        if(bsp_gpio_parm_table[i].def_lv == 0)
        {
            stcGpioInit.u16PinState = PIN_STAT_RST;
        }
        else
        {
            stcGpioInit.u16PinState = PIN_STAT_SET;
        }

        switch (bsp_gpio_parm_table[i].mode)
        {
            case GPIO_MODE_OUT_PP:
                stcGpioInit.u16PinDir = PIN_DIR_OUT;
                stcGpioInit.u16PinDrv = PIN_HIGH_DRV;
                stcGpioInit.u16PinOutputType = PIN_OUT_TYPE_CMOS;
                break;
            case GPIO_MODE_IPD:
                stcGpioInit.u16PinDir =   PIN_DIR_IN;
                stcGpioInit.u16PullDown = PIN_PD_ON;
                break;
            default:
                break;
        }

        GPIO_Init(bsp_gpio_parm_table[i].gpio_periph, bsp_gpio_parm_table[i].pin, &stcGpioInit);
       
    }
}


void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val)
{
    if (val)
    {
        GPIO_SetPins(bsp_gpio_parm_table[num].gpio_periph, bsp_gpio_parm_table[num].pin);
    }
    else
    {
        GPIO_ResetPins(bsp_gpio_parm_table[num].gpio_periph, bsp_gpio_parm_table[num].pin);
    }
}

void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val)
{
    *val = GPIO_ReadInputPins(bsp_gpio_parm_table[num].gpio_periph, bsp_gpio_parm_table[num].pin);
}

