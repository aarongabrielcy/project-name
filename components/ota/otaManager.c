#include "otaManager.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_image_format.h"
#include "driver/uart.h"
#include <string.h>
#include "sim7600.h"

#define TAG "OTA_MANAGER"
#define OTA_BUF_SIZE 1024
#define UART_NUM UART_NUM_1
#define OTA_URL "gruposisprovisa.mx/fw/1.0.1/project-name.bin"
int method = 0;
int httpstatus = 0;
int dataLength = 0;

void ota_manager_mark_valid_if_pending(void) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Imagen OTA pendiente, marcando como válida");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
}
bool ota_manager_perform_update(void) {
    ESP_LOGI(TAG, "Iniciando OTA vía RED CELULAR");
        char cmd[256];
    // 3. Iniciar OTA
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin falló: %s", esp_err_to_name(err));
        return false;
    }

    uint8_t buffer[OTA_BUF_SIZE];
    char at_line[64];
    int total_written = 0;

    // 4. Leer en bloques
    for (int offset = 0; offset < dataLength; offset += OTA_BUF_SIZE) {
        int chunk = (offset + OTA_BUF_SIZE > dataLength) ? (dataLength - offset) : OTA_BUF_SIZE;

        snprintf(cmd, sizeof(cmd), "AT+HTTPREAD=%d,%d", offset, chunk);
        sim7600_sendATCommand(cmd);

        // Leer "+HTTPREAD: DATA,<chunk>\r\n"
        /*sim7600_readResponse(at_line, sizeof(at_line), 1000);*/

        // Leer binario real
        int len = sim7600_readBinary(buffer, chunk, 3000);
        if (len != chunk) {
            ESP_LOGE(TAG, "Chunk incompleto en offset %d", offset);
            /*esp_ota_abort(ota_handle);
            return false;*/
        }
        /*err = esp_ota_write(ota_handle, buffer, len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write falló: %s", esp_err_to_name(err));
            esp_ota_abort(ota_handle);
            return false;
        }*/
        // Leer "OK"
        /*sim7600_readResponse(at_line, sizeof(at_line), 1000);*/
        total_written += len;
        ESP_LOGI(TAG, "TOTAL: %d bytes", total_written);

    }

    ESP_LOGI(TAG, "Firmware recibido: %d bytes", total_written);

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA finalizó con error: %s", esp_err_to_name(err));
        return false;
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo establecer partición de arranque: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "OTA exitosa. Reinicio requerido.");
    return true;
}

bool ota_prepare_http(const char *url) {
    ESP_LOGI(TAG, "Configurando HTTP en SIM7600");
    char *binary_name = "project-name.bin";
    //if (!sim7600_sendReadCommand("AT+HTTPTERM", "OK", 500)) sim7600_sendReadCommand("AT+HTTPINIT", "OK", 500); // por si ya estaba iniciado
    if (!sim7600_sendReadCommand("AT+HTTPTERM")) {
        if (!sim7600_sendReadCommand("AT+HTTPINIT")) { return false; }
    } else if (!sim7600_sendReadCommand("AT+HTTPINIT")) { return false; }

    char url_cmd[256];
    snprintf(url_cmd, sizeof(url_cmd), "AT+HTTPPARA=\"URL\",\"%s/%s\"", url, binary_name);
    ESP_LOGI(TAG, "ATcmd=>%s", url_cmd);
    if (!sim7600_sendReadCommand(url_cmd)) return false;
    /*
     * 
     */
    if (!sim7600_sendReadCommand("AT+HTTPACTION=0")) {
        ESP_LOGE(TAG, "Fallo en HTTPACTION");
        return false;
    }
    /*if (!sim7600_sendReadCommand("AT+HTTPREAD") ) { 
        ESP_LOGE(TAG, "Fallo en HTTPREAD");
        return false; 
    }*/

    return true;
}

bool initUpdate(const char *data) {
    if (sscanf(data, "%d,%d,%d", &method, &httpstatus, &dataLength) == 3) {
    ESP_LOGI(TAG, "method:%d status:%d size:%d", method, httpstatus, dataLength);
    return true;

    } else {
        printf("Error al parsear los datos\n");
        return false;
    }
}
