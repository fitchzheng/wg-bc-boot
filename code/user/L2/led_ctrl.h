#ifndef __LED_CTRL_H
#define __LED_CTRL_H

#include "stdint.h"

typedef enum
{
    LED_CTRL_LED_OFF,
    LED_CTRL_GREEN,
    LED_CTRL_RED,
    LED_CTRL_YELLOW,
    LED_CTRL_BLED_OFF,
    LED_CTRL_BLED_RED,
    LED_CTRL_BLED_GREEN,
} LED_CTRL_MODE_E;

void led_ctrl_set_led(LED_CTRL_MODE_E mode, uint8_t val);

#endif
