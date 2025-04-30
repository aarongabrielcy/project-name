#include "monitor.h"
#include <ctype.h>
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

//int value;

typedef struct {
    int number;
    char symbol;
    char value[64];  // Arreglo para almacenar el valor
} ParsedCommand;
char id[20];
//static void processValueCmd(char *value, int cmd);
static int validateCommand(const char *input,  ParsedCommand *parsed);
static char *proccessAction(ParsedCommand *parsed);
static char *proccessQuery(ParsedCommand *parsed);
static char *proccessQueryWithValue(ParsedCommand *parsed);
static char *processSVPT(const char *data);
static char *proccessCLOP(const char *data);

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
                ESP_LOGI(TAG,"%s",processCmd((char*)data));
                /*value = validCommand((char*)data);
                if(value == 0) {
                    printf("INVALID CMD:");
                }*/
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
//con esta funcion valida el formato del comando
/*int validCommand(const char *input) {
    char key[25], value[25];

    // Usar sscanf para dividir la cadena en key y value
    if (sscanf(input, "%[^=]=%s", key, value) == 2) {
        printf("Key: %s\n", key);
        printf("Value: %s\n", value);
        processValueCmd(value, atoi(key) ); /// con esta funcion valida que exista el comando
        return 1;
    } else {
        printf("Formato inválido. Usa key=value.\n");
        return 0;
    }   
}*/
const char *processCmd(const char *command) {
    static char buffer[256];  // Buffer estático compartido
    ParsedCommand cmd;

    switch (validateCommand(command, &cmd)) {
        case EMPTY:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "EMPTY CMD");
            return buffer;

        case QUERY_WITHOUT_VALUE:
            snprintf(buffer, sizeof(buffer), "%d%s&%s", cmd.number, cmd.value, proccessQuery(&cmd));
            return buffer;

        case ACTION:
            snprintf(buffer, sizeof(buffer), "%d%c%s&%s", cmd.number, cmd.symbol, cmd.value, proccessAction(&cmd));
            return buffer;

        case INVALID_CMD:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "INVALID CMD");
            return buffer;

        case INVALID_SYMBOL:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "INVALID SYMBOL");
            return buffer;

        case INVALID_ACTION:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "INVALID ACTION");
            return buffer;

        case INVALID_NUMBER:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "INVALID NUMBER");
            return buffer;

        case QUERY_WITH_VALUE:
            snprintf(buffer, sizeof(buffer), "%d%c%s&%s", cmd.number, cmd.symbol, cmd.value, proccessQueryWithValue(&cmd));
            return buffer;

        case INVALID_QUERY_VALUE:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "INVALID QUERY VALUE");
            return buffer;

        case INVALID_END_SYMBOL:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "INVALID END SYMBOL");
            return buffer;

        default:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "NO RESULT");
            return buffer;
    }
}
static int validateCommand(const char *input, ParsedCommand *parsed) {

    if (input == NULL || *input == '\0' || parsed == NULL) {
        return EMPTY;
    }

    int i = 0;
    while (isdigit((unsigned char)input[i])) {
        i++;
    }
    if (i == 0) {
        return INVALID_CMD;
    }

    // Guardar número
    char numberStr[6] = {0};
    if (i >= sizeof(numberStr)) {
        return INVALID_NUMBER;
    }
    strncpy(numberStr, input, i);
    parsed->number = atoi(numberStr);

    // Verificar símbolo
    char symbol = input[i];
    if (symbol != '#' && symbol != '?') {
        return INVALID_SYMBOL;
    }
    parsed->symbol = symbol;

    const char *value = input + i + 1;

    // Verificar terminación en '$'
    const char *end = strchr(value, '$');
    if (end == NULL) {
        return INVALID_END_SYMBOL;
    }

    int len = end - value;
    if (len >= sizeof(parsed->value)) {
        len = sizeof(parsed->value) - 1;
    }
    strncpy(parsed->value, value, len);
    parsed->value[len] = '\0';

    // Evaluar tipo de comando según el símbolo
    if (symbol == '#') {
        if (parsed->value[0] == '\0') {
            return INVALID_ACTION;
        }
        return ACTION;
    } else if (symbol == '?') {
        if (parsed->value[0] == '\0') {
            return QUERY_WITHOUT_VALUE;
        }
        if (!isdigit((unsigned char)parsed->value[0]) || parsed->value[1] != '\0') {
            return INVALID_QUERY_VALUE;
        }
        return QUERY_WITH_VALUE;
    }
    ESP_LOGI(TAG, "Saliendo de la validación...");
    return INVALID_CMD;
}
char *proccessAction(ParsedCommand *parsed) {
    switch (parsed->number) {
        case SVPT:
            return processSVPT(parsed->value);
        case CLOP:
            return proccessCLOP(parsed->value);
        default:
            
        return "NA";
    }
}
char *proccessQuery(ParsedCommand *parsed) {
    switch (parsed->number) {
        case DVID:
            if (nvs_read_str("device_id", id, sizeof(id)) != NULL) {
                return id;
            }else { return "ERR"; }
        default:
        return "NA";
    }
}

