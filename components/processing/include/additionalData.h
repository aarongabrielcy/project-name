#ifndef ADDITIONALDATA_H
#define ADDITIONALDATA_H

#include <stdbool.h>  
#include <stdint.h> 

typedef struct {
    int mode;
    int glss_svs;
    int beid_svs;
    float alt;
    float pdop;
    float hdop;
    float vdop;
} additionalData_t;

extern additionalData_t add;

#endif