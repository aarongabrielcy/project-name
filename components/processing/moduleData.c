#include "moduleData.h"
#include "trackerData.h"
#include "additionalData.h"
#include "serviceInfo.h"
#include "sim7600.h"
#include "utilities.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static const char *TAG = "PROCESS";
bool reportFastMode;
float prevCourse = -1.0;
void parseGPS(char *response) {
    char *cleanResponse = cleanData(response, "CGNSSINFO");
    if (cleanResponse == NULL) {
        printf("Error: No se pudo limpiar la respuesta GNSS.\n");
        return;
    }
    ESP_LOGI(TAG, "Clean CGNSSINFO => %s\n", cleanResponse);
    if (strstr(cleanResponse, ",,,,,,,,,,,,,,,") != NULL) {
        printf("No hay fix GNSS. Asignando valores por defecto.\n");
        //aqui estoy asignando por defecto.
        tkr = (trackerData_t){};
        add = (additionalData_t){};
        return;
    }
    // Parsear los datos si la respuesta no es vacía
    char *tokens[16] = {NULL};
    int index = 0;
    char *token = strtok(cleanResponse, ",");

    while (token != NULL && index < 16) {
        tokens[index++] = token;
        token = strtok(NULL, ",");
    }

    if (index < 15 || index > 16) {
        printf("Datos insuficientes en GNSS. Manteniendo valores actuales.\n");
        return;
    }

    static bool noChangeReported = true;

    tkr.mode = atoi(tokens[0]);
    tkr.gps_svs = atoi(tokens[1]);
    add.glss_svs = atoi(tokens[2]);
    add.beid_svs = atoi(tokens[3]);
    tkr.lat = atof(tokens[4]);
    tkr.ns = tokens[5][0];
    tkr.lon = atof(tokens[6]);
    tkr.ew = tokens[7][0];
    strncpy(tkr.date, tokens[8], sizeof(tkr.date) - 1);
    strncpy(tkr.utctime, tokens[9], sizeof(tkr.utctime) - 1);
    add.alt = atof(tokens[10]);
    tkr.speed = atof(tokens[11]) * 1.85;
    // Manejo de course vacío
    if (index == 15) {  // No hay curso, ajustar los índices
        tkr.course = 0.0;
        add.pdop = atof(tokens[12]);  
        add.hdop = atof(tokens[13]);  
        add.vdop = atof(tokens[14]);  
    } else {  // Hay curso, índices normales
        tkr.course = atof(tokens[12]);
        add.pdop = atof(tokens[13]);
        add.hdop = atof(tokens[14]);
        add.vdop = atof(tokens[15]);
    }
    tkr.fix = 1;
    float difference = fabs(tkr.course - prevCourse);
    if (difference >= ANGLE_THRESHOLD) {
        printf("Cambio de rumbo detectado (%.2f°), activando reporte rápido.\n", difference);
        noChangeReported = false;
        prevCourse = tkr.course; 
        if(sim7600_sendReadCommand("AT+CGNSSINFO=3")){
            tkr.tkr_course = 1;
            printf("tiempo de reporte A 2 segundoS!");
        }
    } else if (!noChangeReported) {  
        printf("No hay cambio de curso.\n");
        noChangeReported = true;
        if(sim7600_sendReadCommand("AT+CGNSSINFO=20")){
            tkr.tkr_course = 0;
            printf("tiempo de reporte 20 segundo!");
        }
    }
    printf("\n--- Datos GNSS Parseados ---\n");
    printf("Mode: %d\n", tkr.mode);
    printf("GPS SVs: %d\n", tkr.gps_svs);
    printf("GLONASS SVs: %d\n", add.glss_svs);
    printf("Latitud: %.6f %c\n", tkr.lat, tkr.ns);
    printf("Longitud: %.6f %c\n", tkr.lon, tkr.ew);
    printf("Fecha: %s\n", tkr.date);
    printf("Hora UTC: %s\n", tkr.utctime);
    printf("Altitud: %.2f m\n", add.alt);
    printf("Velocidad: %.2f km/h\n", tkr.speed);
    printf("Curso: %.2f°\n", tkr.course);
    printf("PDOP: %.2f\n", add.pdop);
    printf("HDOP: %.2f\n", add.hdop);
    printf("VDOP: %.2f\n", add.vdop);
    printf("Fix: %d\n", tkr.fix);
    printf("---------------------------\n");
}
bool parsePSI(char *response) {
    char *cleanResponse = cleanData(response, "CPSI");

    if (cleanResponse == NULL) {
        ESP_LOGE(TAG, "Error: No se pudo limpiar la respuesta CPSI.");
        return false;
    }
    // Verificar con qué cabecera comienza la cadena
    if (strncmp(cleanResponse, "GSM", 3) == 0) {
        parseGSM(cleanResponse);
    } else if (strncmp(cleanResponse, "LTE", 3) == 0) {
        parseLTE(cleanResponse);
    } else if (strncmp(cleanResponse, "WCDMA", 5) == 0) {
        parseWCDMA(cleanResponse);
    } else if (strncmp(cleanResponse, "CDMA", 4) == 0) {
        parseCDMA(cleanResponse);
    } else if (strncmp(cleanResponse, "EVDO", 4) == 0) {
        parseEVDO(cleanResponse);
    }else if (strncmp(cleanResponse, "NO SERVICE", 10) == 0) {
        ESP_LOGW(TAG, "Red celular: %s", cleanResponse);
        return false;
    }else {
        ESP_LOGW(TAG, "Formato de CPSI inválido: %s", cleanResponse);
        return false;
    }
    return true;
}
void parseGSM(char *tokens) {
    char *values[10] = {NULL};  
    int count = 0;
    
    // Separar la cadena en tokens
    char *token = strtok(tokens, ",");
    while (token != NULL && count < 10) {
        values[count++] = token;
        token = strtok(NULL, ",");
    }

    if (count < 9) {
        ESP_LOGE(TAG, "Error: Datos insuficientes en GSM.");
        return;
    }
    strncpy(serInf.sys_mode, values[0], sizeof(serInf.sys_mode) - 1);
    serInf.mcc = atoi(values[2]);
    serInf.mnc = atoi(values[2] + 4);
    strncpy(serInf.lac_tac, removeHexPrefix(values[3]), sizeof(serInf.lac_tac) - 1);
    strncpy(serInf.cell_id, values[4], sizeof(serInf.cell_id) - 1);
    serInf.rxlvl_rsrp = atoi(values[6]);

    /*ESP_LOGI(TAG, "GSM Parseado: MCC:%d, MNC:%d, LAC:%s, CellID:%s, RXLVL:%d",
             serInf.mcc, serInf.mnc, serInf.lac_tac, serInf.cell_id, serInf.rxlvl_rsrp);*/
}

