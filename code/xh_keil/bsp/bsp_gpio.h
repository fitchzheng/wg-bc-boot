#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__ 

#include "hc32_ll.h"
#include "hc32_ll_gpio.h"

#define GPIO_MODE_OUT_PP 0
#define GPIO_MODE_IPD    1
//其他类型定义按实际需要增加



//以下定义中，speed与rcu_periph用不到，直接给0
#define GPIO_REG_PARM(_name, _gpx, _pin, _mode, _def_lv) { \
    .bsp_gpio_table = _name,                               \
    .gpio_periph = _gpx,                                   \
    .mode = GPIO_MODE_##_mode,                             \
    .speed = 0,                                            \
    .pin = GPIO_PIN_##_pin,                                \
    .rcu_periph = 0,                                       \
    .def_lv = _def_lv,                                     \
}

typedef enum
{
  BUCK_PWMH_A,
  BUCK_PWML_A,
  BOOST_PWML_A,
  BOOST_PWMH_A,
  RED,
  Green,
  BLED1,
  BLED2,
  BOOST_PWMH_B,
  BOOST_PWML_B,
  BUCK_PWMH_B,
  BUCK_PWML_B,
  LEDS,
  RE,
  TABLE_MAX,
} bsp_gpio_table_e;

typedef struct
{
  bsp_gpio_table_e bsp_gpio_table;
  uint32_t gpio_periph;
  uint32_t mode;
  uint32_t speed;
  uint32_t pin;
  uint32_t rcu_periph;
  uint8_t def_lv;
} bsp_gpio_parm_t;

void bsp_gpio_init(void);
void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val);
void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val);




#endif  

