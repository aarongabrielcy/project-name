#include "tcpServer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "string.h"

#define PORT 3333
static const char *TAG = "TCP_SERVER";

static TaskHandle_t tcp_server_handle = NULL;

static volatile bool tcp_server_running = true;

const char *data_to_send = "STT;2049830928;3FFFFF;95;1.0.21;1;20250506;17:13:19;79554408;334;20;3C2F;-1119;+21.023028;-89.584338;0.00;0.00;4;1;00000001;00000000;1;1;0929;4.1;14.19";

static void tcp_server_task(void *pvParameters) {
    int sockfd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sockfd < 0) {
        ESP_LOGE(TAG, "Error creando socket");
        vTaskDelete(NULL);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Error en bind");
        close(sockfd);
        vTaskDelete(NULL);
    }

    if (listen(sockfd, 1) < 0) {
        ESP_LOGE(TAG, "Error en listen");
        close(sockfd);
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Esperando cliente TCP...");

    while (tcp_server_running) {
        client_sock = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            if (tcp_server_running) ESP_LOGE(TAG, "Error en accept");
            continue;
        }

        ESP_LOGI(TAG, "Cliente TCP conectado");

        while (tcp_server_running) {
            int sent = send(client_sock, data_to_send, strlen(data_to_send), 0);
            if (sent < 0) {
                ESP_LOGE(TAG, "Error enviando datos");
                break;
            }

            ESP_LOGI(TAG, "Datos enviados");
            vTaskDelay(pdMS_TO_TICKS(30000));  // 30 segundos
        }

        ESP_LOGI(TAG, "Cliente desconectado o Wi-Fi desactivado");
        close(client_sock);
    }

    close(sockfd);
    ESP_LOGI(TAG, "Servidor TCP finalizado");
    TaskHandle_t self = tcp_server_handle;
    tcp_server_handle = NULL;
    vTaskDelete(self);
}

void tcp_server_start() {
    if (tcp_server_handle == NULL) {
        tcp_server_running = true;
        xTaskCreate(tcp_server_task, "tcp_server_task", 4096, NULL, 5, &tcp_server_handle);
    }
}

void tcp_server_stop() {
    if (tcp_server_handle != NULL) {
        tcp_server_running = false;

        // Esperamos un tiempo razonable para que la tarea se elimine sola
        for (int i = 0; i < 50; ++i) {  // espera hasta 500 ms
            if (tcp_server_handle == NULL) {
                ESP_LOGI(TAG, "Servidor TCP detenido correctamente");
                return;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        ESP_LOGW(TAG, "Servidor TCP no se detuvo a tiempo");
    }
}