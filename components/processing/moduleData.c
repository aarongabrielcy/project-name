#include "moduleData.h"
#include "trackerData.h"
#include "additionalData.h"
#include "sim7600.h"
#include "utilities.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static const char *TAG = "GNSS";
bool reportFastMode;

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

    double prevCourse = tkr.course;
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

    if (fabs(tkr.course - prevCourse) >= ANGLE_THRESHOLD) {
        printf("Cambio de rumbo detectado (%.2f°), activando reporte rápido.\n", fabs(tkr.course - prevCourse));
        noChangeReported = false; 
        sim7600_sendATCommand("AT+CGNSSINFO=2");
    } else if (!noChangeReported) {  
        printf("No hay cambio de curso.\n");
        noChangeReported = true;
        sim7600_sendATCommand("AT+CGNSSINFO=30");  
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


