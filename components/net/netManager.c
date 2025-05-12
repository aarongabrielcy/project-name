// wifiManager.c
#include "netManager.h"
#include "nvsManager.h"  // Tu componente NVS
#include "eventHandler.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "tcpServer.h"

static const char *TAG = "WIFI_MANAGER";
static bool wifi_connected = false;
static bool wifi_enabled = false;
static esp_netif_t *ap_netif = NULL;
char pass_wifi[30];

static TaskHandle_t wifi_manager_task_handle = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_CONNECTED:
                ESP_LOGI(TAG, "Conectado a Wi-Fi");
                wifi_connected = true;
                esp_event_post(SYSTEM_EVENTS, WIFI_CONNECTED, NULL, 0, portMAX_DELAY);
                break;
            case WIFI_DISCONNECTED:
                ESP_LOGI(TAG, "Desconectado de Wi-Fi");
                wifi_connected = false;
                esp_event_post(SYSTEM_EVENTS, WIFI_DISCONNECTED, NULL, 0, portMAX_DELAY);
                break;
            case WIFI_EVENT_AP_STACONNECTED: 
                wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
                ESP_LOGI(TAG, "Cliente conectado, AID=%d, MAC="MACSTR, event->aid, MAC2STR(event->mac));
                break;
                
            case WIFI_EVENT_AP_STADISCONNECTED: 
                wifi_event_ap_stadisconnected_t *event_disc = (wifi_event_ap_stadisconnected_t *)event_data;
                ESP_LOGI(TAG, "Cliente desconectado, AID=%d, MAC="MACSTR, event_disc->aid, MAC2STR(event_disc->mac));
                break;
                
            default:
                break;
        }
    }
}

esp_err_t start_wifi_connection(const char *ssid, const char *password) {
    esp_err_t err;

    if (ap_netif == NULL) {
        ap_netif = esp_netif_create_default_wifi_ap();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if ((err = esp_wifi_init(&cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "Error en esp_wifi_init: %s", esp_err_to_name(err));
        return err;
    }

    if (wifi_enabled) {
        esp_wifi_stop();
        wifi_enabled = false;
    }

    if ((err = esp_wifi_set_mode(WIFI_MODE_AP)) != ESP_OK) {
        ESP_LOGE(TAG, "Error al configurar modo Wi-Fi: %s", esp_err_to_name(err));
        return err;
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "",
            .password = "",
            .ssid_len = 0,
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password));

    if (strlen(password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    if ((err = esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config)) != ESP_OK) {
        ESP_LOGE(TAG, "Error al configurar Wi-Fi: %s", esp_err_to_name(err));
        return err;
    }

    if ((err = esp_wifi_start()) != ESP_OK) {
        ESP_LOGE(TAG, "Error al iniciar Wi-Fi: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Punto de acceso iniciado. SSID: %s", ssid);
    wifi_enabled = true;
    return ESP_OK;
}

void wifi_manager_task(void *param) {
    while (1) {
        if (wifi_enabled && !wifi_connected) {
            /*ESP_LOGI(TAG, "Esperando que un cliente se conecte al AP...");*/
            vTaskDelay(pdMS_TO_TICKS(5000));  // Espera de 5 segundos antes de verificar nuevamente
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

esp_err_t wifi_manager_init() {
    esp_err_t err;

    ESP_LOGI(TAG, "Inicializando Wi-Fi...");

    if ((err = esp_netif_init()) != ESP_OK) {
        ESP_LOGE(TAG, "Error en esp_netif_init: %s", esp_err_to_name(err));
        return err;
    }

    if ((err = esp_event_loop_create_default()) != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Error creando loop de eventos: %s", esp_err_to_name(err));
        return err;
    }

    // Desregistrar el handler por si ya fue registrado
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);

    // Registrar después de asegurar que el loop está creado
    if ((err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL)) != ESP_OK) {
        ESP_LOGE(TAG, "Error al registrar manejador de eventos Wi-Fi: %s", esp_err_to_name(err));
        return err;
    }

    if (xTaskCreate(&wifi_manager_task, "wifi_manager_task", 4096, NULL, 5, &wifi_manager_task_handle) != pdPASS)
 {
        ESP_LOGE(TAG, "No se pudo crear la tarea Wi-Fi");
        return ESP_FAIL;
    }

    return ESP_OK;
}

bool wifi_manager_enable(bool enable) {
    char ssid[20] = {0};
    char pass[10] = {0};

    if (enable) {
        if (nvs_read_str("device_id", ssid, sizeof(ssid)) == NULL) {
            ESP_LOGE(TAG, "SSID no encontrado en NVS");
            return false;
        }
        ESP_LOGI(TAG, "SSID: %s", ssid);

        if (nvs_read_str("password_wifi", pass, sizeof(pass)) == NULL) {
            ESP_LOGE(TAG, "Password WiFi no encontrado en NVS");
            return false;
        }
        ESP_LOGI(TAG, "PASS: %s", pass);

        if (wifi_manager_init() != ESP_OK) {      
            ESP_LOGE(TAG, "Error inicializando Wi-Fi manager");
            return false;
        }
        if (start_wifi_connection(ssid, pass) != ESP_OK) {
            ESP_LOGE(TAG, "Error iniciando conexión Wi-Fi");
            return false;
        }
        tcp_server_start();
        return true;
    } else {
        ESP_LOGI(TAG, "Deshabilitando Wi-Fi...");
        tcp_server_stop();
        if (esp_wifi_stop() != ESP_OK) {
            ESP_LOGE(TAG, "Error deteniendo Wi-Fi");
            return false;
        }
        wifi_enabled = false;
        if (ap_netif != NULL) {
            esp_netif_destroy(ap_netif);
            ap_netif = NULL;
        }
        if (wifi_manager_task_handle != NULL) {
            vTaskDelete(wifi_manager_task_handle);
            wifi_manager_task_handle = NULL;
        }
        return true;
    }
}
