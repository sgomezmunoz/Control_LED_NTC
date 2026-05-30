#ifndef UART_MANAGER_H
#define UART_MANAGER_H

#include "driver/uart.h"            //funciones y tipos de perifericos UART
#include "freertos/FreeRTOS.h"    
#include "freertos/semphr.h"        //para usar semáforos (mutex)

#define UART_PORT_NUM    UART_NUM_0    // UART0 es el puerto por defecto para logs y comunicación serial
#define UART_BAUD_RATE   115200       // Velocidad de comunicación Baud
#define UART_BUFFER_SIZE 1024       // Tamaño del buffer para recepción de datos


typedef enum {                     // Unidades de temperatura
    UNIT_CELSIUS    = 0,           
    UNIT_FAHRENHEIT = 1,
    UNIT_KELVIN     = 2
} temp_unit_t;                      // Tipo enumerado para unidades de temperatura

typedef struct {                    // struct las agrupa variables con el mismo nommbre
    float   min;                   
    float   max;
    uint8_t intensity;                  //  Intensidad del LED (0-255)
} color_range_t;

typedef struct {             
    color_range_t red;
    color_range_t green;
    color_range_t blue;

    temp_unit_t unit;
    uint32_t    print_interval_s;           // Intervalo de impresión en segundos

    SemaphoreHandle_t mutex;            // Semáforo para sincronización de acceso a la configuración 
} system_config_t;                      // Estructura principal que contiene toda la configuración del sistema, incluyendo rangos de temperatura para cada color, unidad de temperatura, intervalo de impresión y un mutex para proteger el acceso concurrente a esta configuración.

void uart_init(void);                   // Inicializa el UART con la configuración especificada y muestra los comandos disponibles
void uart_task(void *pvParameters);    //

#endif
