idf_component_register(
    SRCS "deviceManager.c" "eventHandler.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event esp_timer nvs
)