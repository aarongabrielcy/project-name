#include "pwManager.h"
#include "uartManager.h"
#include "sim7600.h"
#include "network.h"
#include "monitor.h"
#include "nvsManager.h"
#include "eventHandler.h"
#include "storageManager.h"

void app_main(void) {
    nvs_init();
    storage_init();
    power_init();
    get_event_loop();
    uart_init();
    uartManager_start();
    network_init();
    serialConsole_init();
    io_manager_init();
    start_uart_task();
    //esp_log_level_set("uartManager", ESP_LOG_NONE);  // Desactiva logs de "uartManager"
}