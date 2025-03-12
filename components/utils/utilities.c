#include "utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

//static const char *TAG = "UTILS";

char *cleanData(char *response, const char *command) {
    char *prefix = strstr(response, "+");
    if (!prefix) return response;

    prefix = strchr(prefix, ':');
    if (!prefix) return response;

    return prefix + 2;  // Saltar ": " para obtener la data limpia
}
char* cleanATResponse(const char *input) {
    static char cleaned[100];  // Buffer estático para la respuesta limpia
    const char *start = strchr(input, ':');  // Encontrar el primer ':'   
    if (start) {
        start += 2;  // Saltar ": " (dos caracteres)
        const char *end = strstr(start, ",OK");  // Buscar ",OK" para eliminarlo
        if (!end) {
            end = input + strlen(input);  // Si no hay ",OK", tomar hasta el final
        }
        strncpy(cleaned, start, end - start);  // Copiar la parte limpia
        cleaned[end - start] = '\0';  // Agregar terminador de cadena   
        // **Eliminar comillas si están al inicio y al final**
        size_t len = strlen(cleaned);
        if (len > 1 && cleaned[0] == '"' && cleaned[len - 1] == '"') {
            memmove(cleaned, cleaned + 1, len - 2); // Mover el contenido a la izquierda
            cleaned[len - 2] = '\0';  // Ajustar terminador de cadena
        }
    } else {
        strcpy(cleaned, "");  // Devolver cadena vacía si no hay formato válido
    }
    return cleaned;
}
char* cleanResponse(const char *response) {
    char *cleanResponse = (char *)malloc(strlen(response) + 1); // Reservar memoria
    if (cleanResponse == NULL) {
        return NULL; // Manejo de error en caso de fallo en la asignación de memoria
    }
    int j = 0;
    for (int i = 0; response[i] != '\0'; i++) {
        if (response[i] == '\r' || response[i] == '\n') {
            // Si el siguiente carácter también es un salto de línea, ignorarlo
            if (response[i + 1] == '\r' || response[i + 1] == '\n') {
                continue;
            }
            // Si no es el final de la línea y no hay una coma previa, agregar una coma ","
            if (j > 0 && response[i + 1] != '\0' && response[i + 1] != '\r' && response[i + 1] != '\n') {
                cleanResponse[j++] = ',';
            }
        } else {
            cleanResponse[j++] = response[i];
        }
    }
    cleanResponse[j] = '\0';  // Asegurar terminación de cadena
    return cleanResponse;  // Retornar la nueva cadena procesada
}
char* formatCoordinates(double coord, char direction) { 
    static char buffer[12]; // Suficiente para almacenar el número con signo

    // Separar grados y minutos
    int degrees = (int)(coord / 100);
    double minutes = coord - (degrees * 100);
    double decimalDegrees = degrees + (minutes / 60.0);

    // Aplicar signo según la dirección
    if (direction == 'S' || direction == 'W') {
        decimalDegrees = -decimalDegrees;
    }

    // Formatear el número como cadena con signo explícito
    snprintf(buffer, sizeof(buffer), "%+.6f", decimalDegrees);

    return buffer; // Retornar el puntero al buffer estático
}

char *formatDate(const char *date) {
    // Formato de entrada: DDMMYY (e.g., "090325")
    // Formato de salida: YYYYMMDD (e.g., "20250309")
    char *output = (char *)malloc(9 * sizeof(char)); // Reservar memoria para YYYYMMDD + '\0'
    if (output != NULL) {
        snprintf(output, 9, "20%c%c%c%c%c%c", date[4], date[5], date[2], date[3], date[0], date[1]);
    }
    return output;
}

char *formatTime(const char *utcTime) {
    // Formato de entrada: HHMMSS.s (e.g., "141640.0")
    // Formato de salida: HH:MM:SS (e.g., "14:16:40")
    char *output = (char *)malloc(9 * sizeof(char)); // Reservar memoria para HH:MM:SS + '\0'
    if (output != NULL) {
        snprintf(output, 9, "%c%c:%c%c:%c%c", utcTime[0], utcTime[1], utcTime[2], utcTime[3], utcTime[4], utcTime[5]);
    }
    return output;
}

char* getFormatUTC(const char* input) {
    static char output[34];  // Suficiente para el formato "YYYYMMDD;HH:MM:SS"
    int year, month, day, hour, minute, second, tz_offset;
    // Registro de entrada
    //ESP_LOGI(TAG, "data formatUTC => %s", input);
    // Extraer los valores, asegurando que tz_offset pueda manejar valores negativos
    if (sscanf(input, "%2d/%2d/%2d,%2d:%2d:%2d%3d", 
               &year, &month, &day, &hour, &minute, &second, &tz_offset) != 7) {
        strcpy(output, "00000000;00:00:00");  // Valor por defecto en caso de error
        return output;
    }
    // Ajustar año al formato completo (asume 2000+)
    year += 2000;
    // **Validar valores antes de continuar**
    if (year < 2000 || year > 2099) year = 2000;
    if (month < 1 || month > 12) month = 1;
    if (day < 1 || day > 31) day = 1;
    if (hour < 0 || hour > 23) hour = 0;
    if (minute < 0 || minute > 59) minute = 0;
    if (second < 0 || second > 59) second = 0;   
    // **Validar tz_offset para evitar desbordamiento**
    if (tz_offset < -96 || tz_offset > 96) tz_offset = 0;
    // **Convertir a UTC**
    int offsetMinutes = tz_offset * 15;
    hour -= offsetMinutes / 60;
    minute -= offsetMinutes % 60;
    // **Normalizar hora y fecha**
    while (minute >= 60) {
        hour += 1;
        minute -= 60;
    }
    while (minute < 0) {
        hour -= 1;
        minute += 60;
    }
    while (hour >= 24) {
        hour -= 24;
        day += 1;
    }
    while (hour < 0) {
        hour += 24;
        day -= 1;
    }
    // **Formatear la salida de manera segura**
    snprintf(output, sizeof(output), "%04d%02d%02d;%02d:%02d:%02d", 
             year, month, day, hour, minute, second);
    return output;
}
const char *formatDevID(const char *input) {
    static char output[11]; // Buffer estático para almacenar el resultado (10 caracteres + '\0')
    size_t len = strlen(input);
    if (len >= 10) {
        strncpy(output, input + (len - 10), 10); // Copiar los últimos 10 caracteres
    } else {
        strncpy(output, input, len); // Copiar el string completo si es menor a 10 caracteres
    }
    output[10] = '\0'; // Asegurar terminador nulo
    return output;
}
char* removeHexPrefix(const char *hexValue) {
    if (hexValue == NULL) {
        return NULL;
    }

    // Verificar si la cadena empieza con "0x" o "0X"
    if ((hexValue[0] == '0' && (hexValue[1] == 'x' || hexValue[1] == 'X'))) {
        return strdup(hexValue + 2);  // Crear una copia de la cadena sin "0x"
    }

    return strdup(hexValue);  // Retornar copia de la cadena original
}