#include "sim7600.h"
#include "uartManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "string.h"
#include "pwManager.h"
#define TAG "SIM7600"
#define MAX_RETRIES 10

void sim7600_init(const char *command) {
    ESP_LOGI(TAG, "Initializing SIM7600");

    int attempts = 0;
    const int max_attempts = 15;

    while (attempts < max_attempts) {
        if (uartManager_sendReadUart(command)) {
            ESP_LOGI(TAG, "SIM7600 successfully initialized");
            return;  // Exit function if communication is successful
        } else {
            ESP_LOGI(TAG, "Attempt %d failed, retrying...", attempts + 1);
            vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second before retrying
        }
        attempts++;
    }
    ESP_LOGE(TAG, "Failed to initialize SIM7600 after %d attempts", max_attempts);
}

void sim7600_basic_config() {
    bool ign = !power_get_ignition_state();
    sim7600_init("AT+SIMEI?");
    sim7600_init("AT+CGPS=1");
    sim7600_init("AT+CPSI=28");
    /*El valor del comando debe cambiar sengun el estado de la ignición*/
    if(ign) {
        ESP_LOGI(TAG, "Ignition State: %d", ign);
    }else {
        sim7600_init("AT+CGNSSINFO=255");
    }
    sim7600_init("AT+NETOPEN");
    sim7600_init("AT+CIPOPEN=0,\"TCP\",\"34.196.135.179\",5200");
    //sim7600_init("ATE0"); //NO REPLICA LOS COMANDOS ENVIADOS EN LAS RESPUESTAS "0"
}
void sim7600_reconnect_tcp_server() {
    /*vuelve boolana la funcion o valida los comandos que se ejecuten validando la respuesta
    de cada comando o consultalos después de ejecutarlos*/
    sim7600_sendATCommand("AT+CIPOPEN=0,\"TCP\",\"34.196.135.179\",5200");   
}
void sim7600_reconnect_tcp_service() {
    /*vuelve boolana la funcion o valida los comandos que se ejecuten validando la respuesta
    de cada comando o consultalos después de ejecutarlos*/
    sim7600_sendATCommand("AT+NETOPEN");
    sim7600_sendATCommand("AT+CIPOPEN=0,\"TCP\",\"34.196.135.179\",5200");   
}
void sim7600_sendATCommand(const char *command) {
    ESP_LOGI(TAG, "Enviando comando: %s", command);
    uartManager_sendCommand(command);
}

/** Donde se usa o donde puedo usar esta funcion? */
int sim7600_readResponse(char *buffer, int max_length) {
    int len = uartManager_readEvent(buffer, max_length);
    if (len > 0) {
        ESP_LOGI(TAG, "Respuesta recibida: %s", buffer);
    }
    return len;
}

bool sim7600_sendReadCommand(const char *command) {
    if (uartManager_sendReadUart(command)) {
        ESP_LOGI(TAG, "SIM7600 successfully initialized ~~~~");
        return true;  // Exit function if communication is successful
    } else {
        ESP_LOGI(TAG, "command failed %s", command);
        return false;
    }   
}