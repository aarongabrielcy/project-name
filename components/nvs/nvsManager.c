#include "nvsManager.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

#define STORAGE_NAMESPACE "storage"
static const char *TAG = "NVS_MANAGER";

// Inicializa NVS (Solo llamar una vez en `app_main()`)
esp_err_t nvs_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}
// Guardar un string con una clave específica
esp_err_t nvs_save_str(const char* key, const char* value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_str(handle, key, value);
    if (err == ESP_OK) {
        nvs_commit(handle);
        ESP_LOGI(TAG, "%s guardado: %s", key, value);
    }

    nvs_close(handle);
    return err;
}

// Leer un string con una clave específica
char* nvs_read_str(const char* key) {
    static char buffer[64];  //ESPACIO RESERVADO FIJO EN MEMORIA
    size_t required_size;
    nvs_handle_t handle;

    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return NULL;

    err = nvs_get_str(handle, key, NULL, &required_size);
    if (err != ESP_OK || required_size > sizeof(buffer)) {
        nvs_close(handle);
        return NULL;
    }

    err = nvs_get_str(handle, key, buffer, &required_size);
    nvs_close(handle);

    return (err == ESP_OK) ? buffer : NULL;
}

// Guardar un entero con una clave específica
esp_err_t nvs_save_int(const char* key, int value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_i32(handle, key, value);
    if (err == ESP_OK) {
        nvs_commit(handle);
        ESP_LOGI(TAG, "%s guardado: %d", key, value);
    }

    nvs_close(handle);
    return err;
}

//Leer un entero con una clave específica
int nvs_read_int(const char* key) {
    nvs_handle_t handle;
    int32_t value = 0;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return 0;

    err = nvs_get_i32(handle, key, &value);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "%s leído: %ld", key, (long)value);
    }
    nvs_close(handle);
    return value;
}



