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
    esp_event_post_to(get_event_loop(), SYSTEM_EVENTS, KEEP_ALIVE, NULL, 0, portMAX_DELAY);
}

esp_event_loop_handle_t get_event_loop(void) {
    if (!event_loop_handle) {
        esp_event_loop_args_t loop_args = {
            .queue_size = 10,
            .task_name = "event_task",
            .task_priority = 5,
            .task_stack_size = 2048,
            .task_core_id = 0
        };
        esp_event_loop_create(&loop_args, &event_loop_handle);
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
    esp_timer_create_args_t timer_args = {
        .callback = &keep_alive_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "keep_alive_timer"
    };
    esp_timer_create(&timer_args, &keep_alive_timer);
    esp_timer_start_periodic(keep_alive_timer, keep_alive_interval * 1000);
}