void parseLTE(char *tokens) {
    char *values[15] = {NULL};  
    int count = 0;

    char *token = strtok(tokens, ",");
    while (token != NULL && count < 15) {
        values[count++] = token;
        token = strtok(NULL, ",");
    }

    if (count < 14) {
        ESP_LOGE(TAG, "Error: Datos insuficientes en LTE.");
        return;
    }
    strncpy(serInf.sys_mode, values[0], sizeof(serInf.sys_mode) - 1);
    strncpy(serInf.oper_mode, values[1], sizeof(serInf.sys_mode) - 1);
    serInf.mcc = atoi(values[2]);
    serInf.mnc = atoi(values[2] + 4);
    strncpy(serInf.lac_tac, removeHexPrefix(values[3]), sizeof(serInf.lac_tac) - 1);
    strncpy(serInf.cell_id, values[4], sizeof(serInf.cell_id) - 1);
    serInf.rxlvl_rsrp = atoi(values[11]);

    /*ESP_LOGI(TAG, "LTE Parseado: MCC:%d, MNC:%d, TAC:%s, CellID:%s, RSRP:%d",
             serInf.mcc, serInf.mnc, serInf.lac_tac, serInf.cell_id, serInf.rxlvl_rsrp);*/
}

void parseWCDMA(char *tokens) {
    char *values[15] = {NULL};  
    int count = 0;

    char *token = strtok(tokens, ",");
    while (token != NULL && count < 15) {
        values[count++] = token;
        token = strtok(NULL, ",");
    }

    if (count < 14) {
        ESP_LOGE(TAG, "Error: Datos insuficientes en WCDMA.");
        return;
    }

    serInf.mcc = atoi(values[2]);
    serInf.mnc = atoi(values[2] + 4);
    strncpy(serInf.lac_tac, removeHexPrefix(values[3]), sizeof(serInf.lac_tac) - 1);
    strncpy(serInf.cell_id, values[4], sizeof(serInf.cell_id) - 1);
    serInf.rxlvl_rsrp = atoi(values[12]);

    /*ESP_LOGI(TAG, "WCDMA Parseado: MCC:%d, MNC:%d, LAC:%s, CellID:%s, RXLVL:%d",
             serInf.mcc, serInf.mnc, serInf.lac_tac, serInf.cell_id, serInf.rxlvl_rsrp);*/
}

void parseCDMA(char *tokens) {
    char *values[15] = {NULL};  
    int count = 0;

    char *token = strtok(tokens, ",");
    while (token != NULL && count < 15) {
        values[count++] = token;
        token = strtok(NULL, ",");
    }

    if (count < 14) {
        ESP_LOGE(TAG, "Error: Datos insuficientes en CDMA.");
        return;
    }

    serInf.mcc = atoi(values[2]);
    serInf.mnc = atoi(values[2] + 4);
    serInf.rxlvl_rsrp = atoi(values[6]);

    /*ESP_LOGI(TAG, "CDMA Parseado: MCC:%d, MNC:%d, RXLVL:%d",
             serInf.mcc, serInf.mnc, serInf.rxlvl_rsrp);*/
}

void parseEVDO(char *tokens) {
    char *values[10] = {NULL};  
    int count = 0;

    char *token = strtok(tokens, ",");
    while (token != NULL && count < 10) {
        values[count++] = token;
        token = strtok(NULL, ",");
    }

    if (count < 10) {
        ESP_LOGE(TAG, "Error: Datos insuficientes en EVDO.");
        return;
    }

    serInf.mcc = atoi(values[2]);
    serInf.mnc = atoi(values[2] + 4);
    serInf.rxlvl_rsrp = atoi(values[5]);

    /*ESP_LOGI(TAG, "EVDO Parseado: MCC:%d, MNC:%d, RXLVL:%d",
             serInf.mcc, serInf.mnc, serInf.rxlvl_rsrp);*/
}