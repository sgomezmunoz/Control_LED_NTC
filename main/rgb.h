#ifndef RGB_H
#define RGB_H

#include "driver/ledc.h"

// pines[0..2] = R1,G1,B1  (LED temperatura, ánodo común)
// pines[3..5] = R2,G2,B2  (LED potenciómetro, cátodo común)
void rgb_init(int pines[6]);
void rgb_led1_set(float r, float g, float b);   // valores 0.0–1.0
void rgb_led2_set(float r, float g, float b);   // valores 0.0–1.0

#endif
