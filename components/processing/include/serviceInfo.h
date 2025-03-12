#ifndef SERVICEINFO_H
#define SERVICEINFO_H

#include <stdbool.h>  
#include <stdint.h> 

typedef struct {
    char sys_mode[20];
    char oper_mode[20];
    int mcc;
    int mnc;
    char lac_tac[10];
    char cell_id[16];
    int rxlvl_rsrp;
} serviceInfo_t;

extern serviceInfo_t serInf;

#endif