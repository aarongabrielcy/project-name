#include "pwManager.h"
#include "uartManager.h"
#include "sim7600.h"
#include "network.h"
#include "monitor.h"
#include "nvsManager.h"
#include "eventHandler.h"

void app_main(void) {
    nvs_init();
    power_init();
    get_event_loop();       // Inicializar el loop de eventos
    uart_init();
    uartManager_start();
    network_init();
    serialConsole_init();
    io_manager_init();
    start_uart_task();
    start_keep_alive_timer();
    //set_keep_alive_interval(600000); // funcion para cambiar el intervalo agregar a "monitor.c" para dev
}
