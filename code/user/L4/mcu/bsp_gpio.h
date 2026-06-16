#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H

#include "gd32f30x.h"

/*!
    \brief      GPIO parameter initialization
    \param[in]  gpio_periph: GPIOx(x = A,B,C,D,E,F,G)
    \param[in]  mode: gpio pin mode
      \arg        GPIO_MODE_AIN: analog input mode
      \arg        GPIO_MODE_IN_FLOATING: floating input mode
      \arg        GPIO_MODE_IPD: pull-down input mode
      \arg        GPIO_MODE_IPU: pull-up input mode
      \arg        GPIO_MODE_OUT_OD: GPIO output with open-drain
      \arg        GPIO_MODE_OUT_PP: GPIO output with push-pull
      \arg        GPIO_MODE_AF_OD: AFIO output with open-drain
      \arg        GPIO_MODE_AF_PP: AFIO output with push-pull
    \param[in]  speed: gpio output max speed value
      \arg        GPIO_OSPEED_10MHZ: output max speed 10MHz
      \arg        GPIO_OSPEED_2MHZ: output max speed 2MHz
      \arg        GPIO_OSPEED_50MHZ: output max speed 50MHz
      \arg        GPIO_OSPEED_MAX: output max speed more than 50MHz
    \param[in]  pin: GPIO_PIN_x(x=0..15), GPIO_PIN_ALL
    \param[out] none
    \retval     none
*/

#define GPIO_REG_PARM(name, gpx, pin, mode) {name, gpx, GPIO_MODE_##mode, GPIO_OSPEED_50MHZ, GPIO_PIN_##pin, RCU_##gpx}

typedef enum
{
  AUXOFF,
  RED,
  Green,
  BLED1,
  BLED2,
  LEDS,
  PG_EN,
  DB2,
  DB1,
  DA2,
  DA1,
  RE,
  GPIO_TABLE_MAX
} bsp_gpio_table_e;

typedef struct
{
  bsp_gpio_table_e bsp_gpio_table;
  uint32_t gpio_periph;
  uint32_t mode;
  uint32_t speed;
  uint32_t pin;
  rcu_periph_enum rcu_periph;
} bsp_gpio_parm_t;

void bsp_gpio_init(void);
void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val);
void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val);

#endif
