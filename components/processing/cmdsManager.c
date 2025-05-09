#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cmdsManager.h"
#include "smsData.h"
#include "utilities.h"
#include "esp_log.h"
#include "monitor.h"
#include "nvsManager.h"
#include "sim7600.h"
#include "pwManager.h"
#include "eventHandler.h"

static const char *TAG = "cmdsManager";
char dev_imei[20];
int dev_rst_count;
smsData_t smsData = {0};  // Definición e inicialización de la variable global

static void smsDelete(char *command);
static int smsResponse(char * phone, char *cmd_response);

void parseSMS(char *message) {
    // Inicializamos los campos a cadena vacía
    smsData.sender_phone[0] = '\0';
    smsData.imei_received[0] = '\0';
    smsData.cmd_received[0] = '\0';

    // Eliminar comillas dobles (función implementada previamente)
    char *clean_sms = removeDoubleQuotesInPlace(message);
    
    // Variable auxiliar para recorrer la cadena
    char *cursor = clean_sms;
    char *token;
    int tokenIndex = 0;
    
    while ((token = strsep(&cursor, ",")) != NULL) {
        // Si el token está vacío, asignamos un valor por defecto, por ejemplo "N/A"
        if (token[0] == '\0') {
            token = "N/A";  // Valor por defecto para campos vacíos
        }

        // Aquí, según la posición, asignamos a cada variable
        if (tokenIndex == 0) { 
            strncpy(smsData.cmdat_sms, token, sizeof(smsData.cmdat_sms) - 1);
            smsData.cmdat_sms[sizeof(smsData.cmdat_sms) - 1] = '\0';
        }else if (tokenIndex == 2) {         
            strncpy(smsData.sender_phone, token, sizeof(smsData.sender_phone) - 1);
            smsData.sender_phone[sizeof(smsData.sender_phone) - 1] = '\0';
        } else if (tokenIndex == 6) { 
            strncpy(smsData.imei_received, token, sizeof(smsData.imei_received) - 1);
            smsData.imei_received[sizeof(smsData.imei_received) - 1] = '\0';
        } else if (tokenIndex == 7) {  
            strncpy(smsData.cmd_received, token, sizeof(smsData.cmd_received) - 1);
            smsData.cmd_received[sizeof(smsData.cmd_received) - 1] = '\0';
        } else if (tokenIndex == 8) {
            strncpy(smsData.sms_flag, token, sizeof(smsData.sms_flag) - 1);
            smsData.sms_flag[sizeof(smsData.sms_flag) - 1] = '\0';
        }
        // También puedes imprimir o procesar otros tokens si lo deseas:
        // Por ejemplo, para ver el contenido de cada token:
        // printf("Token[%d]: %s\n", tokenIndex, token);

        tokenIndex++;
    }

    if(nvs_read_str("device_id", dev_imei, sizeof(dev_imei)) != NULL) {

    }
    dev_rst_count = nvs_read_int("dev_reboots");

    ESP_LOGI(TAG, "DEV_ID:%s, PHONE:%s, RECEI_IMEI:%s, CMD:%s, FLAG:%s, AT:%s",formatDevID(dev_imei), smsData.sender_phone, smsData.imei_received, smsData.cmd_received, smsData.sms_flag, smsData.cmdat_sms);
    if(strcmp(formatDevID(dev_imei), smsData.imei_received) == 0) {
        char *responseCMD = processCmd(smsData.cmd_received);
        vTaskDelay(pdMS_TO_TICKS(7000));
        if(smsResponse(smsData.sender_phone, responseCMD ) ) {
            vTaskDelay(pdMS_TO_TICKS(3000));
            smsDelete(smsData.cmdat_sms);
            if(strcmp(responseCMD, "18#1&RST") == 0) {
                vTaskDelay(pdMS_TO_TICKS(2000));
                power_off_module();
                if(!state_module() ) {
                    power_restart();
                }
            }            
        }
    } else {ESP_LOGI(TAG,"message:%s", formatDevID(dev_imei));}
}

static int smsResponse(char * phone, char *cmd_response) {
    char command[50];
    char response[100];
    char ctrl_z_str[2] = { 0x1A, '\0' };  // Cadena con Ctrl+Z y terminador nulo
    snprintf(command, sizeof(command), "AT+CMGS=\"%s\"", phone);
    ESP_LOGI(TAG, "SEND AT_CMD: %s\n", command);
    if(sim7600_sendReadCommand(command) ) {
      snprintf(response, sizeof(response), "%s,%s",formatDevID(dev_imei), cmd_response);
      sim7600_sendATCommand(response);
      sim7600_sendATCommand(ctrl_z_str);
      return 1;
    }
    return 0;
}

static void smsDelete(char *command) {
    const char *pos = strchr(command, '=');
    if (pos != NULL) {
        ESP_LOGI(TAG, "DELETE SMS:%s",pos + 1);
        char command[50];
        snprintf(command, sizeof(command), "AT+CMGD=%s", pos+1);
        ESP_LOGI(TAG, "SED AT: %s\n", command);
        sim7600_sendATCommand(command);
    }
}