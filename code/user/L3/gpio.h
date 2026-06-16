#ifndef __GPIO_H
#define __GPIO_H

#include "stdint.h"

void gpio_set_auxoff(uint8_t val);

void gpio_set_red(uint8_t val);

void gpio_set_green(uint8_t val);

void gpio_set_bled1(uint8_t val);

void gpio_set_bled2(uint8_t val);

void gpio_set_leds(uint8_t val);

uint8_t gpio_get_pg_en(void);

void gpio_set_db2(uint8_t val);

void gpio_set_db1(uint8_t val);

void gpio_set_da2(uint8_t val);

void gpio_set_da1(uint8_t val);

void gpio_set_re(uint8_t val);

#endif
