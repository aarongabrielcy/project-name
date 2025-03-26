#include "eventHandler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"

ESP_EVENT_DEFINE_BASE(SYSTEM_EVENTS);

static esp_event_loop_handle_t event_loop_handle = NULL;
static esp_timer_handle_t keep_alive_timer = NULL;
static uint32_t keep_alive_interval = 600000; // 10 minutos por defecto

static void keep_alive_callback(void *arg) {
    uint32_t keep_alive_data = keep_alive_interval;
    esp_event_post_to(get_event_loop(), SYSTEM_EVENTS, KEEP_ALIVE, &keep_alive_data, sizeof(uint32_t), portMAX_DELAY);
}

esp_event_loop_handle_t get_event_loop(void) {
    //static bool initialized = false;

    if (!event_loop_handle) {
        ESP_LOGW("EVENT_HANDLER", "Event loop no inicializado. Creando...");
        
        esp_event_loop_args_t loop_args = {
            .queue_size = 10,
            .task_name = "event_task",
            .task_priority = 5,
            .task_stack_size = 4096,  // Incrementado el stack por seguridad
            .task_core_id = 0
        };

        esp_err_t err = esp_event_loop_create(&loop_args, &event_loop_handle);
        if (err != ESP_OK) {
            ESP_LOGE("EVENT_HANDLER", "Error creando event loop: %s", esp_err_to_name(err));
            return NULL;
        }
        
        //initialized = true;
    }   
    return event_loop_handle;
}

void set_keep_alive_interval(uint32_t interval_ms) {
    keep_alive_interval = interval_ms;
    if (keep_alive_timer) {
        esp_timer_stop(keep_alive_timer);
        esp_timer_start_periodic(keep_alive_timer, keep_alive_interval * 1000);
    }
}

void start_keep_alive_timer(void) {
    if (keep_alive_timer != NULL) {
        ESP_LOGW("EVENT_HANDLER", "keep_alive_timer ya está en ejecución");
        return;
    }

    esp_timer_create_args_t timer_args = {
        .callback = &keep_alive_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "keep_alive_timer"
    };
    if (esp_timer_create(&timer_args, &keep_alive_timer) == ESP_OK) {
        esp_timer_start_periodic(keep_alive_timer, keep_alive_interval * 1000);
        ESP_LOGI("EVENT_HANDLER", "keep_alive_timer iniciado");
    } else {
        ESP_LOGE("EVENT_HANDLER", "Error creando el keep_alive_timer");
    }
}

void stop_keep_alive_timer(void) {
    if (keep_alive_timer != NULL) {
        esp_timer_stop(keep_alive_timer);
        esp_timer_delete(keep_alive_timer);
        keep_alive_timer = NULL;
        ESP_LOGI("EVENT_HANDLER", "keep_alive_timer detenido y eliminado");
    }
}
