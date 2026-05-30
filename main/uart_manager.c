#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uart_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "UART"                              // Tag para logs relacionados con UART

void uart_init(void)                            // Inicializa el UART con la configuración especificada y muestra los comandos disponibles
{
    uart_config_t cfg = {                       // el cfg es una estructura que contiene la configuración del UART, incluyendo velocidad de baudios, bits de datos, paridad, bits de parada y control de flujo
        .baud_rate = UART_BAUD_RATE,            
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_driver_install(UART_PORT_NUM, UART_BUFFER_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &cfg);                                   // Configura el UART con los parámetros definidos

    ESP_LOGI(TAG, "UART listo. Comandos:");
    ESP_LOGI(TAG, "  SET UNIT [C|F|K]");
    ESP_LOGI(TAG, "  SET INTERVAL [s]");
    ESP_LOGI(TAG, "  SET [RED|GREEN|BLUE] [min] [max]");
    ESP_LOGI(TAG, "  SET INTENSITY [RED|GREEN|BLUE] [0-255]");
}

static void process_command(char *cmd, system_config_t *cfg)      // Procesa un comando recibido por UART y actualiza la configuración del sistema en consecuencia, utilizando un mutex para proteger el acceso concurrente a la configuración
{
    char  tok[16];                                      // Token para almacenar partes del comando (como color o unidad)
    int   iv;                                           // # entero para intervalos y intensidades
    float f1, f2;                                       // # desimales para rangos de temperatura

    // SET UNIT
    if (sscanf(cmd, "SET UNIT %3s", tok) == 1) {        // sscanf extrae la unidad de temperatura del comando y la almacena en 'tok'. Luego, se compara 'tok' con "C", "F" y "K" para determinar la unidad seleccionada por el usuario, y se actualiza la configuración del sistema en consecuencia, protegiendo el acceso a la configuración con un mutex para evitar condiciones de carrera.
        temp_unit_t u;
        if      (strcmp(tok, "C") == 0) u = UNIT_CELSIUS;    // strcmp compara el token con "C", "F" y "K" para determinar la unidad de temperatura seleccionada por el usuario, y asigna el valor correspondiente a la variable 'u'
        else if (strcmp(tok, "F") == 0) u = UNIT_FAHRENHEIT;
        else if (strcmp(tok, "K") == 0) u = UNIT_KELVIN;
        else { ESP_LOGW(TAG, "Unidad invalida: %s", tok); return; } // Si la unidad no es válida, se muestra un mensaje de advertencia y se sale de la función
        xSemaphoreTake(cfg->mutex, portMAX_DELAY);                  // Se toma el mutex para proteger el acceso a la configuración del sistema, se actualiza la unidad de temperatura en la configuración y luego se libera el mutex para permitir que otras partes del programa accedan a la configuración de manera segura.
        cfg->unit = u;
        xSemaphoreGive(cfg->mutex);                               
        ESP_LOGI(TAG, "Unidad → %s", tok);
        return;
    }

    // SET INTERVAL
    if (sscanf(cmd, "SET INTERVAL %d", &iv) == 1 && iv > 0) {       // sscanf extrae el intervalo de impresión en segundos del comando y lo almacena en 'iv'. Luego, se verifica que 'iv' sea un número entero positivo. Si es válido, se actualiza la configuración del sistema con el nuevo intervalo de impresión, protegiendo el acceso a la configuración con un mutex para evitar condiciones de carrera.
        xSemaphoreTake(cfg->mutex, portMAX_DELAY);
        cfg->print_interval_s = (uint32_t)iv;
        xSemaphoreGive(cfg->mutex);
        ESP_LOGI(TAG, "Intervalo → %d s", iv);
        return;
    }

    // SET INTENSITY [COLOR] [0-255]
    if (sscanf(cmd, "SET INTENSITY %15s %d", tok, &iv) == 2) {
        if (iv < 0 || iv > 255) { ESP_LOGW(TAG, "Intensidad invalida (0-255)"); return; }
        xSemaphoreTake(cfg->mutex, portMAX_DELAY);
        if      (strcmp(tok, "RED")   == 0) cfg->red.intensity   = (uint8_t)iv;
        else if (strcmp(tok, "GREEN") == 0) cfg->green.intensity = (uint8_t)iv;
        else if (strcmp(tok, "BLUE")  == 0) cfg->blue.intensity  = (uint8_t)iv;
        else { xSemaphoreGive(cfg->mutex); ESP_LOGW(TAG, "Color invalido: %s", tok); return; }
        xSemaphoreGive(cfg->mutex);
        ESP_LOGI(TAG, "Intensidad %s → %d", tok, iv);
        return;
    }

    // SET [COLOR] [min] [max]
    if (sscanf(cmd, "SET %15s %f %f", tok, &f1, &f2) == 3) {
        if (f1 >= f2) { ESP_LOGW(TAG, "min debe ser < max"); return; }
        xSemaphoreTake(cfg->mutex, portMAX_DELAY);
        if      (strcmp(tok, "RED")   == 0) { cfg->red.min   = f1; cfg->red.max   = f2; }
        else if (strcmp(tok, "GREEN") == 0) { cfg->green.min = f1; cfg->green.max = f2; }
        else if (strcmp(tok, "BLUE")  == 0) { cfg->blue.min  = f1; cfg->blue.max  = f2; }
        else { xSemaphoreGive(cfg->mutex);
        ESP_LOGW(TAG, "Color invalido: %s", tok); return; }
        xSemaphoreGive(cfg->mutex);
        ESP_LOGI(TAG, "Rango %s → %.1f–%.1f°C", tok, f1, f2);
        return;
    }

    ESP_LOGW(TAG, "Comando no reconocido: '%s'", cmd);
}

void uart_task(void *pvParameters)              //
{
    system_config_t *cfg = (system_config_t *)pvParameters;    //pcParameters es un puntero a la configuración del sistema que se pasa al crear la tarea UART. Se castea a un puntero de tipo system_config_t para acceder a la configuración del sistema dentro de la tarea UART, lo que permite leer y modificar la configuración de manera segura utilizando un mutex para proteger el acceso concurrente.
    uint8_t buf[128];                           

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, buf, sizeof(buf) - 1, pdMS_TO_TICKS(100));    
        if (len > 0) {
            buf[len] = '\0';
            char *p;
            if ((p = strchr((char *)buf, '\r'))) *p = '\0';         // r es un espacio y el n un salto de line ,,
            if ((p = strchr((char *)buf, '\n'))) *p = '\0';
            if (strlen((char *)buf) > 0) {
                ESP_LOGI(TAG, "RX: '%s'", (char *)buf);
                process_command((char *)buf, cfg);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));     // Se agrega un pequeño retraso para evitar que la tarea consuma demasiados recursos de CPU cuando no hay datos para leer, permitiendo que otras tareas del sistema se ejecuten de manera eficiente.
    }
}