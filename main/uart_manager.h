#ifndef UART_MANAGER_H
#define UART_MANAGER_H

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Configuraciones UART
#define UART_PORT_NUM      UART_NUM_0
#define UART_BAUD_RATE     115200
#define UART_BUFFER_SIZE   1024

// Estructuras de configuración para la temperatura
typedef struct {
    float min;
    float max;
} range_t;

typedef struct {
    range_t red;
    range_t green;
    range_t blue;
    SemaphoreHandle_t mutex;
} system_config_t;

// Prototipos de funciones
void uart_init(void);
void uart_task(void *pvParameters);

#endif