#include "uartManager.h"
#include "sim7600.h"
#include "trackerData.h"
#include "moduleData.h"
#include "pwManager.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "network.h"
#include "utilities.h"
#include "nvsManager.h"

static const char *TAG = "UART_MANAGER";

static int waitingForPrompt = 0;
char latitud[20];
char longitud[20];
bool ignition;
char date_time[34];
char* dev_id;
static void uart_task(void *arg) {
    char response[256];
    char message[256];
    while (1) {
        int len = uartManager_readEvent(response, sizeof(response));
        if (len > 0) {
            ESP_LOGI(TAG, "Evento UART: %s", response);
  
                /*if (strchr(response, '>') != NULL) {
                    ESP_LOGI(TAG, "Recibido '>', enviando datos GNSS...");
                    //network_sendData(gnssData);
                    sim7600_sendATCommand(message);
                    waitingForPrompt = 0;
                }
                else*/ if (strstr(response, "+CGNSSINFO:") != NULL) {
                    ESP_LOGI(TAG, "Evento GNSS detectado.");
                    parseGPS(response);
                                        
                    strcpy(latitud, formatCoordinates(tkr.lat, tkr.ns));
                    ESP_LOGI(TAG, "Lat=> %s", latitud); 

                    strcpy(longitud, formatCoordinates(tkr.lon, tkr.ew));
                    ESP_LOGI(TAG, "Lon=> %s", longitud); 

                    ignition = !power_get_ignition_state();
                    if(tkr.fix) {
                        snprintf(date_time, sizeof(date_time), "%s;%s", formatDate(tkr.date), formatTime(tkr.utctime));
                        power_blink_gnss_led(tkr.fix);
                    }else {
                        ESP_LOGI(TAG, "dateTime del modulo SIM");
                        if(uartManager_sendReadUart("AT+CCLK?") ){
                        }
                        power_blink_gnss_led(tkr.fix);   
                    }
                    ESP_LOGI(TAG, "Enviando datos GNSS ->date time: %s Lat: %s, Lon: %s, Velocidad: %.2f, Fix: %d, ign: %d, id: %s", 
                        date_time, latitud, longitud, tkr.speed, tkr.fix, ignition, dev_id);
                    
                    snprintf(message, sizeof(message), "STT;%s;3FFFFF;95;1.0.21;1;%s;04BB4A02;334;21;3C1F;18;%s;%s;%.2f;%.2f;%d;%d;0000000%d;00000000;1;1;0929;4.1;14.19",formatDevID(dev_id), date_time, latitud, longitud,tkr.speed, tkr.course, tkr.gps_svs, tkr.fix, ignition);

                    char sendCommand[50];
                    snprintf(sendCommand, sizeof(sendCommand), "AT+CIPSEND=0,%d", (int)strlen(message)); //FORMA EL COMANDO "AT+CIPSEND=0,length"
                    if(uartManager_sendReadUart(sendCommand) ){
                        sim7600_sendATCommand(message);
                    }
                    //sim7600_sendATCommand(sendCommand); 
                    waitingForPrompt = 1;
                } else if (strstr(response, "+CIPSEND:") != NULL) {
                    ESP_LOGI(TAG, "Envío exitoso!");
                    
                }else if (strstr(response,"READY") != NULL || strstr(response,"+CPIN:") != NULL) {
                    ESP_LOGI(TAG, "Modulo listo para recibir comandos");
                    sim7600_basic_config();
                }else if(strstr(response, "NO SERVICE") != NULL) {
                    //////////////////////// falta el fix

                    ESP_LOGI(TAG, "guarda candenas con coordenadas en buffer");
                }else if(strstr(response, "+IPCLOSE:") != NULL)  {
                    ESP_LOGI(TAG, "evento de desconexión del servior TCP, reconectando...");
                }else if(strstr(response, "+CPSI:") != NULL) {
                    /* SI LOS DATOS SE PARSEAN ENTONCES LA CONEXION DE RED CELULAR SE PUEDE ESTABLECER ENTONCES
                       VALIDA SI "SIPSEND ES ERROR ENTONCES CONFIGURA EL SERVIDOR DE NUEVO" */
                }else if(strstr(response, "+CIPERROR:") != NULL) {
                    ///// SI LOS DATOS CPSI SE PARSEAN Y MARCA ESTE ERROR AL MANDAR AL SERVIDOR,  RECONECTA EL SERVIDOR
                } 
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_SIM, &uart_config);
    uart_set_pin(UART_SIM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_SIM, BUF_SIZE * 2, 0, 0, NULL, 0);
}

int uartManager_readEvent(char *buffer, int max_length) {
    int len = uart_read_bytes(UART_SIM, (uint8_t *)buffer, max_length - 1, pdMS_TO_TICKS(100));
    if (len > 0) {
        buffer[len] = '\0';
    }
    return len;
}
void uartManager_sendCommand(const char *command) {
    uart_write_bytes(UART_SIM, command, strlen(command));
    uart_write_bytes(UART_SIM, "\r\n", 2);
}
bool uartManager_sendReadUart(const char *command) {
    char response[BUF_SIZE];
    memset(response, 0, sizeof(response));

    ESP_LOGI(TAG, "Enviando comando: %s", command);
    uart_write_bytes(UART_SIM, command, strlen(command));
    uart_write_bytes(UART_SIM, "\r\n", 2);

    // Leer la respuesta del SIM7600
    int len = uart_read_bytes(UART_SIM, (uint8_t *)response, BUF_SIZE - 1, 500 / portTICK_PERIOD_MS);
    if (len > 0) {
        response[len] = '\0';
         // Limpiar la respuesta
         char *cleanedResponse = cleanResponse(response);        
        ESP_LOGI(TAG, "Respuesta:%s", cleanedResponse );
        // Check if response contains '>'
        if (strchr(cleanedResponse, '>') != NULL) {
            ESP_LOGI(TAG, "Detected '>', returning true");
            return true;
        }
        if(strstr(cleanedResponse, "SIMEI") != NULL) {
            dev_id = nvs_read_str("dev_id");
            if (dev_id != NULL) {
                printf("Dev ID: %s\n", dev_id);
                //free(dev_id);
            }else {
                char* dev_id = cleanATResponse(cleanedResponse);
                ESP_LOGI(TAG, "DEV_ID=>%s", dev_id);
                nvs_save_str("dev_id", dev_id);
            }
            return true;  
        }else if(strstr(cleanedResponse, "CCLK") != NULL) {
            char * cleaned_response = cleanATResponse(cleanedResponse);
            ESP_LOGI(TAG, "Formateando UTC... %s", cleaned_response); 
            char* result = getFormatUTC(cleaned_response);
            // Copiar el resultado en date_time sin desbordar
            strncpy(date_time, result, sizeof(date_time) - 1);
            // Asegurar terminación nula
            ESP_LOGI(TAG, "dateTime UTC... %s", result); 
            date_time[sizeof(date_time) - 1] = '\0';
        }else if (strstr(cleanedResponse, "OK") != NULL ) { ////////// si validas solo el comando "AT" busca mejor "AT,OK"
            ESP_LOGI(TAG, "Response ends with 'OK', returning true"); 
            //sim7600_sendATCommand("AT+CPIN?");
            return true;
        }
        
    } else {
        ESP_LOGW(TAG, "No hubo respuesta del módulo.");
        return false;

    }
    return false;
}
void uartManager_start() {
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 5, NULL);
}

