#include "pwManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sim7600.h"
#include "trackerData.h"

#define TAG "POWER"

static unsigned long previousMillis = 0;
static int ledState = 0;
static bool ignition_state = false;
static TimerHandle_t ignition_timer = NULL; // Timer para detectar 30 minutos en OFF
static uint32_t ignition_off_interval = (7 * 60 * 1000);  // Intervalo en minutos (modificable) 
void power_init() {
    ESP_LOGI(TAG, "Inicializando Power Manager...");
    power_on_module();
    power_press_key();
    power_init_gnss_led();
    power_init_ignition();
}

// Encender el módulo SIM
void power_on_module() {
    gpio_reset_pin(POWER_SIM_PIN);
    ESP_LOGI(TAG, "Inicializando POWER SIM en PIN=%d", POWER_SIM_PIN);

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << POWER_SIM_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);
    gpio_set_level(POWER_SIM_PIN, 1);
}

// Apagar el módulo SIM
void power_off_module() {
    gpio_set_direction(POWER_SIM_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(POWER_SIM_PIN, 0);
}

// Reiniciar el ESP32
void power_restart() {
    esp_restart();
}

// Activar la tecla de encendido del módulo SIM
void power_press_key() {
    gpio_reset_pin(POWER_KEY_PIN);
    ESP_LOGI(TAG, "Inicializando POWER KEY en PIN=%d", POWER_KEY_PIN);

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << POWER_KEY_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);
    gpio_set_level(POWER_KEY_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(POWER_KEY_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(3000));
}

// Configurar el LED GNSS
void power_init_gnss_led() {
    gpio_reset_pin(GNSS_LED_PIN);
    ESP_LOGI(TAG, "Inicializando LED GNSS en PIN=%d", GNSS_LED_PIN);

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << GNSS_LED_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);
    gpio_set_level(GNSS_LED_PIN, 0); // Apagar inicialmente
}

// Parpadear el LED GNSS dependiendo del estado del fix
void power_blink_gnss_led(int fixState) {
    unsigned long currentMillis = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (currentMillis - previousMillis >= 1000) { // 1 segundo de intervalo
        previousMillis = currentMillis;
        ledState = (fixState == 0) ? !ledState : fixState;
        gpio_set_level(GNSS_LED_PIN, ledState);
    }
}

// Inicializar el pin de ignición
void power_init_ignition() {
    gpio_reset_pin(IGNITION_PIN);
    ESP_LOGI(TAG, "Inicializando IGNITION en PIN=%d", IGNITION_PIN);

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << IGNITION_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // Pull-up para detección de encendido
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);
}
void io_monitor_task(void *arg) {
    bool last_state = gpio_get_level(IGNITION_PIN);  // Estado inicial
    ignition_state = last_state;  // Guardar estado inicial
    /////////////////////////////// NO FUNCIONA EL TIMER PARA EL KEEP A LIVE
    while (1) {
        bool current_state = gpio_get_level(IGNITION_PIN);
        if (current_state != last_state) {  // Detectar cambio de estado
            ignition_state = current_state; // Guardar nuevo estado
            ESP_LOGI(TAG, "Ignición %s", !current_state ? "ENCENDIDA" : "APAGADA"); /// si es false (LOW/0V) es encendido y true (HIGH/3.3V o 5V) es apagado Por la configuración PULL UP
            if (!ignition_state) {
                // Si la ignición se enciende, detener el temporizador
                xTimerStop(ignition_timer, 0);
                ///TEN EN CUENTA QUE SI EL EQUIPO SE REINICIA ASIGNA DOS VECES EL VALOR DE AT+CGNSSINFO
                tkr.in_ig_st = 1;
                sim7600_sendATCommand("AT+CGNSSINFO=30");
            } else if(ignition_state) {
                // Si la ignición está OFF, iniciar el temporizador de 30 minutos
                xTimerStart(ignition_timer, 0);
                tkr.in_ig_st = 0;
                sim7600_sendATCommand("AT+CGNSSINFO=0");
                
            }    
        }
        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(50));  // Pequeño retraso para evitar falsos positivos
    }
}
static void ignition_timer_callback(TimerHandle_t xTimer) {
    if (!ignition_state) {  // Solo ejecutar si la ignición sigue apagada
        ESP_LOGI(TAG, "Ignición OFF durante 30 minutos. Ejecutando acción...");
        //función que necesites ejecutar
        sim7600_sendATCommand("AT+CGNSSINFO?");   
    }
}
void io_manager_init() {
    gpio_set_direction(IGNITION_PIN, GPIO_MODE_INPUT);
    // Crear el temporizador (no iniciado aún)
    ignition_timer = xTimerCreate("IgnitionTimer", pdMS_TO_TICKS(ignition_off_interval), pdFALSE, NULL, ignition_timer_callback);
    xTaskCreate(io_monitor_task, "io_monitor_task", 2048, NULL, 5, NULL);
}
// Leer el estado de ignición
bool power_get_ignition_state() {
    //return gpio_get_level(IGNITION_PIN);
    return ignition_state;
}