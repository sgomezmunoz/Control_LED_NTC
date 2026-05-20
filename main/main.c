#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"

// ========================================
// PINES ORIGINALES MANTENIDOS
// ========================================
#define BTN_GUARDAR GPIO_NUM_8

#define R1 GPIO_NUM_2
#define G1 GPIO_NUM_3
#define B1 GPIO_NUM_4

#define R2 GPIO_NUM_5
#define G2 GPIO_NUM_6
#define B2 GPIO_NUM_7

#define ADC_NTC ADC_CHANNEL_0  // GPIO 0
#define ADC_POT ADC_CHANNEL_1  // GPIO 1

// Handle para el ADC
adc_oneshot_unit_handle_t adc_handle;

// Variables globales para el Mezclador (LED 2)
float r_save = 0, g_save = 0, b_save = 0;
int paso = 0; 

// ========================================
// CONFIGURACIÓN DE PERIFÉRICOS
// ========================================

void pwm_init() {
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    // Configurar los 6 pines (LED 1 y LED 2)
    int pines[6] = {R1, G1, B1, R2, G2, B2};
    for(int i = 0; i < 6; i++) {
        ledc_channel_config_t ch = {
            .gpio_num = pines[i],
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = i, // Canales 0,1,2 para LED1 | Canales 3,4,5 para LED2
            .timer_sel = LEDC_TIMER_0,
            .duty = 0
        };
        ledc_channel_config(&ch);
    }
}

// Control de color para el LED 1 (Canales 0, 1, 2)
// MODIFICADO: Inversión de lógica para LED de Ánodo Común
void set_led1_temperatura(float r, float g, float b) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 0, (1.0 - r) * 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 1, (1.0 - g) * 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 1);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 2, (1.0 - b) * 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 2);
}

// Control de color para el LED 2 (Canales 3, 4, 5) - SIN CAMBIOS
void set_led2_mezclador(float r, float g, float b) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 3, r * 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 3);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 4, g * 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 4);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, 5, b * 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, 5);
}

// Lectura del sensor de temperatura NTC
float leer_temperatura() {
    int adc = 0;
    // Si se congela aquí, el problema es que el ADC no responde
    adc_oneshot_read(adc_handle, ADC_NTC, &adc);

    // IMPRESIÓN DE DIAGNÓSTICO: Ver el valor crudo del sensor
    printf("Lectura ADC Cruda: %d\n", adc);

    // ESCUDO PROTECTOR: Evita que el ESP32 se congele por errores matemáticos
    if (adc <= 10 || adc >= 4080) {
        printf("[ALERTA] Sensor NTC mal conectado o en corto. Revisa cables.\n");
        return 25.0; // Retorna temperatura ficticia para que no se trabe el LED
    }

    float voltaje = ((float)adc / 4095.0) * 3.3;
    
    // Calcular resistencia (Evitar división por cero)
    if (voltaje <= 0.05) return 0;
    float resistencia = (3.3 - voltaje) * 10000.0 / voltaje;

    // Ecuación de Steinhart-Hart
    float tempK = 1.0 / ((1.0 / 298.15) + (1.0 / 3950.0) * log(resistencia / 10000.0));
    float tempC = tempK - 273.15;

    printf("Temperatura Calculada: %.2f C\n", tempC);
    return tempC; 
}

// ========================================
// APP MAIN
// ========================================

void app_main() {
    pwm_init();
    
    // Inicializar Unidad ADC1
    adc_oneshot_unit_init_cfg_t init = {.unit_id = ADC_UNIT_1};
    adc_oneshot_new_unit(&init, &adc_handle);
    
    // Configurar canales ADC (NTC en CH0, POT en CH1)
    adc_oneshot_chan_cfg_t config = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT};
    adc_oneshot_config_channel(adc_handle, ADC_NTC, &config);
    adc_oneshot_config_channel(adc_handle, ADC_POT, &config);

    // Configurar Botón de Guardar
    gpio_config_t io = {.mode = GPIO_MODE_INPUT, .pin_bit_mask = (1ULL << BTN_GUARDAR), .pull_up_en = 1};
    gpio_config(&io);

    int last_btn = 1;

    while(1) {
        // ====================================
        // GESTIÓN DEL LED 1: TEMPERATURA (LÓGICA INVERTIDA)
        // ====================================
        float temp = leer_temperatura();
        
        if (temp < 25.0) {
            set_led1_temperatura(0, 0, 1);      // Azul (< 25°C)
        } else if (temp <= 35.0) {
            set_led1_temperatura(0, 1, 0);      // Verde (25°C a 35°C)
        } else {
            set_led1_temperatura(1, 0, 0);      // Rojo (> 35°C)
        }

        // ====================================
        // GESTIÓN DEL LED 2: MEZCLADOR MANUAL (SIN CAMBIOS)
        // ====================================
        int val_pot;
        adc_oneshot_read(adc_handle, ADC_POT, &val_pot);
        float intensidad = (float)val_pot / 4095.0;

        int btn = gpio_get_level(BTN_GUARDAR);
        if(btn == 0 && last_btn == 1) {
            vTaskDelay(pdMS_TO_TICKS(200)); // Antirebote
            
            if(paso == 0) r_save = intensidad;
            else if(paso == 1) g_save = intensidad;
            else if(paso == 2) b_save = intensidad;

            paso = (paso + 1) % 4;
        }
        last_btn = btn;

        // Actualización en tiempo real del mezclador
        switch(paso) {
            case 0: set_led2_mezclador(intensidad, 0, 0); break; // Ajustando Rojo
            case 1: set_led2_mezclador(0, intensidad, 0); break; // Ajustando Verde
            case 2: set_led2_mezclador(0, 0, intensidad); break; // Ajustando Azul
            case 3: set_led2_mezclador(r_save, g_save, b_save); break; // Mezcla final
        }

        // Una pequeña pausa para que el sistema respire y el ADC sea estable
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}