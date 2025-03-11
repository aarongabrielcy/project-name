#ifndef NVSMANAGER_H
#define NVSMANAGER_H

#include <stdio.h>
#include "esp_err.h"

// Inicializa NVS (una sola vez)
esp_err_t nvs_init(void);

// Guardar y leer strings (ejemplo: dev_id, WiFi SSID, etc.)
esp_err_t nvs_save_str(const char* key, const char* value);
char* nvs_read_str(const char* key);

// Guardar y leer enteros (ejemplo: modo de configuración)
esp_err_t nvs_save_int(const char* key, int value);
int nvs_read_int(const char* key);

#endif
