#include "rgb.h"

#define PWM_MAX 8191   // 13 bits

void rgb_init(int pines[6])                 // Inicializa los canales de PWM para controlar los LEDs RGB, configurando un temporizador de PWM con una resolución de 13 bits y una frecuencia de 5 kHz, y luego configura cada canal de PWM para los pines correspondientes a los LEDs RGB, estableciendo inicialmente el duty cycle en 0 (LEDs apagados).
{
    ledc_timer_config_t timer = {           // Configura un temporizador de PWM con una resolución de 13 bits y una frecuencia de 5 kHz, utilizando el reloj automático para seleccionar la fuente de reloj adecuada. 
        .speed_mode      = LEDC_LOW_SPEED_MODE,     
        .timer_num       = LEDC_TIMER_0,            
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz         = 5000,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);          //aplica la configuracion del timer

    for (int i = 0; i < 6; i++) {           //itera 6 veces 
        ledc_channel_config_t ch = {        
            .gpio_num   = pines[i],
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel    = (ledc_channel_t)i,
            .timer_sel  = LEDC_TIMER_0,
            .duty       = 0
        };
        ledc_channel_config(&ch);
    }
}

// Ánodo común → lógica invertida
void rgb_led1_set(float r, float g, float b)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 0, (uint32_t)((1.0f - r) * PWM_MAX));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 1, (uint32_t)((1.0f - g) * PWM_MAX));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 1);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 2, (uint32_t)((1.0f - b) * PWM_MAX));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 2);
}

// Cátodo común → lógica directa
void rgb_led2_set(float r, float g, float b)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 3, (uint32_t)(r * PWM_MAX));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 3);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 4, (uint32_t)(g * PWM_MAX));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 4);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 5, (uint32_t)(b * PWM_MAX));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 5);
}
