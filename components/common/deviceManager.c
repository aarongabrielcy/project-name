#include "deviceManager.h"
#include "nvsManager.h"
#include "esp_log.h"

void increment_reboot_counter(void) {
    const char* reboot_key = "dev_reboots";

    // Leer el valor actual
    int count = nvs_read_int(reboot_key);

    // Incrementar el contador
    count++;

    // Guardar el nuevo valor
    esp_err_t err = nvs_save_int(reboot_key, count);

    if (err == ESP_OK) {
        ESP_LOGI("REBOOT_TRACKER", "Reinicio #%d guardado correctamente", count);
    } else {
        ESP_LOGE("REBOOT_TRACKER", "Error al guardar el contador de reinicios: %s", esp_err_to_name(err));
    }
}




