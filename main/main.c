#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#include "ntc.h"
#include "rgb.h"
#include "uart_manager.h"

#define TAG "MAIN"

#define BTN_PIN   GPIO_NUM_8
#define R1        GPIO_NUM_2
#define G1        GPIO_NUM_3
#define B1        GPIO_NUM_4
#define R2        GPIO_NUM_5
#define G2        GPIO_NUM_6
#define B2        GPIO_NUM_7
#define CH_NTC    ADC_CHANNEL_0
#define CH_POT    ADC_CHANNEL_1

#define DEFAULT_INTERVAL_S  5
#define TICK_MS             30

static float convertir(float c, temp_unit_t u)
{
    if (u == UNIT_FAHRENHEIT) return c * 9.0f / 5.0f + 32.0f;
    if (u == UNIT_KELVIN)     return c + 273.15f;
    return c;
}

static const char *unidad_str(temp_unit_t u)
{
    if (u == UNIT_FAHRENHEIT) return "F";
    if (u == UNIT_KELVIN)     return "K";
    return "C";
}
                                //handle es un puntero a una estructura que representa la unidad ADC configurada previamente, y ch es el canal ADC específico que se va a leer para obtener la temperatura en grados Celsius. La función devuelve un valor de tipo float que representa la temperatura medida por el sensor NTC en grados Celsius, utilizando la configuración del ADC para realizar la lectura y conversión adecuada.
