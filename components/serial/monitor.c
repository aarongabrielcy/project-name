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
const char *cmds[CMD_COUNT] = {"DVID", "DVIM" ,"KLRP", "PWMC", "PWMS", "RTMC", "RTMS", "DRNV", "TMRP", "TKRP", "SVPT", "CLOP"};

static int validCommand(const char *input);
static void processValueCmd(char *value, int cmd);
static void processSVPT(const char *data);
static int proccessCLOP(const char *data);

static void serialConsole_task(void *arg) {
    uint8_t data[BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0'; // Convertir a string
            //ESP_LOGI(TAG, "Comando recibido: %s", (char*)data);
            if (strncmp((char*)data, "AT", 2) == 0 && SERIAL_DEBUG) {
                // Enviar comandos AT al SIM7600
                sim7600_sendATCommand((char*)data);
            } else if (strncmp((char*)data, "PWMS", 4) == 0) {
                printf((char*)data);
                validCommand((char*)data);
            }else if (strncmp((char*)data, "RTMS", 4) == 0) {
                printf((char*)data);
                validCommand((char*)data);
            } else if (strncmp((char*)data, "PWMC", 4) == 0) {
                printf((char*)data);
                validCommand((char*)data);
            }else if (strncmp((char*)data, "TKRP", 4) == 0) { ////// SE reinicia el modulo
                printf((char*)data);
                validCommand((char*)data);
            } else if (strncmp((char*)data, "RTMC", 4) == 0) {
                printf((char*)data);
                validCommand((char*)data);
            } else if (strncmp((char*)data, "TMRP", 4) == 0) {
                printf((char*)data);
                validCommand((char*)data);
            } else if (strncmp((char*)data, "DRNV", 4) == 0) {
                ESP_LOGW(TAG, "buffer => %s", (char*)data);
                printf((char*)data);
                validCommand((char*)data);
            } else if (strncmp((char*)data, "KLRP", 4) == 0) {
                ESP_LOGW(TAG, "cambiando reporte KLRP: %s", (char*)data);
                printf((char*)data);
                validCommand((char*)data);
            } else if (strncmp((char*)data, "SVPT", 4) == 0) {
                printf((char*)data);
                validCommand((char*)data);
            } else if (strncmp((char*)data, "CLOP", 4) == 0) {
                printf((char*)data);
                validCommand((char*)data);
            } else if (strncmp((char*)data, "DVID", 4) == 0) {
                printf((char*)data);
                validCommand((char*)data);
            } else if (strncmp((char*)data, "DVIM", 4) == 0) {
                printf((char*)data);
                validCommand((char*)data);
            }else {
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

    xTaskCreate(serialConsole_task, "serial_console_task", 8192, NULL, 5, NULL);
}
/********* AGREGAR A UTILS **********/
static int validCommand(const char *input) {
    if (input == NULL) {
        printf("Error: Entrada nula.\n");
        return 0;
    }

    // Buscar el signo '='
    char *equalSign = strchr(input, '=');
    if (equalSign == NULL) {
        printf("Error: No se encontró '=' en la entrada.\n");
        return 0;
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
                processValueCmd(value, RTMS);
                return 1;
            } else if(strcmp(key, "KLRP") == 0) {
                if(atoi(value) > 10) {
                    processValueCmd(value, KLRP);
                    return 1;
                }else {
                    return 0;
                    printf("EL valor debe ser mayor a 10.\n");  
                }
            } else if(strcmp(key, "DRNV") == 0) {
                processValueCmd(value, DRNV);
                return 1;
            } else if(strcmp(key, "RTMC") == 0 ) {
                processValueCmd(value, RTMC);
                return 1;
            } else if(strcmp(key, "TKRP") == 0) {
                processValueCmd(value, TKRP);
                return 1;
            } else if(strcmp(key, "TMRP") == 0) {
                processValueCmd(value, TMRP);
                return 1;
            } else if(strcmp(key, "SVPT") == 0) {
                processValueCmd(value, SVPT);
                return 1;
            } else if(strcmp(key, "CLOP") == 0) {
                processValueCmd(value, CLOP);
                return 1;
            } else if(strcmp(key, "DVID") == 0) {
                processValueCmd(value, DVID);
                return 1;
            } else if(strcmp(key, "DVIM") == 0) {
                processValueCmd(value, DVIM);
                return 1;
            }
            return 0;
        }
    }
    printf("Comando desconocido: %s\n", key);
    return 0;
}
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
            } else {printf("El tiempo de reporte no puede ser menor 5"); }
            
            break;
        case CLOP:
            proccessCLOP(value);
            break;
        case DVID:
            char *dev_id = nvs_read_str("dev_id");
            if (dev_id != NULL) {
                printf("DVID:%s\n", dev_id);
            }else {printf("Error:");}
            break;
        case DVIM:
            if(atoi(value) == 1 ) {
                sim7600_sendATCommand("AT+SIMEI?");//LIMPIAR RESPUESTA
            }
            break;
        default:
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


