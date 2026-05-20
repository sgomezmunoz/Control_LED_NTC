#include <stdio.h>
#include <string.h>

#include "uart_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"

#define TAG "UART_MANAGER"

// =========================
// UART INIT
// =========================

void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(
        UART_PORT_NUM,
        UART_BUFFER_SIZE,
        0,
        0,
        NULL,
        0
    );

    uart_param_config(
        UART_PORT_NUM,
        &uart_config
    );

    ESP_LOGI(TAG, "UART initialized");
}

// =========================
// PROCESS COMMAND
// =========================

static void process_command(char *command,
                            system_config_t *config)
{
    char color[16];

    float min;
    float max;

    if(sscanf(command,
              "SET %s %f %f",
              color,
              &min,
              &max) == 3)
    {
        xSemaphoreTake(config->mutex,
                       portMAX_DELAY);

        if(strcmp(color, "RED") == 0)
        {
            config->red.min = min;
            config->red.max = max;

            ESP_LOGI(TAG,
                     "RED updated: %.2f - %.2f",
                     min,
                     max);
        }

        else if(strcmp(color, "GREEN") == 0)
        {
            config->green.min = min;
            config->green.max = max;

            ESP_LOGI(TAG,
                     "GREEN updated: %.2f - %.2f",
                     min,
                     max);
        }

        else if(strcmp(color, "BLUE") == 0)
        {
            config->blue.min = min;
            config->blue.max = max;

            ESP_LOGI(TAG,
                     "BLUE updated: %.2f - %.2f",
                     min,
                     max);
        }

        else
        {
            ESP_LOGW(TAG,
                     "Invalid color");
        }

        xSemaphoreGive(config->mutex);
    }

    else
    {
        ESP_LOGW(TAG,
                 "Invalid command");
    }
}

// =========================
// UART TASK
// =========================

void uart_task(void *pvParameters)
{
    system_config_t *config =
        (system_config_t *)pvParameters;

    uint8_t data[128];

    while(1)
    {
        int len = uart_read_bytes(
            UART_PORT_NUM,
            data,
            sizeof(data) - 1,
            pdMS_TO_TICKS(100)
        );

        if(len > 0)
        {
            data[len] = '\0';

            char *pos;

            pos = strchr((char *)data, '\r');
            if(pos != NULL)
            {
                *pos = '\0';
            }

            pos = strchr((char *)data, '\n');
            if(pos != NULL)
            {
                *pos = '\0';
            }

            if(strlen((char *)data) == 0)
            {
                continue;
            }

            ESP_LOGI(TAG,
                    "Received: %s",
                    (char *)data);

            process_command(
                (char *)data,
                config
            );
        }
    }
}