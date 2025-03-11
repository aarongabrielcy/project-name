#include "network.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "NETWORK";

void network_init() {
    ESP_LOGI(TAG, "Inicializando módulo de red...");
}
void network_sendData(const char *data) {
    ESP_LOGI(TAG, "Enviando datos al servidor: %s", data);
}