void app_main(void)
{
    // ADC                              &configuracion
    adc_oneshot_unit_handle_t adc;                          // adc es un manejador para la unidad ADC configurada previamente, que se utiliza para realizar lecturas de los canales ADC asociados al sensor NTC y al potenciómetro. Este manejador se inicializa con la configuración adecuada para el ADC y se utiliza posteriormente para leer los valores de temperatura y umbral de temperatura en el bucle principal del programa.
    adc_oneshot_unit_init_cfg_t adc_init = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&adc_init, &adc);      

    adc_oneshot_chan_cfg_t adc_cfg = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT };
    adc_oneshot_config_channel(adc, CH_NTC, &adc_cfg);
    adc_oneshot_config_channel(adc, CH_POT, &adc_cfg);
    ntc_init(adc, CH_NTC);

    // LEDs
    int pines[6] = { R1, G1, B1, R2, G2, B2 };    //define los pines de pwm
    rgb_init(pines);

    // confiugracion del  Botón
    gpio_config_t btn_cfg = {               //btn_cfg es una estructura que contiene la configuración para el pin del botón, incluyendo el modo de operación (entrada), la máscara de bits para seleccionar el pin específico y la habilitación de la resistencia pull-up interna. Esta configuración se utiliza para configurar el pin del botón como una entrada con resistencia pull-up, lo que permite detectar cuándo se presiona el botón al leer un nivel lógico bajo en el pin correspondiente.
        .mode         = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BTN_PIN),    //pin_bit_mask es una máscara de bits que se utiliza para seleccionar el pin específico del botón que se va a configurar. En este caso, se utiliza un desplazamiento de bits (1ULL << BTN_PIN) para crear una máscara que tiene un bit establecido en la posición correspondiente al número de pin del botón, lo que permite configurar solo ese pin como entrada.  
        .pull_up_en   = GPIO_PULLUP_ENABLE
    };
    gpio_config(&btn_cfg);

    // Config global — sin pot_threshold
    system_config_t config = {
        .red   = { .min = 35.0f,  .max = 100.0f, .intensity = 255 },
        .green = { .min = 25.0f,  .max = 35.0f,  .intensity = 255 },
        .blue  = { .min = -40.0f, .max = 25.0f,  .intensity = 255 },
        .unit             = UNIT_CELSIUS,
        .print_interval_s = DEFAULT_INTERVAL_S,            // Intervalo de impresión en segundos
        .mutex            = xSemaphoreCreateMutex()        // Crea un mutex para proteger el acceso a la configuración del sistema, asegurando que solo una tarea pueda modificar la configuración a la vez, evitando condiciones de carrera y garantizando la integridad de los datos cuando se accede o modifica la configuración desde diferentes partes del programa.
    };

    uart_init();
    xTaskCreate(uart_task, "uart", 4096, &config, 5, NULL);

    ESP_LOGI(TAG, "Sistema listo");

    int      last_btn  = 1;             //guarda el estado anterior del botón para detectar cambios de estado (presión y liberación), lo que permite implementar la funcionalidad de ciclar entre las unidades de temperatura (Celsius, Fahrenheit, Kelvin) cada vez que se presiona el botón, asegurando que la acción solo se realice una vez por cada pulsación del botón.
    uint32_t ticks_log = 0;             //ticks_log es una variable que se utiliza para llevar un registro del tiempo transcurrido desde la última vez que se imprimió la temperatura en el log.       

    while (1) {                         //mutex para cuando deja de enviar el comando 
        // 1. Temperatura
        float tc = ntc_leer_celsius(adc, CH_NTC);

        // 2. LED1 — color según rango de temperatura
        xSemaphoreTake(config.mutex, portMAX_DELAY); // Se toma el mutex para proteger el acceso a la configuración del sistema, se leen los rangos de temperatura y las intensidades de color para cada LED desde la configuración, y luego se libera el mutex para permitir que otras partes del programa accedan a la configuración de manera segura. Luego, se utiliza la temperatura medida (tc) para determinar el color del LED1 según los rangos configurados, estableciendo la intensidad de cada color (rojo, verde, azul) en función de si la temperatura está por debajo del rango verde, dentro del rango verde o por encima del rango rojo.
        float gmin = config.green.min;
        float gmax = config.green.max;
        float ir   = config.red.intensity   / 255.0f;
        float ig   = config.green.intensity / 255.0f;
        float ib   = config.blue.intensity  / 255.0f;
        xSemaphoreGive(config.mutex);

        if      (tc < gmin) rgb_led1_set(0, 0, ib);         //rojo ,,, verde,,,azul 
        else if (tc < gmax) rgb_led1_set(0, ig, 0);        
        else                rgb_led1_set(ir, 0, 0);

        // 3. Log periódico
        ticks_log += TICK_MS;
        xSemaphoreTake(config.mutex, portMAX_DELAY);
        uint32_t    intv = config.print_interval_s * 1000;          // Se calcula el intervalo de impresión en milisegundos multiplicando el valor configurado en segundos por 1000, 
        temp_unit_t unit = config.unit;
        xSemaphoreGive(config.mutex);

        if (ticks_log >= intv) {
            ESP_LOGI(TAG, "Temp: %.2f °%s", convertir(tc, unit), unidad_str(unit));
            ticks_log = 0;
        }

        // 4. Botón — cicla unidad C→F→K→C
        int btn = gpio_get_level(BTN_PIN);          // Lee el estado del botón
        if (btn == 0 && last_btn == 1) {            //compara el estado actual con el anterior para definir si esta presionado si sí eeseta presionado entra al mutex para cambiar la unidad  
            vTaskDelay(pdMS_TO_TICKS(50));
            xSemaphoreTake(config.mutex, portMAX_DELAY);
            if      (config.unit == UNIT_CELSIUS)    config.unit = UNIT_FAHRENHEIT;
            else if (config.unit == UNIT_FAHRENHEIT) config.unit = UNIT_KELVIN;
            else                                      config.unit = UNIT_CELSIUS;
            temp_unit_t u = config.unit;
            xSemaphoreGive(config.mutex);
            ESP_LOGI(TAG, "Unidad → %s", unidad_str(u));
        }
        last_btn = btn;

        // 5. Potenciómetro — umbral de temperatura 0–100°C
        int pot_raw = 0;                //pot_raw es una variable que se utiliza para almacenar el valor bruto leído desde el canal ADC asociado al potenciómetro, que se utiliza para determinar el umbral de temperatura en grados Celsius. Este valor se lee utilizando la función adc_oneshot_read, y luego se convierte a un rango de 0 a 100 grados Celsius para establecer el umbral de temperatura que se utilizará para controlar el estado del LED2, encendiéndolo en rojo cuando la temperatura medida supere este umbral y apagándolo cuando esté por debajo del umbral.
        adc_oneshot_read(adc, CH_POT, &pot_raw);            //no modifca el orginal 
        float umbral_temp = pot_raw * 100.0f / 4095.0f;

        if (tc >= umbral_temp)
            rgb_led2_set(1.0f, 0.0f, 0.0f);   // Rojo: temperatura supera umbral
        else
            rgb_led2_set(0.0f, 0.0f, 0.0f);   // Apagado

        ESP_LOGD(TAG, "POT umbral: %.1f°C | Temp: %.1f°C", umbral_temp, tc);

        vTaskDelay(pdMS_TO_TICKS(TICK_MS));         //delay de 30ms
    }
}