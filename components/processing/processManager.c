#include "processManager.h"
#include "trackerData.h"
#include "gnssData.h"
#include "cellnetData.h"
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
        gnss = (gnssData_t){};
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

    gnss.mode = atoi(tokens[0]);
    gnss.gps_svs = atoi(tokens[1]);
    gnss.glss_svs = atoi(tokens[2]);
    gnss.beid_svs = atoi(tokens[3]);
    gnss.lat = atof(tokens[4]);
    gnss.ns = tokens[5][0];
    gnss.lon = atof(tokens[6]);
    gnss.ew = tokens[7][0];
    strncpy(gnss.date, tokens[8], sizeof(gnss.date) - 1);
    strncpy(gnss.utctime, tokens[9], sizeof(gnss.utctime) - 1);
    gnss.alt = atof(tokens[10]);
    gnss.speed = atof(tokens[11]) * 1.85;
    // Manejo de course vacío
    if (index == 15) {  // No hay curso, ajustar los índices
        gnss.course = 0.0;
        gnss.pdop = atof(tokens[12]);  
        gnss.hdop = atof(tokens[13]);  
        gnss.vdop = atof(tokens[14]);  
    } else {  // Hay curso, índices normales
        gnss.course = atof(tokens[12]);
        gnss.pdop = atof(tokens[13]);
        gnss.hdop = atof(tokens[14]);
        gnss.vdop = atof(tokens[15]);
    }
    gnss.fix = 1;
    float difference = fabs(gnss.course - prevCourse);
    if (difference >= ANGLE_THRESHOLD) {
        printf("Cambio de rumbo detectado (%.2f°), activando reporte rápido.\n", difference);
        noChangeReported = false;
        prevCourse = gnss.course; 
        if(sim7600_sendReadCommand("AT+CGNSSINFO=3")){ //volver dinamico
            tkr.tkr_course = 1;
            printf("tiempo de reporte A 3 segundoS!");
        }
    } else if (!noChangeReported) {  
        printf("No hay cambio de curso.\n");
        noChangeReported = true;
        if(sim7600_sendReadCommand("AT+CGNSSINFO=30")) { //Volver dinamico
            tkr.tkr_course = 0;
            printf("tiempo de reporte 30 segundo!");
        }
    }
    printf("\n--- Datos GNSS Parseados ---\n");
    printf("Mode: %d\n", gnss.mode);
    printf("GPS SVs: %d\n", gnss.gps_svs);
    printf("GLONASS SVs: %d\n", gnss.glss_svs);
    printf("Latitud: %.6f %c\n", gnss.lat, gnss.ns);
    printf("Longitud: %.6f %c\n", gnss.lon, gnss.ew);
    printf("Fecha: %s\n", gnss.date);
    printf("Hora UTC: %s\n", gnss.utctime);
    printf("Altitud: %.2f m\n", gnss.alt);
    printf("Velocidad: %.2f km/h\n", gnss.speed);
    printf("Curso: %.2f°\n", gnss.course);
    printf("PDOP: %.2f\n", gnss.pdop);
    printf("HDOP: %.2f\n", gnss.hdop);
    printf("VDOP: %.2f\n", gnss.vdop);
    printf("Fix: %d\n", gnss.fix);
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
    strncpy(cpsi.sys_mode, values[0], sizeof(cpsi.sys_mode) - 1);
    cpsi.mcc = atoi(values[2]);
    cpsi.mnc = atoi(values[2] + 4);
    strncpy(cpsi.lac_tac, removeHexPrefix(values[3]), sizeof(cpsi.lac_tac) - 1);
    strncpy(cpsi.cell_id, values[4], sizeof(cpsi.cell_id) - 1);
    cpsi.rxlvl_rsrp = atoi(values[6]) /10;

    /*ESP_LOGI(TAG, "GSM Parseado: MCC:%d, MNC:%d, LAC:%s, CellID:%s, RXLVL:%d",
             cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.cell_id, cpsi.rxlvl_rsrp);*/
}
/*void parseLTE(char *tokens) {
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
    strncpy(cpsi.sys_mode, values[0], sizeof(cpsi.sys_mode) - 1);
    strncpy(cpsi.oper_mode, values[1], sizeof(cpsi.sys_mode) - 1);
    cpsi.mcc = atoi(values[2]);
    cpsi.mnc = atoi(values[2] + 4);
    strncpy(cpsi.lac_tac, removeHexPrefix(values[3]), sizeof(cpsi.lac_tac) - 1);
    strncpy(cpsi.cell_id, values[4], sizeof(cpsi.cell_id) - 1);
    cpsi.rxlvl_rsrp = atoi(values[11]);
}*/
void parseLTE(char *tokens) {
    char *values[15] = {NULL};  
    int count = 0;

    // Separar tokens
    char *token = strtok(tokens, ",");
    while (token != NULL && count < 15) {
        values[count++] = token;
        token = strtok(NULL, ",");
    }

    if (count < 14) {
        ESP_LOGE(TAG, "Error: Datos insuficientes en LTE.");
        return;
    }

    strncpy(cpsi.sys_mode, values[0], sizeof(cpsi.sys_mode) - 1);
    cpsi.sys_mode[sizeof(cpsi.sys_mode) - 1] = '\0';

    strncpy(cpsi.oper_mode, values[1], sizeof(cpsi.oper_mode) - 1);
    cpsi.oper_mode[sizeof(cpsi.oper_mode) - 1] = '\0';

    cpsi.mcc = atoi(values[2]);
    cpsi.mnc = atoi(values[2] + 4); // Según tu formato MCC-MNC, sigues leyendo bien

    const char *lac_tac_clean = removeHexPrefix(values[3]);
    strncpy(cpsi.lac_tac, lac_tac_clean, sizeof(cpsi.lac_tac) - 1);
    cpsi.lac_tac[sizeof(cpsi.lac_tac) - 1] = '\0';

    strncpy(cpsi.cell_id, values[4], sizeof(cpsi.cell_id) - 1);
    cpsi.cell_id[sizeof(cpsi.cell_id) - 1] = '\0';

    cpsi.rxlvl_rsrp = atoi(values[11]) /10;
    /*ESP_LOGI(TAG, "LTE Parseado: MCC:%d, MNC:%d, TAC:%s, CellID:%s, RSRP:%d",
             cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.cell_id, cpsi.rxlvl_rsrp);*/
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

    cpsi.mcc = atoi(values[2]);
    cpsi.mnc = atoi(values[2] + 4);
    strncpy(cpsi.lac_tac, removeHexPrefix(values[3]), sizeof(cpsi.lac_tac) - 1);
    strncpy(cpsi.cell_id, values[4], sizeof(cpsi.cell_id) - 1);
    cpsi.rxlvl_rsrp = atoi(values[12] ) /10; 

    /*ESP_LOGI(TAG, "WCDMA Parseado: MCC:%d, MNC:%d, LAC:%s, CellID:%s, RXLVL:%d",
             cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.cell_id, cpsi.rxlvl_rsrp);*/
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

    cpsi.mcc = atoi(values[2]);
    cpsi.mnc = atoi(values[2] + 4);
    cpsi.rxlvl_rsrp = atoi(values[6]) /10;

    /*ESP_LOGI(TAG, "CDMA Parseado: MCC:%d, MNC:%d, RXLVL:%d",
             cpsi.mcc, cpsi.mnc, cpsi.rxlvl_rsrp);*/
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

    cpsi.mcc = atoi(values[2]);
    cpsi.mnc = atoi(values[2] + 4);
    cpsi.rxlvl_rsrp = atoi(values[5]) /10;

    /*ESP_LOGI(TAG, "EVDO Parseado: MCC:%d, MNC:%d, RXLVL:%d",
             cpsi.mcc, cpsi.mnc, cpsi.rxlvl_rsrp);*/
}