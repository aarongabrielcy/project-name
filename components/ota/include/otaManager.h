#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "esp_check.h"

void ota_manager_mark_valid_if_pending(void);
esp_err_t ota_manager_perform_update(void);
esp_err_t ota_prepare_http(const char *url);
bool initUpdate(const char *size);
esp_err_t end_ota();
esp_err_t ota_writeChunk(uint8_t *buf, size_t  len);
#ifdef __cplusplus
}
#endif