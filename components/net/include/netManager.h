#ifndef NET_MANAGER_H
#define NET_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"

esp_err_t wifi_manager_init(void);
bool wifi_manager_enable(bool enable);
void wifi_manager_task(void *param);
#endif
