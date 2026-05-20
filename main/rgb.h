#ifndef RGB_H
#define RGB_H

#include "driver/ledc.h"

void pwm_init(int pins[6]);
void set_led1_temperatura(float r, float g, float b);
void set_led2_mezclador(float r, float g, float b);

#endif