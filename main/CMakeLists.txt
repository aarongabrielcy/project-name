idf_component_register(
    SRCS "main.c"
    PRIV_REQUIRES spi_flash
    INCLUDE_DIRS "."
    REQUIRES nvs spf power uart net sim serial nvs_flash common spiffs
)
