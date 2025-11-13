#ifndef TRACKERDATA_H
#define TRACKERDATA_H

#include <stdbool.h>  // Para usar bool
#include <stdint.h>   // Para usar tipos estándar (int32_t, etc.)

typedef struct {
    char rep_map[16];    // "3FFFFF"
    int model;           // 99
    char sw_ver[8];      // "1.0.1"
    int msg_type;        // 1
    int tkr_course;
    int tkr_meters;
    /*int in1_state;    // "00000100"
    int in_ig_st;
    int out1_state;   // "00001000" */
    int mode;            // 0
    int stt_rpt_type;    // 0
    int msg_num;         // 0
    float bck_volt;      // 0.00
    float power_volt;    // 0.00
} trackerData_t;

// Declarar una variable global para almacenar los datos
extern trackerData_t tkr;

#endif
