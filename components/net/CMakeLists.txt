idf_component_register(
    SRCS "tcpServer.c" "netManager.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_netif common lwip
)