#ifndef TRACKERDATA_H
#define TRACKERDATA_H

#include <stdbool.h>  // Para usar bool
#include <stdint.h>   // Para usar tipos estándar (int32_t, etc.)

typedef struct {
    char header[4];      // "STT"
    char imei[20];       // "2049830928" /////// no es  necesario que esté aqui guarda en la memoria no volatil del esp el imei y cuando se reinicie leelo de nuevo
    char rep_map[16];    // "3FFFFF"
    int model;           // 99
    char sw_ver[8];      // "1.0.1"
    int msg_type;        // 1
    char date[10];       // "00000000"
    char utctime[10];       // "00:00:00"
    char cell_id[16];    // "00000000"
    int mcc;             // 0
    int mnc;             // 0
    char lac_tac[8];     // "FFFF"
    int rxlvl_rsrp;      // 999
    double lat;        // "+00.000000"
    char ns;
    double lon;        // "+/-00.000000"
    char ew;        
    float speed;         // 0.00
    float course;        // 0.00
    int gps_svs;         // 0
    int fix;             // 0
    int in1_state;    // "00000100"
    int in_ig_st;
    int out1_state;   // "00001000"
    int mode;            // 0
    int stt_rpt_type;    // 0
    int msg_num;         // 0
    float bck_volt;      // 0.00
    float power_volt;    // 0.00
} trackerData_t;

// Declarar una variable global para almacenar los datos
extern trackerData_t tkr;

#endif
