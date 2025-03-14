#include "monitor.h"
#include "uartManager.h"
#include "sim7600.h"
#include "network.h"
#include "pwManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"
#include <string.h>

#define TAG "SERIAL_CONSOLE"
#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)

static void serialConsole_task(void *arg) {
    uint8_t data[BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0'; // Convertir a string
            ESP_LOGI(TAG, "Comando recibido: %s", (char*)data);

            if (strncmp((char*)data, "AT", 2) == 0) {
                // Enviar comandos AT al SIM7600
                sim7600_sendATCommand((char*)data);
            } else if (strncmp((char*)data, "ON", 2) == 0) {
                // Encender módulo SIM
                //power_on_module();
            } else if (strncmp((char*)data, "OFF", 3) == 0) {
                // Apagar módulo SIM
                //power_off_module();
            } else if (strncmp((char*)data, "RESTART", 7) == 0) {
                // Reiniciar ESP32
                //power_restart();
            } else if (strncmp((char*)data, "CLNBFR", 7) == 0) {
                ESP_LOGW(TAG, "Borrando buffer con el comando: %s", (char*)data);
            } else {
                ESP_LOGW(TAG, "Comando no reconocido: %s", (char*)data);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void serialConsole_init() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    
    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    xTaskCreate(serialConsole_task, "serial_console_task", 4096, NULL, 5, NULL);
}
