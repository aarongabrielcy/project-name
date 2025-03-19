#include "pwManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sim7600.h"
#include "trackerData.h"
#include "eventHandler.h"

#define TAG "POWER"

static int ledState = 0;
static int fixState = 0;
static bool ignition_state = false;
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
void led_task(void *arg) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    while (1) {
        TickType_t interval = (fixState == 1) ? pdMS_TO_TICKS(150) : pdMS_TO_TICKS(1000); // Rápido (150ms) o lento (1000ms)   
        ledState = !ledState;  // Alterna el LED
        gpio_set_level(GNSS_LED_PIN, ledState);
        vTaskDelayUntil(&lastWakeTime, interval);
    }
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
    xTaskCreatePinnedToCore(led_task, "led_task", 2048, NULL, 2, NULL, tskNO_AFFINITY);
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
    ESP_LOGI(TAG, "Leyendo estado de entradas y salidas...");
    while (1) {
        bool current_state = gpio_get_level(IGNITION_PIN);
        if (current_state != last_state) {  // Detectar cambio de estado
            ignition_state = current_state; // Guardar nuevo estado
            ESP_LOGI(TAG, "Ignition %s", !current_state ? "ON" : "OFF"); /// si es false (LOW/0V) es encendido y true (HIGH/3.3V o 5V) es apagado Por la configuración PULL UP
            // Obtener el event loop
            esp_event_loop_handle_t loop = get_event_loop();

            if (loop) {
                esp_err_t err = esp_event_post_to(loop, SYSTEM_EVENTS, 
                                                  ignition_state ? IGNITION_OFF : IGNITION_ON, 
                                                  NULL, 0, portMAX_DELAY);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Error enviando evento: %s", esp_err_to_name(err));
                }
            }
        }
        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(50));  // Pequeño retraso para evitar falsos positivos
    }
}
void io_manager_init() {
    gpio_set_direction(IGNITION_PIN, GPIO_MODE_INPUT);
    xTaskCreate(io_monitor_task, "io_monitor_task", 2048, NULL, 5, NULL);
}
bool power_get_ignition_state() {
    //return gpio_get_level(IGNITION_PIN);
    return ignition_state;
}
void set_gnss_led_state(int state) {
    fixState = state; // Cambia la velocidad de parpadeo
}
