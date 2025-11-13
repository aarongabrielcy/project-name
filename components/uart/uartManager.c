#include "uartManager.h"
#include "sim7600.h"
#include "trackerData.h"
#include "processManager.h"
#include "pwManager.h"
#include "cellnetData.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <math.h>
#include "utilities.h"
#include "nvsManager.h"
#include "eventHandler.h"
#include "storageManager.h"
#include "cmdsManager.h"
#include "nvsData.h"
#include "gnssData.h"
#include "otaManager.h"

#define EPSILON 0.0001

static const char *TAG = "UART_MANAGER";

char latitud[20] = "+0.000000";
char longitud[20] = "+0.000000";
char last_latitud[20] ="";
char last_longitud[20] ="";
bool ignition = false;
char date_time[34];
bool redService = false;
bool configState = false;
int event = DEFAULT;
static int keep_alive_interval = 600000; // Valor en milisegundos (10 minutos)
volatile bool uart_task_enabled = true;

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
    ESP_LOGI(TAG, "Leyendo eventos del modulo SIM...");
    while (1) {
        if (!uart_task_enabled) {
            vTaskDelay(pdMS_TO_TICKS(50));  // Esperar mientras está deshabilitado
            continue;
        }
        int len = uartManager_readEvent(response, sizeof(response), 100);
        //////////// DEJAR FIJO EL TIEMPO DE REPORTE HAYA O NO HAYA IGNICIÓN ON, PERO EL EVENTO NO SE EMITE, SE GENERA EL EVENTO DEFAULT
        if (len > 0) {      
         // Limpiar la respuesta
            /*char *task_response = cleanResponse(response);
            if (task_response == NULL) {
                ESP_LOGE(TAG, "task response retornó NULL");
            }*/
            if (strstr(response, "+CGNSSINFO:") != NULL ) {
                //ESP_LOGI(TAG, "Evento GNSS detectado.");
                if (parseGPS(response) ) {
                    strcpy(latitud, formatCoordinates(gnss.lat, gnss.ns));
                    strcpy(last_latitud, latitud);
                    /*if (fabs(atof(last_latitud) - atof(latitud) ) > EPSILON) {
                        ESP_LOGI(TAG, "Las_lat=> %s", last_latitud);

                    }*/
                    strcpy(longitud, formatCoordinates(gnss.lon, gnss.ew));
                    strcpy(last_longitud, longitud);
                    /*if (fabs(atof(last_longitud) - atof(longitud) ) > EPSILON) {
                        ESP_LOGI(TAG, "Last_lon=> %s", last_longitud);
                    }*/
                    snprintf(date_time, sizeof(date_time), "%s;%s", formatDate(gnss.date), formatTime(gnss.utctime));
                } else {
                      if (last_latitud[0] != '\0') {
                            printf("last_latitud ya tiene valor: %s\n", last_latitud);
                            strcpy(latitud, last_latitud);
                            //guarda en nvs
                            nvs_save_str("last_valid_lat", last_latitud);
                            //valida que no esté vacia last nvs y si no esta asignale latitud 
                      } else if(nvs_read_str("last_valid_lat", latitud, sizeof(latitud)) != NULL) {
                            ESP_LOGI(TAG, "last_lat_NVS=%s", latitud);   
                      } 
                      if (last_longitud[0] != '\0') {
                            printf("last_longitud ya tiene valor: %s\n", last_longitud);
                            strcpy(longitud, last_longitud);
                            //guarda en nvs
                            nvs_save_str("last_valid_lon", last_longitud);
                      } else if(nvs_read_str("last_valid_lon", longitud, sizeof(longitud)) != NULL) {
                            ESP_LOGI(TAG, "last_lat_NVS=%s", longitud);   
                      }
                    if(uartManager_sendReadUart("AT+CCLK?") ) {
                        if (strchr(date_time, ';')) {
                            sscanf(date_time, "%8[^;];%8s", gnss.date, gnss.utctime);
                        } else {
                            // Asignar valores por defecto (Ya lo hago en utcFormat en utils)
                            strcpy(gnss.date, "00000000");
                            strcpy(gnss.utctime, "00:00:00");
                        }
                    }
                }  
                switch (event) {       
                    case TRACKING_RPT:
                        //ESP_LOGI(TAG, "Evento TRAKING REPORT ~~~~~~~~~~~~~~~~~~~~~~~~");
                        snprintf(message, sizeof(message), "STT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;%d;%s;%s;%.2f;%.2f;%d;%d;%d%d00000%d;00000000;1;1;0929;4.1;14.19",
                        nvs_data.device_id, date_time,cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.rxlvl_rsrp, latitud, longitud,gnss.speed, gnss.course,
                        gnss.gps_svs, gnss.fix, tkr.tkr_course, tkr.tkr_meters, ignition);
                        if(!sendToServer(message) ) {
                            ESP_LOGW(TAG, "error sending data,event:%d", TRACKING_RPT);/// para seguir usando los logs de ESP crea un enum de los TAGs para saber de que archivo viene
                            sim7600_sendATCommand("AT+CPSI?");
                        }
                        event = tkr.tkr_course || tkr.tkr_meters ? TRACKING_RPT : DEFAULT;
                        //event = DEFAULT;
                        //event = ignition ?  TRACKING_RPT : DEFAULT;
                    break;
                    case IGNITION_ON:
                        //ESP_LOGI(TAG, "Evento IGN ON ~~~~~~~~~~~~~~~~~~~~~~~~");  
                        snprintf(message, sizeof(message), "ALT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;%d;%s;%s;%.2f;%.2f;%d;%d;%d%d00000%d;00000000;%d;;",
                        nvs_data.device_id, date_time,cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.rxlvl_rsrp, latitud, longitud,gnss.speed, gnss.course,
                        gnss.gps_svs, gnss.fix, tkr.tkr_course, tkr.tkr_meters, ignition, 33);  
                        if(!sendToServer(message) ) {
                            ESP_LOGW(TAG, "error sending data, event:%d",IGNITION_ON);    
                            sim7600_sendATCommand("AT+CPSI?");
                        }
                        //event = TRACKING_RPT;
                        event = DEFAULT;
                    break;
                    case IGNITION_OFF:
                        //ESP_LOGI(TAG, "Evento IGN OFF ~~~~~~~~~~~~~~~~~~~~~~~~");
                        snprintf(message, sizeof(message), "ALT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;%d;%s;%s;%.2f;%.2f;%d;%d;%d%d00000%d;00000000;%d;;",
                        nvs_data.device_id, date_time,cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.rxlvl_rsrp, latitud, longitud,gnss.speed, gnss.course,
                        gnss.gps_svs, gnss.fix, tkr.tkr_course, tkr.tkr_meters, ignition, 34);
                        if(!sendToServer(message) ) {
                            ESP_LOGW(TAG, "error sending data,event:%d",IGNITION_OFF);    
                            sim7600_sendATCommand("AT+CPSI?");
                        }
                        event = DEFAULT;
                        //im7600_sendATCommand("AT+CGNSSINFO=0");    
                    break;
                    case INPUT1_ON:
                        //ESP_LOGI(TAG, "Evento IGN OFF ~~~~~~~~~~~~~~~~~~~~~~~~");
                        snprintf(message, sizeof(message), "ALT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;%d;%s;%s;%.2f;%.2f;%d;%d;%d%d00000%d;00000000;%d;;",
                        nvs_data.device_id, date_time,cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.rxlvl_rsrp, latitud, longitud,gnss.speed, gnss.course,
                        gnss.gps_svs, gnss.fix, tkr.tkr_course, tkr.tkr_meters, ignition, 42);
                        if(!sendToServer(message) ) {
                            ESP_LOGW(TAG, "error sending data,event:%d",IGNITION_OFF);    
                            sim7600_sendATCommand("AT+CPSI?");
                        }
                        event = DEFAULT;
                    break;
                    case KEEP_ALIVE:
                        /* Valia que el keep a live se mande solo después de la ignición */
                        //ESP_LOGI(TAG, "Evento KEEP A LIVE ~~~~~~~~~~~~~~~~~~~~~~~~");
                        snprintf(message, sizeof(message), "ALV;%s",nvs_data.device_id);    
                        if(!sendToServer(message) ) {    
                            ESP_LOGW(TAG, "error sending data,event:%d",KEEP_ALIVE);
                            sim7600_sendATCommand("AT+CPSI?");
                        }
                        event = DEFAULT;
                    break;    
                    default:
                        //ESP_LOGI(TAG, "SIN EVENTO ~~~~~~~~~~~~~~~~~~~~~~~~");
                        /** cuando se reincia en esta linea es por que el id está vacio */
                        ESP_LOGW(TAG, "<head>\n<sys_mode>%s<oper>%s<cell_id>%s<mcc>%d<mnc>%d<lac>%s<rx_lvl>%d<date_time>%s,<lat>%s,<lon>%s,<speed>%.2f,<fix>%d,<ign>%d,<id>%s,<ccid>%s,<wifi_AP_mac>%s,<Ble Mac>%s<tkr_course>%d,<tkr_meters>%d", 
                           cpsi.sys_mode, cpsi.oper_mode, cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.rxlvl_rsrp, date_time, latitud, longitud, gnss.speed, gnss.fix, ignition, nvs_data.device_id, nvs_data.sim_iccid, nvs_data.wifi_ap, 
                           nvs_data.blue_addr, tkr.tkr_course, tkr.tkr_meters); 
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
                    event = TRACKING_RPT;
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
                if(redService) {
                    //ESP_LOGI(TAG, "Parseando CPSI EXITOSO!");
                    ESP_LOGI(TAG, "sys mode:%s, operador: %s, MCC:%d, MNC:%d, LAC:%s, CellID:%s, RXLVL:%d",
                                cpsi.sys_mode, cpsi.oper_mode, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.cell_id, cpsi.rxlvl_rsrp);    

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

            } else if(strstr(response, "+HTTPACTION:") != NULL) {
                char *sizeBinary =  cleanATResponse(response);
                if (sizeBinary != NULL) {
                    ESP_LOGI(TAG, "Size Binary:%s", sizeBinary);
                    initUpdate(sizeBinary);
                }
            } else if(strstr(response, "+HTTPREAD:") != NULL) {
                ESP_LOGI(TAG, "RESPONSE HTTP:%s", response);
                
            } else if(strstr(response, "PB DONE") != NULL) {
                ESP_LOGI(TAG, "REACTIVANDO TRAKER REPORT: %s", response);
                sim7600_sendATCommand("AT+CGPS=1");
                vTaskDelay(pdMS_TO_TICKS(1000));
                //sim7600_sendATCommand("AT+CGNSSINFO=30");// VALIDA EL ESTADO DE LA IGNICION
            } else { ESP_LOGE(TAG, "RD URT: %s", response); }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        set_gnss_led_state(gnss.fix);
    }
}
int uartManager_readBinary(uint8_t *buffer, int max_length, int timeout_ms) {
    uart_task_enabled = false;
    return uart_read_bytes(UART_SIM, buffer, max_length, pdMS_TO_TICKS(timeout_ms));
}
int uartManager_readEvent(char *buffer, int max_length, int timeout_ms) {
    int len = uart_read_bytes(UART_SIM, (uint8_t *)buffer, max_length - 1, pdMS_TO_TICKS(timeout_ms));
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
    memset(response, 0, sizeof(response));      // Limpiar buffer de recepción
    uart_flush(UART_SIM);                       // Limpiar buffer UART
    ESP_LOGI(TAG, "Enviando comando: %s", command);
    uart_write_bytes(UART_SIM, command, strlen(command));
    uart_write_bytes(UART_SIM, "\r\n", 2);
    // Leer la respuesta del SIM7600
    int len = uart_read_bytes(UART_SIM, (uint8_t *)response, BUF_SIZE - 1, pdMS_TO_TICKS(500) );
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
                    free(cleanSend);
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
        } else if(strstr(cleanedResponse, "ICCID") != NULL) {
            if (nvs_read_str("sim_id", nvs_data.sim_iccid, sizeof(nvs_data.sim_iccid)) != NULL) {
                ESP_LOGI(TAG, "Longitud real: %d", (int)strlen(nvs_data.sim_iccid));
                ESP_LOGI(TAG,"SIM ID:%s\n", nvs_data.sim_iccid);
                if (strlen(nvs_data.sim_iccid) != 19) {
                    ESP_LOGI(TAG,"SIMID Incorrecto: %s\n", nvs_data.sim_iccid);
                    nvs_delete_key("sim_id");
                    return false; 
                    ///Reiniciar dispositivo
                } else if(strlen(nvs_data.sim_iccid) == 19) { 
                    ESP_LOGI(TAG, "ICCID correcto: %s", nvs_data.sim_iccid);
                    return true;
                }    
            } else {
                char *new_ccid = cleanATResponse(cleanedResponse);
                ESP_LOGI(TAG, "SIM parseado: %s", new_ccid);
                ESP_LOGI(TAG, "Longitud: %d", strlen(new_ccid));
                if (strlen(new_ccid) == 19) {
                    ESP_LOGI(TAG, "new_ccid=>%s", new_ccid);
                    nvs_save_str("sim_id", new_ccid);
                    if (nvs_read_str("sim_id", nvs_data.sim_iccid, sizeof(nvs_data.sim_iccid)) != NULL) {

                        ESP_LOGI(TAG, "ICCID#>%s", nvs_data.sim_iccid);
                        return true;    
                    }    
                }
            }
            return false; 
        } else if(strstr(cleanedResponse, "SIMEI") != NULL) {  
            if (nvs_read_str("dev_simei", nvs_data.imei_module, sizeof(nvs_data.imei_module)) != NULL) {
                ESP_LOGI(TAG, "Longitud real: %d", (int)strlen(nvs_data.imei_module));
                printf("Dev SIMEI:%s\n", nvs_data.imei_module);
                if (strlen(nvs_data.imei_module) != 15) {
                    printf("imei Incorrecto: %s\n", nvs_data.imei_module);
                    nvs_delete_key("dev_simei");
                    return false;
                    //REINICIAR EL DISPOSITIVO PARA QUE VUELVA A INTENTAR LEER EL IMEI 
                } else if(strlen(nvs_data.imei_module) == 15) { 
                    ESP_LOGI(TAG, "IMEI correcto: %s", nvs_data.imei_module);
                    
                    if( nvs_read_str("device_id", nvs_data.device_id, sizeof(nvs_data.device_id)) != NULL) {
                        ESP_LOGI(TAG, "DEVICE_ID:%s", nvs_data.device_id);
                        return true;
                    }
                    nvs_save_str("device_id", formatDevID(nvs_data.imei_module) );
                     if(nvs_read_str("device_id", nvs_data.device_id, sizeof(nvs_data.device_id)) != NULL ) {
                        ESP_LOGI(TAG, "DEVICE_ID#%s", nvs_data.device_id); 
                        return true;
                     }
                }    
            } else {
                char *new_simei = cleanATResponse(cleanedResponse);
                ESP_LOGI(TAG, "IMEI parseado: %s", new_simei);
                vTaskDelay(pdMS_TO_TICKS(5));
                ESP_LOGI(TAG, "Longitud: %d", strlen(new_simei));
                if (strlen(new_simei) == 15) {
                    ESP_LOGI(TAG, "new simei=>%s", new_simei);
                    nvs_save_str("dev_simei", new_simei);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    nvs_save_str("device_id", formatDevID(new_simei) );
                    vTaskDelay(pdMS_TO_TICKS(100));
                    if(nvs_read_str("device_id", nvs_data.device_id, sizeof(nvs_data.device_id)) != NULL){
                        ESP_LOGI(TAG, "DEVICE_ID=%s", nvs_data.device_id); 
                        return true;  
                    }
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
                    return true;
                } else {
                    ESP_LOGE(TAG, "getFormatUTC retornó NULL");
                    return false;
                }
            }
            return false;
        } else if (strstr(cleanedResponse,"CIPOPEN") != NULL) {
            ESP_LOGI(TAG, "READ CIPOPEN");
            return true;
        }  else if(strstr(cleanedResponse, "+CIPERROR:") != NULL) {
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
                event = TRACKING_RPT;
            }
            return false;
        } else if (strstr(cleanedResponse, "OK") != NULL ) { ////////// si validas solo el comando "AT" busca mejor "AT,OK"
            ESP_LOGI(TAG, "Response ends with 'OK', returning true"); 
            //sim7600_sendATCommand("AT+CPIN?");
            return true;
        }
    free(cleanedResponse);
    } else {
        ESP_LOGW(TAG, "Respuesta de comando No Procesado...");
        return false;
    }
    return false;
}
void uartManager_start() {
    xTaskCreate(uart_task, "uart_task", 8192, NULL, 5, NULL);
}
int sendToServer(char * message) {
    if(redService) { // al inicio de la configuración validar el estado cd CPSI
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
           uart_task_enabled = true;
            //sim7600_sendATCommand("AT+CPSI?");
            sim7600_sendATCommand("AT+CGNSSINFO=3");
            start_tracking_report_timer();
            stop_keep_alive_timer();
            //sim7600_sendATCommand("AT+CGNSSINFO"); // CREAR UN TIMER DE 1 MINUTO EN RELANTI
        break;
        case IGNITION_OFF:
            ignition = false;
            event = IGNITION_OFF;
            ESP_LOGI(TAG, "Ignition=> APAGADA");
            sim7600_sendATCommand("AT+CGNSSINFO=30");
            stop_tracking_report_timer();
            start_keep_alive_timer();
            //sim7600_sendATCommand("AT+CGNSSINFO")// ELIMINAR EL TIMER DE 1 MINUTO PERO EJECUTALO ANTES 1 VEZ
        break;
        case INPUT1_ON:
            //event = INPUT1_ON;
            /*si llegara a ver un falso de ignición hay que validar que el envó repetitivo de comandos no afecte, ponle una validación que solo se ejecute una vez hasta que haya ignicion  OFF y viseversa*/
            ESP_LOGI(TAG, "INPUT_1=> ENCENDIDA"); 
            //sim7600_sendATCommand("AT+CGNSSINFO");
        break;
        case INPUT1_OFF:
            //event = INPUT1_OFF;
            ESP_LOGI(TAG, "INPUT_1=> APAGADA");
        break;
        case KEEP_ALIVE:
            event = KEEP_ALIVE;
            ESP_LOGI(TAG, "Evento KEEP_ALIVE: han pasado %d minutos", keep_alive_interval / 60000);
            //sim7600_sendATCommand("AT+CGNSSINFO");    
        break;
        case TRACKING_RPT:
            event = TRACKING_RPT;
            ESP_LOGI(TAG, "Generando TRACKING_RPT");
            //sim7600_sendATCommand("AT+CGNSSINFO");  // o la acción que desees
        break;
    }
}
void start_event_handler_uart(void) {
    esp_event_loop_handle_t loop = get_event_loop();
    esp_event_handler_register_with(loop, SYSTEM_EVENTS, ESP_EVENT_ANY_ID, system_event_handler, NULL);
}
