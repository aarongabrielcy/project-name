#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void ota_manager_mark_valid_if_pending(void);
bool ota_manager_perform_update(void);
bool ota_prepare_http(const char *url);
bool initUpdate(const char *size);
#ifdef __cplusplus
}
#endif