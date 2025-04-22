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
#include "utilities.h"
#include "storageManager.h"

#define TAG "SERIAL_CONSOLE"
#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)

int value;
static void processValueCmd(char *value, int cmd);
static void processSVPT(const char *data);
static int proccessCLOP(const char *data);

static void serialConsole_task(void *arg) {
    uint8_t data[BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0'; // Convertir a string
            if (strncmp((char*)data, "AT", 2) == 0 && SERIAL_DEBUG) {
                // Enviar comandos AT al SIM7600
                sim7600_sendATCommand((char*)data);
            } else {
                value = validCommand((char*)data);
                if(value == 0) {
                    printf("INVALID CMD:");
                }
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

    xTaskCreate(serialConsole_task, "serial_console_task", 8192, NULL, 5, NULL);
}
/********* AGREGAR A UTILS **********/
int validCommand(const char *input) {
    char key[25], value[25];

    // Usar sscanf para dividir la cadena en key y value
    if (sscanf(input, "%[^=]=%s", key, value) == 2) {
        printf("Key: %s\n", key);
        printf("Value: %s\n", value);
        processValueCmd(value, atoi(key) );
        return 1;
    } else {
        printf("Formato inválido. Usa key=value.\n");
        return 0;
    }   
}
///convertir la funcion a tipo char *
static void processValueCmd(char *value, int cmd) {
    switch (cmd) {
        case SVPT:
            processSVPT(value);
            break;
        case KLRP:
            printf("cambianto el tiempo de keep: %s\n", value);
            break;
        case DRNV:
            if (atoi(value) == 1 ) {
                printf("borrando NVS: %s\n", value);   
            } else if(atoi(value) == 0) {
                printf("Value NVS:%s", value);
            }
            break;
        case RTMS:
            printf("Comando RTMS:%s\n", value);
            break;
        case RTMC:
            if(atoi(value) == 1 ) {
                printf("Comando RTMC:%s\n", value);
            }
            break;
        case TKRP:
            if(atoi(value) == 1 ) {
                sim7600_sendATCommand("AT+CGNSSINFO"); 
            }
            break;
        case TMRP:
            if(atoi(value) >= 5 ) {
                char command[50];
                snprintf(command, sizeof(command), "AT+CGNSSINFO=%s", value);
                printf("Comando AT: %s\n", command);
            } else {printf("El tiempo de reporte no puede ser menor 5s"); }
            
            break;
        case CLOP:
            proccessCLOP(value);
            break;
        case DVID:
            char *dev_id = nvs_read_str("dev_id");
            if (dev_id != NULL) {
                printf("DVID:%s\n", formatDevID(dev_id) );
            }else {printf("ERROR:");}
            break;
        case DVIM:
        char *dev_imei = nvs_read_str("dev_id");
            if (dev_imei != NULL) {
                printf("DVID:%s\n", dev_imei);
            }else {printf("ERROR:");}
                break;
        case DLBF:
            if(atoi(value) > 0) {
                spiffs_delete_block(atoi(value));
            }
            break;
        case RABF:
            if(atoi(value) > 0) {
                char * block_bf = spiffs_process_blocks_buffer(atoi(value));
                if(block_bf != NULL) {
                    printf("RABF=%s", block_bf);
                } else { printf("not found!");}
            }
            break;
        case WTBF:
            if(atoi(value) == 1) {
                spiffs_process_and_delete_all_blocks();            }
            break;
        case DBMD:
            if(atoi(value) == 1 ) {
                printf("%s,OK", value);
            } else if(atoi(value) == 0){
                printf("%s,OK", value);
            }
            break;
        case CLDT:
            if(atoi(value) == 1 ) {
                sim7600_sendATCommand("AT+CPSI?"); 
            } else if(atoi(value) == 0){
                printf("%s,OK", value);
            }
            break;
        case CLRP:
            if(atoi(value) >= 60 ) {
                char command[50];
                snprintf(command, sizeof(command), "AT+CPSI=%s", value);
                printf("Comando AT: %s\n", command);
            } else {printf("El tiempo de reporte no puede ser menor 60s"); }
            break;
        case OUT1:
            if(atoi(value) == 1 ) {
                printf("OUT1=%s",value);
                engineCutOn();
            } else if(atoi(value) == 0){
                printf("OUT1=%s",value);
                engineCutOff();
            }
            break;
        case RTCT:
        if(atoi(value) == 1 ) {
            int dev_rst = nvs_read_int("dev_reboots");
            if (dev_rst != 0) {
                printf("reboots:%d\n", dev_rst);
            }else {printf("##cero reinicios##");}
        }
            break;
        default:
            printf("Comando NO valido");
            break;
    };
}
static int proccessCLOP(const char *data) {
    char command[99];
snprintf(command, sizeof(command), "AT+CGDCONT=1,\"IP\",\"%s\"", data);
    printf("Comando AT: %s\n", command);
    if(1){
        printf("OK");
        return 1;
    }else {
        return 0;
        printf("ERROR");
    }
    return 0;
}
static void processSVPT(const char *data) {
    char server[32];  // Almacena la IP
    char port[6];     // Almacena el puerto (máximo 5 dígitos + terminador)

    // Buscar la posición del ':' en la cadena
    char *ptr = strchr(data, ':');
    if (ptr == NULL) {
        printf("Error: formato incorrecto\n");
        return;
    }
    // Copiar la parte de la IP antes de ':'
    size_t len = ptr - data;
    strncpy(server, data, len);
    server[len] = '\0';  // Agregar terminador de cadena

    // Copiar la parte del puerto después de ':'
    strncpy(port, ptr + 1, sizeof(port) - 1);
    port[sizeof(port) - 1] = '\0';  // Asegurar terminador

    // Construir el comando AT
    char command[64];
    snprintf(command, sizeof(command), "AT+CIPOPEN=0,\"TCP\",\"%s\",%s", server, port);
    // Imprimir el comando resultante
    printf("Comando AT: %s\n", command);
    if(1){
        printf("OK");
    }else {
        printf("ERROR");
    }    
}


