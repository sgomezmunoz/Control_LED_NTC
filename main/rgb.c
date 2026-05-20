#include "rgb.h"

void pwm_init(int pins[6]) {
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    for(int i = 0; i < 6; i++) {
        ledc_channel_config_t ch = {
            .gpio_num = pins[i],
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = i,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0
        };
        ledc_channel_config(&ch);
    }
}

// Lógica de ÁNODO COMÚN (Invertida)
void set_led1_temperatura(float r, float g, float b) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 0, (1.0 - r) * 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 1, (1.0 - g) * 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 1);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 2, (1.0 - b) * 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 2);
}

// Lógica de CÁTODO COMÚN (Directa)
void set_led2_mezclador(float r, float g, float b) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 3, r * 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 3);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 4, g * 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 4);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 5, b * 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 5);
}