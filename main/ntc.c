#include "ntc.h"
#include <math.h>

#define NTC_R_FIJA   10000.0f
#define NTC_R_NOM    20000.0f
#define NTC_BETA     4950.0f
#define NTC_T_NOM    298.15f   // 25 °C en Kelvin
#define ADC_MAX      4095.0f
#define V_REF        3.3f

void ntc_init(adc_oneshot_unit_handle_t handle, adc_channel_t ch)
{
    adc_oneshot_chan_cfg_t cfg = {              // 12 bits de resolución y atenuación de 12 dB para el canal ADC asociado al sensor NTC, lo que permite obtener lecturas precisas de la temperatura medida por el sensor.
        .atten    = ADC_ATTEN_DB_12,            
        .bitwidth = ADC_BITWIDTH_DEFAULT    
    };
    adc_oneshot_config_channel(handle, ch, &cfg);
}

float ntc_leer_celsius(adc_oneshot_unit_handle_t handle, adc_channel_t ch)
{
    int raw = 0;
    adc_oneshot_read(handle, ch, &raw);


    if (raw <= 10 || raw >= 4080) return 25.0f;

    float v = (raw / ADC_MAX) * V_REF;

    // Protección contra división por cero
    if (v <= 0.05f) return 125.0f;   // NTC en cortocircuito → muy caliente
    if (v >= 3.25f) return -40.0f;   // NTC abierta → muy frío

    // NTC entre VCC y ADC, R_FIJA entre ADC y GND
    float r  = NTC_R_FIJA * (V_REF - v) / v;
    float tK = 1.0f / ((1.0f / NTC_T_NOM) + (1.0f / NTC_BETA) * logf(r / NTC_R_NOM));
    return tK - 273.15f + 29.0f;
}
