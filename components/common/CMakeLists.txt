idf_component_register(
    SRCS "eventHandler.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event esp_timer
)