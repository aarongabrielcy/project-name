idf_component_register(
    SRCS "uartManager.c"
    INCLUDE_DIRS "include"
    REQUIRES driver net sim processing utils power nvs common esp_event
)
