#ifndef NTC_H
#define NTC_H

#include "esp_adc/adc_oneshot.h"

// Prototipos
void ntc_init(adc_oneshot_unit_handle_t handle, adc_channel_t channel);
float leer_temperatura(adc_oneshot_unit_handle_t handle, adc_channel_t channel);

#endif