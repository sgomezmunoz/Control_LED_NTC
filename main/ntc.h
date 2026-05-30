#ifndef NTC_H
#define NTC_H

#include "esp_adc/adc_oneshot.h"

void  ntc_init(adc_oneshot_unit_handle_t handle, adc_channel_t ch);
float ntc_leer_celsius(adc_oneshot_unit_handle_t handle, adc_channel_t ch);

#endif