char *proccessQueryWithValue(ParsedCommand *parsed) {
    switch (parsed->number) {
        case OPST: 
            if(atoi(parsed->value) == 1) {
                return "1";
            }else if(atoi(parsed->value) == 2) {
                return "1";
            }
        return "0";
        default:
            return "NA";
        break;
    }
}
/*static void processValueCmd(char *value, int cmd) {
    switch (cmd) {
        case SVPT:
            processSVPT(value);
            break;
        case KLRP:
            printf("cambianto el tiempo de keep: %s\n", value);
            break;
        case DRNV:
            if (value != NULL) {
                nvs_delete_key(value);
            }else {printf("la llave NVS no existe");}
            if (atoi(value) == 1) {
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
            if(atoi(value) == 1 ) {
                char id[20];
                if (nvs_read_str("device_id", id, sizeof(id)) != NULL) {
                    printf("DVID:%s\n", id );
                }else {printf("ERROR:");}
            }
            break;
        case DVIM:
            if(atoi(value) == 1 ) {
                char simei[30];
                if (nvs_read_str("dev_simei", simei, sizeof(simei)) != NULL) {
                    printf("SIMEI:%s\n", simei);
                } else {printf("ERROR:");}
            }
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
                spiffs_process_and_delete_all_blocks();            
            }
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
            } else {vTaskDelay(pdMS_TO_TICKS(5));printf("El tiempo de reporte no puede ser menor 60s"); }
            break;
        case OPCT:
            if(atoi(value) == 1 ) {
                printf("OUT1=%s",value);
                outputControl(OUTPUT_1, atoi(value));
            } else if(atoi(value) == 0){
                outputControl(OUTPUT_1, atoi(value));
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
        case IGST:
            if(atoi(value) == 1 ) {
                printf("IGST:%d", power_get_ignition_state());
            } else { printf("ERROR"); }
            break;
        case SIID:
            if(atoi(value) == 1 ) {
                char iccid[30];
                if (nvs_read_str("sim_id", iccid, sizeof(iccid)) != NULL) {
                    printf("SIMID:%s\n", iccid);
                } else { printf("ERROR:"); }
            }
            break;
        default:
            printf("Comando NO valido");
            break;
    };
}*/
static char *proccessCLOP(const char *data) {
    char command[99];
snprintf(command, sizeof(command), "AT+CGDCONT=1,\"IP\",\"%s\"", data);
    printf("Comando AT: %s\n", command);
    if(1){
        return "OK1";
    }else {
        return "ERROR";
    }
}
static char *processSVPT(const char *data) {
    char server[32];  // Almacena la IP
    char port[6];     // Almacena el puerto (máximo 5 dígitos + terminador)

    // Buscar la posición del ':' en la cadena
    char *ptr = strchr(data, ':');
    if (ptr == NULL) {
        return "Error format";
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
        return "OK2";
    }else {
        return "ERROR";
    }    
}


