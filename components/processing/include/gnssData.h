#ifndef GNSSDATA_H
#define GNSSDATA_H

#include <stdbool.h>  
#include <stdint.h> 

typedef struct {
    int mode;
    int gps_svs;         // 0
    int glss_svs;
    int beid_svs;
    double lat;        // "+00.000000"
    char ns;
    double lon;        // "+/-00.000000"
    char ew;        
    char date[10];       // "00000000"
    char utctime[10];       // "00:00:00"
    float alt;
    float speed;         // 0.00
    float course;        // 0.00
    float pdop;
    float hdop;
    float vdop;
    int fix;             // 0
} gnssData_t;

extern gnssData_t gnss;

#endif