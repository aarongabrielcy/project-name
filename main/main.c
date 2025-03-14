/////// convierte el uart a "C"
#include "pwManager.h"
#include "uartManager.h"
#include "sim7600.h"
#include "network.h"
#include "monitor.h"
#include "nvsManager.h"

void app_main(void) {
    nvs_init();
    power_init();
    uart_init();
    uartManager_start();
    network_init();
    serialConsole_init();
    io_manager_init();
}
