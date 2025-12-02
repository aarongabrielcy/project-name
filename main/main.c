#include "pwManager.h"
#include "uartManager.h"
#include "sim7600.h"
#include "netManager.h"
#include "monitor.h"
#include "nvsManager.h"
#include "eventHandler.h"
#include "storageManager.h"
#include "deviceManager.h"
#include "otaManager.h"

void app_main(void) {
    nvs_init();
    storage_init();
    power_init();
    get_event_loop();
    uart_init();
    uartManager_start();
    serialConsole_init();
    start_event_handler_uart();
    seco_init();
    out2_init();
    io_manager_init();
    device_init();
    //ota_manager_mark_valid_if_pending();
    //esp_log_level_set("uartManager", ESP_LOG_NONE);  // Desactiva logs de "uartManager"
}