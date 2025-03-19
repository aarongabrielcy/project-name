#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(SYSTEM_EVENTS);

typedef enum {
    IGNITION_ON,
    IGNITION_OFF,
    KEEP_ALIVE,
    TRACKING_RPT,
    DEFAULT
} system_event_t;

esp_event_loop_handle_t get_event_loop(void);
void set_keep_alive_interval(uint32_t interval_ms);
void start_keep_alive_timer(void);

#endif // EVENT_HANDLER_H