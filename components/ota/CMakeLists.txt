idf_component_register(
    SRCS "otaManager.c"
    INCLUDE_DIRS "include"
    REQUIRES sim esp_timer nvs_flash esp_system esp_http_client esp_https_ota app_update
)