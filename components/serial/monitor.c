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
#include "nvsManager.h"
#include "eventHandler.h"

#define TAG "SERIAL_CONSOLE"
#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
const char *cmds[CMD_COUNT] = {"KALV", "PWMC", "PWMS", "RTMC", "RTMS"};

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
            } else if (strncmp((char*)data, "PWMS", 5) == 0) {
                parseCommand((char*)data);
                // Encender módulo SIM
                //power_on_module();
            }else if (strncmp((char*)data, "RTMS", 5) == 0) {
                parseCommand((char*)data);
                // Encender módulo SIM
                //power_on_module();
            } else if (strncmp((char*)data, "PWMC", 5) == 0) {
                parseCommand((char*)data);
                // Apagar módulo SIM
                //power_off_module();
            } else if (strncmp((char*)data, "RTMC", 5) == 0) {
                parseCommand((char*)data);
                // Reiniciar ESP32
                //power_restart();
            } else if (strncmp((char*)data, "CLBR", 5) == 0) {
                ESP_LOGW(TAG, "Borrando buffer con el comando: %s", (char*)data);
                parseCommand((char*)data);
            } else if (strncmp((char*)data, "KALV", 5) == 0) {
                ESP_LOGW(TAG, "cambiando reporte KALV: %s", (char*)data);
                parseCommand((char*)data);
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

void parseCommand(const char *input) {
    if (input == NULL) {
        printf("Error: Entrada nula.\n");
        return;
    }

    // Buscar el signo '='
    char *equalSign = strchr(input, '=');
    if (equalSign == NULL) {
        printf("Error: No se encontró '=' en la entrada.\n");
        return;
    }

    // Separar clave y valor
    size_t keyLength = equalSign - input;
    char key[20];  // Ajustar tamaño según necesidad
    strncpy(key, input, keyLength);
    key[keyLength] = '\0';  // Agregar terminador de cadena

    char *value = equalSign + 1;  // Apunta después del '='

    // Buscar la clave en la lista de comandos
    for (int i = 0; i < CMD_COUNT; i++) {
        if (strcmp(key, cmds[i]) == 0) {
            if(strcmp(key, "RTMS") == 0) {
                printf("Comando encontrado: %s, Valor: %s\n", key, value);
            }else if(strcmp(key, "KALV") == 0) {
                if(atoi(value) > 10) {
                    printf("cambiaanto el tiempo de keep: %s\n", value);
                    set_keep_alive_interval(atoi(value) * 60000);
                }else {
                    printf("EL valor debe ser mayor a 10.\n");  
                }
                printf("Comando encontrado: %s, Valor: %s\n", key, value);
            }else if(strcmp(key, "CLBR") == 0) {
                if(atoi(value) == 1 ) {
                    printf("borrando NVS: %s, Valor: %s\n", key, value);
                    //nvs_flash_erase();
                }
            }
            return;
        }
    }

    printf("Comando desconocido: %s\n", key);
}

