#include "uartManager.h"
#include "sim7600.h"
#include "trackerData.h"
#include "moduleData.h"
#include "pwManager.h"
#include "serviceInfo.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "network.h"
#include "utilities.h"
#include "nvsManager.h"
#include "eventHandler.h"
#include "storageManager.h"
#include "cmdsManager.h"

static const char *TAG = "UART_MANAGER";

char latitud[20];
char longitud[20];
bool ignition = false;
char date_time[34];
char* dev_id;
char* sim_id;
bool redService = false;
bool configState = false;
int event = DEFAULT;
static int keep_alive_interval = 600000; // Valor en milisegundos (10 minutos)

void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_SIM, &uart_config);
    uart_set_pin(UART_SIM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_SIM, BUF_SIZE * 2, 0, 0, NULL, 0);
}
static void uart_task(void *arg) {
    char response[256];
    char message[256];

    while (1) {
        int len = uartManager_readEvent(response, sizeof(response));
        if (len > 0) {      
            if (strstr(response, "+CGNSSINFO:") != NULL ) {
                //ESP_LOGI(TAG, "Evento GNSS detectado.");
                parseGPS(response);
                strcpy(latitud, formatCoordinates(tkr.lat, tkr.ns));
                //ESP_LOGI(TAG, "Lat=> %s", latitud); 
                strcpy(longitud, formatCoordinates(tkr.lon, tkr.ew));
                //ESP_LOGI(TAG, "Lon=> %s", longitud); 
                if(tkr.fix) {
                    snprintf(date_time, sizeof(date_time), "%s;%s", formatDate(tkr.date), formatTime(tkr.utctime));
                }else {
                    if(uartManager_sendReadUart("AT+CCLK?") ){
                    }
                }
                switch (event) {       
                    case TRACKING_RPT:
                        //ESP_LOGI(TAG, "Evento TRAKING REPORT ~~~~~~~~~~~~~~~~~~~~~~~~");
                        snprintf(message, sizeof(message), "STT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;%d;%s;%s;%.2f;%.2f;%d;%d;0%d00000%d;00000000;1;1;0929;4.1;14.19",
                        formatDevID(dev_id), date_time,serInf.cell_id, serInf.mcc, serInf.mnc, serInf.lac_tac, serInf.rxlvl_rsrp, latitud, longitud,tkr.speed, tkr.course,
                        tkr.gps_svs, tkr.fix, tkr.tkr_course, ignition);
                        if(!sendToServer(message) ) {
                            ESP_LOGW(TAG, "error sending data,event:%d", TRACKING_RPT);/// para seguir usando los logs de ESP crea un enum de los TAGs para saber de que archivo viene
                            sim7600_sendATCommand("AT+CPSI?");
                        }
                    break;
                    case IGNITION_ON:
                        //ESP_LOGI(TAG, "Evento IGN ON ~~~~~~~~~~~~~~~~~~~~~~~~");  
                        snprintf(message, sizeof(message), "ALT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;%d;%s;%s;%.2f;%.2f;%d;%d;0000000%d;00000000;%d;;",
                        formatDevID(dev_id), date_time,serInf.cell_id, serInf.mcc, serInf.mnc, serInf.lac_tac, serInf.rxlvl_rsrp, latitud, longitud,tkr.speed, tkr.course,
                        tkr.gps_svs, tkr.fix, ignition, 33);  
                        if(!sendToServer(message) ) {
                            ESP_LOGW(TAG, "error sending data, event:%d",IGNITION_ON);    
                            sim7600_sendATCommand("AT+CPSI?");
                        }
                        event = TRACKING_RPT;
                    break;
                    case IGNITION_OFF:
                        //ESP_LOGI(TAG, "Evento IGN OFF ~~~~~~~~~~~~~~~~~~~~~~~~");
                        snprintf(message, sizeof(message), "ALT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;%d;%s;%s;%.2f;%.2f;%d;%d;0000000%d;00000000;%d;;",
                        formatDevID(dev_id), date_time,serInf.cell_id, serInf.mcc, serInf.mnc, serInf.lac_tac, serInf.rxlvl_rsrp, latitud, longitud,tkr.speed, tkr.course,
                        tkr.gps_svs, tkr.fix, ignition, 34);
                        if(!sendToServer(message) ) {
                            ESP_LOGW(TAG, "error sending data,event:%d",IGNITION_OFF);    
                            sim7600_sendATCommand("AT+CPSI?");
                        }
                        event = DEFAULT;
                        sim7600_sendATCommand("AT+CGNSSINFO=0");    
                    break;
                    case KEEP_ALIVE:
                        /* Valia que el keep a live se mande solo después de la ignición */
                        //ESP_LOGI(TAG, "Evento KEEP A LIVE ~~~~~~~~~~~~~~~~~~~~~~~~");
                        snprintf(message, sizeof(message), "ALV;%s",
                        formatDevID(dev_id));    
                        if(!sendToServer(message) ) {    
                            ESP_LOGW(TAG, "error sending data,event:%d",KEEP_ALIVE);
                            sim7600_sendATCommand("AT+CPSI?");
                        }
                        event = DEFAULT;
                    break;    
                    default:
                        //ESP_LOGI(TAG, "SIN EVENTO ~~~~~~~~~~~~~~~~~~~~~~~~");
                        /** cuando se reincia en esta linea es por que el id está vacio */
                        ESP_LOGW(TAG, "<head>\n<sys_mode>%s<oper>%s<cell_id>%s<mcc>%d<mnc>%d<lac>%s<rx_lvl>%d<date_time>%s,<lat>%s,<lon>%s,<speed>%.2f,<fix>%d,<ign>%d,<id>%s,<ccid>%s", 
                           serInf.sys_mode, serInf.oper_mode, serInf.cell_id, serInf.mcc, serInf.mnc, serInf.lac_tac, serInf.rxlvl_rsrp, date_time, latitud, longitud, tkr.speed, tkr.fix, ignition, dev_id, sim_id); 
                    break;
                }  
            } else if (strstr(response, "+NETOPEN: 0") != NULL) {
                char *net = cleanData(response, "NETOPEN");
                if(strstr(net, "0") != NULL) {
                    ESP_LOGI(TAG, "servicio tcp activo");    
                }
            } else if (strstr(response, "+CIPOPEN:") != NULL) {
                char *cip = cleanData(response, "CIPOPEN");
                if(strstr(cip, "0,0") != NULL) {
                    ESP_LOGI(TAG, "conexion a servidor tcp establecida!");
                }
            } else if (strstr(response,"READY") != NULL || strstr(response,"+CPIN:") != NULL) {
                ESP_LOGI(TAG, "Modulo listo para recibir comandos");
                if(!configState){
                    sim7600_basic_config();
                    configState = true;    
                }
                
            } else if(strstr(response, "+IPCLOSE:") != NULL) {
                ESP_LOGI(TAG, "Desconexión IPCLOSE ...");
                sim7600_reconnect_tcp_server();
            } else if(strstr(response, "+CPSI:") != NULL) { 
                //ESP_LOGI(TAG, "validando CPSI...");
                redService = parsePSI(response);

                if(redService){
                    //ESP_LOGI(TAG, "Parseando CPSI EXITOSO!");
                } else {
                    ESP_LOGI(TAG, "No fué posible parsear CPSI");
                } 
            } else if(strstr(response, "+IPD") != NULL)  {
                char * clean_idp = cleanResponse(response);
                ESP_LOGI(TAG, "CMD TCP => %s", clean_idp);
                ESP_LOGI(TAG, "clean CMD TCP => %s", cleanATResponse(clean_idp));

            } else if(strstr(response, "+CIPEVENT:") != NULL)  {
                char * cip_event = cleanData(response, "CIPEVENT");
                ESP_LOGI(TAG, "CIP EVENT => %s", cip_event);
            } else if (strstr(response, "+CMTI:") != NULL) {  
                /**Crea una funcion peridoca de cada 2 minutos que valide si tienes mensajes por si se pierde alguno **/
                printf("SMS Detectado, enviando comando para leer...\n");
                // Encontrar la posición de la coma ","
                char *comma_pos = strchr(response, ',');
                if (comma_pos == NULL) {
                    printf("Error: No se encontró el índice del SMS.\n");
                    return;
                }        
                // Obtener el índice después de la coma
                char index[10];  
                strcpy(index, comma_pos + 1);  // Copia el número del índice
        
                // Crear el comando "AT+CMGR="
                char command[20];
                snprintf(command, sizeof(command), "AT+CMGR=%s", index);
        
                printf("Comando a enviar: %s\n", command);
                 sim7600_sendATCommand(command);

            } else if(strstr(response, "+CMGR:") != NULL) {
                char * sms_long = cleanResponse(response);
                parseSMS(sms_long);

            } else if(strstr(response, "PB DONE") != NULL) {
                ESP_LOGI(TAG, "REACTIVANDO TRAKER REPORT: %s", response);
                sim7600_sendATCommand("AT+CGNSSINFO=30");
            } else { ESP_LOGE(TAG, "RD URT: %s", response); }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        set_gnss_led_state(tkr.fix);
    }
}
int uartManager_readEvent(char *buffer, int max_length) {
    int len = uart_read_bytes(UART_SIM, (uint8_t *)buffer, max_length - 1, pdMS_TO_TICKS(100));
    if (len > 0) {
        buffer[len] = '\0';
    }
    return len;
}
void uartManager_sendCommand(const char *command) {
    uart_write_bytes(UART_SIM, command, strlen(command));
    uart_write_bytes(UART_SIM, "\r\n", 2);
}
bool uartManager_sendReadUart(const char *command) {
    
    char response[BUF_SIZE];
    memset(response, 0, sizeof(response));

    ESP_LOGI(TAG, "Enviando comando: %s", command);
    uart_write_bytes(UART_SIM, command, strlen(command));
    uart_write_bytes(UART_SIM, "\r\n", 2);

    // Leer la respuesta del SIM7600
    int len = uart_read_bytes(UART_SIM, (uint8_t *)response, BUF_SIZE - 1, 500 / portTICK_PERIOD_MS);
    if (len > 0) {
        response[len] = '\0';
         // Limpiar la respuesta
         char *cleanedResponse = cleanResponse(response);
         if (cleanedResponse == NULL) {
            ESP_LOGE(TAG, "cleanResponse retornó NULL");
            return false;
        }    
        ESP_LOGE(TAG, "Respuesta:%s", cleanedResponse );
        // Check if response contains '>'
        if (strchr(cleanedResponse, '>') != NULL) {
            ESP_LOGE(TAG, "Detected '>', returning true");
            /// AQUI VOY A PONER LA RESPUESTA DEL SMS
            return true;

        } else if (strstr(cleanedResponse, "+CIPSEND:") != NULL) {
            if (cleanedResponse != NULL && command != NULL) {
                char *cleanSend = clean(cleanedResponse, command);
                if (cleanSend != NULL) {
                    ESP_LOGE(TAG, "CIPSEND CLEAN=>%s", cleanSend);
                } else if (cleanSend == NULL) {
                    ESP_LOGE(TAG, "clean() retornó NULL — cleanedResponse='%s', command='%s'", 
                             cleanedResponse ? cleanedResponse : "NULL",
                             command ? command : "NULL");
                    return false;
                }
                 
                return true;
            } else {
                ESP_LOGE(TAG, "Punteros NULL antes de llamar a clean()");
                return false;
            }
        } else if(strstr(cleanedResponse, "CCID") != NULL) {
            sim_id = nvs_read_str("sim_id");   
            if (sim_id != NULL) {
                ESP_LOGI(TAG, "Longitud real: %d", (int)strlen(sim_id));
                printf("SIM ID:%s\n", sim_id);
                if (strlen(sim_id) != 19) {
                    printf("SIMID Incorrecto: %s\n", sim_id);
                    nvs_delete_key(sim_id); 
                    ///Reiniciar dispositivo
                } else { 
                    ESP_LOGI(TAG, "SIMID correcto: %s", sim_id);
                    return true;
                }    
            } else {
                sim_id = cleanATResponse(cleanedResponse);
                ESP_LOGI(TAG, "SIM parseado: %s", sim_id);
                vTaskDelay(pdMS_TO_TICKS(5));
                ESP_LOGI(TAG, "Longitud: %d", strlen(sim_id));
                if (strlen(sim_id) == 19) {
                    ESP_LOGI(TAG, "SIM_ID=>%s", sim_id);
                    nvs_save_str("sim_id", sim_id);
                    return true;
                }
            }
            return false; 

        } else if(strstr(cleanedResponse, "SIMEI") != NULL) {
            dev_id = nvs_read_str("dev_id");
            if (dev_id != NULL) {
                ESP_LOGI(TAG, "Longitud real: %d", (int)strlen(dev_id));
                printf("Dev ID:%s\n", dev_id);
                if (strlen(dev_id) != 15) {
                    printf("imei Incorrecto: %s\n", dev_id);
                    nvs_delete_key(dev_id);
                    //REINICIAR EL DISPOSITIVO PARA QUE VUELVA A INTENTAR LEER EL IMEI 
                }else { 
                    ESP_LOGI(TAG, "IMEI correcto: %s", dev_id);
                    return true;
                }    
            } else {
                dev_id = cleanATResponse(cleanedResponse);
                ESP_LOGI(TAG, "IMEI parseado: %s", dev_id);
                vTaskDelay(pdMS_TO_TICKS(5));
                ESP_LOGI(TAG, "Longitud: %d", strlen(dev_id));
                if (strlen(dev_id) == 15) {
                    ESP_LOGI(TAG, "DEV_ID=>%s", dev_id);
                    nvs_save_str("dev_id", dev_id);
                    return true;  
                }
            }
            return false;
        } else if(strstr(cleanedResponse, "CCLK") != NULL) {
            char *cleaned_response = cleanATResponse(cleanedResponse);
            if (cleaned_response != NULL) {
                char *result = getFormatUTC(cleaned_response);
                if (result != NULL) {
                    strncpy(date_time, result, sizeof(date_time) - 1);
                    date_time[sizeof(date_time) - 1] = '\0';
                } else {
                    ESP_LOGE(TAG, "getFormatUTC retornó NULL");
                }
            }
        } else if(strstr(cleanedResponse, "+CIPERROR:") != NULL) {
            sim7600_sendATCommand("AT+CPSI?");
            char *err = cleanData(response, "AT+CIPSEND=0,");
            ESP_LOGI(TAG, "ERROR TCP:%s, estdo de la red:%d", err, redService);
            if(redService) {
                ESP_LOGI(TAG, "restableciendo conexión TCP...");
                if(strstr(err,"2") != NULL) {
                    ESP_LOGI(TAG, "ERROR DEL SERVICIO TCP");
                    sim7600_reconnect_tcp_service();

                } else if(strstr(err,"4") != NULL) {
                    ESP_LOGI(TAG, "ERROR DEL CONEXIÓN A SERVIDOR TCP");
                    sim7600_reconnect_tcp_server();
                }
            }
            return false;
        } else if (strstr(cleanedResponse, "OK") != NULL ) { ////////// si validas solo el comando "AT" busca mejor "AT,OK"
            ESP_LOGI(TAG, "Response ends with 'OK', returning true"); 
            //sim7600_sendATCommand("AT+CPIN?");
            return true;
        }
    } else {
        ESP_LOGW(TAG, "No hubo respuesta del módulo.");
        return false;
    }
    return false;
}
void uartManager_start() {
    xTaskCreate(uart_task, "uart_task", 8192, NULL, 5, NULL);
}
int sendToServer(char * message) {
    if(redService) {
        int total_blocks = get_total_block_count();
        printf("total block_buffer?>%d\n", total_blocks);
        if(total_blocks > 0) {
            int f_block = get_first_block_number();
            printf("block_%d.txt", f_block);
            char * buffer = spiffs_process_blocks_buffer(f_block);
            //forma el comando lee los carateres y sumale el del comando
            char sendCmdBuffer[30];
            snprintf(sendCmdBuffer, sizeof(sendCmdBuffer), "AT+CIPSEND=0,%d", (int)strlen(buffer)); //FORMA EL COMANDO "AT+CIPSEND=0,length"
            /*ESP_LOGI(TAG, "COMMAND to send:%s", sendCmdBuffer);
            printf("SEND BUFFER=>%s", buffer);*/
            if(uartManager_sendReadUart(sendCmdBuffer) ) {

                if(uartManager_sendReadUart(buffer) ){
                    ESP_LOGI(TAG, "Envío exitoso BUFFER!");
                    spiffs_delete_block(f_block); 
                }
            }
        }
        char sendCommand[30];
        snprintf(sendCommand, sizeof(sendCommand), "AT+CIPSEND=0,%d", (int)strlen(message)); //FORMA EL COMANDO "AT+CIPSEND=0,length"
        //ESP_LOGI(TAG, "COMMAND to send:%s", sendCommand);
        if(uartManager_sendReadUart(sendCommand) ){
            if(uartManager_sendReadUart(message) ){ 
                ESP_LOGI(TAG, "Envío exitoso %d!", event);
                return 1;
            }
            ESP_LOGE(TAG, "Fallo al enviar comando AT+CIPSEND=0,%d", strlen(message));
        }
    } else {
        /** Ahora solo cuando no hay servicio celular y se intenta mandar al servidor se manda a la cadena a buffer */
        ESP_LOGI(TAG, "Write in buffer:%s", message);
        spiffs_append_record(message);
        return 0;
    }
    return 0;
}
static void system_event_handler(void *handler_arg, esp_event_base_t base, int32_t event_id, void *event_data) {
    switch (event_id) {
        case IGNITION_ON:
            ignition = true;
            event = IGNITION_ON;
            /*si llegara a ver un falso de ignición hay que validar que el envó repetitivo de comandos no afecte, ponle una validación que solo se ejecute una vez hasta que haya ignicion  OFF y viseversa*/
            ESP_LOGI(TAG, "Ignition=> ENCENDIDA"); 
            sim7600_sendATCommand("AT+CPSI?");
            sim7600_sendATCommand("AT+CGNSSINFO=30");
            stop_keep_alive_timer();
            break;
        case IGNITION_OFF:
            ignition = false;
            event = IGNITION_OFF;
            ESP_LOGI(TAG, "Ignition=> APAGADA");
            sim7600_sendATCommand("AT+CGNSSINFO");
            start_keep_alive_timer();     
            break;
        case KEEP_ALIVE:
            event = KEEP_ALIVE;
            ESP_LOGI(TAG, "Evento KEEP_ALIVE: han pasado %d minutos", keep_alive_interval / 60000);
            sim7600_sendATCommand("AT+CGNSSINFO");    
            break;
    }
}
void start_uart_task(void) {
    esp_event_loop_handle_t loop = get_event_loop();
    esp_event_handler_register_with(loop, SYSTEM_EVENTS, ESP_EVENT_ANY_ID, system_event_handler, NULL);
}
