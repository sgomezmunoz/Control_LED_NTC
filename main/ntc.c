#include "ntc.h"
#include <math.h>
#include <stdio.h>

void ntc_init(adc_oneshot_unit_handle_t handle, adc_channel_t channel) {
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };
    adc_oneshot_config_channel(handle, channel, &config);
}

float leer_temperatura(adc_oneshot_unit_handle_t handle, adc_channel_t channel) {
    int adc = 0;
    adc_oneshot_read(handle, channel, &adc);

    if (adc <= 10 || adc >= 4080) {
        return 25.0; // Valor seguro por falla
    }

    float voltaje = ((float)adc / 4095.0) * 3.3;
    if (voltaje <= 0.05) return 0;
    
    float resistencia = (3.3 - voltaje) * 10000.0 / voltaje;
    float tempK = 1.0 / ((1.0 / 298.15) + (1.0 / 3950.0) * log(resistencia / 10000.0));
    
    return tempK - 273.15;
}