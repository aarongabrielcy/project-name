#include "trackerData.h"
#include <string.h>

// Inicializar los valores por defecto
trackerData_t tkr = {
    .rep_map = "3FFFFF",
    .model = 99,
    .sw_ver = "1.0.1",
    .msg_type = 1,
    .date = "00000000",
    .utctime = "00:00:00",
    .lat = 0.0,
    .ns = 'N',
    .lon = 0.0,
    .ew = 'W',
    .speed = 0.00,
    .course = 0.00,
    .gps_svs = 0,
    .fix = 0,
    .in_ig_st = 3,
    .in1_state = 0,
    .out1_state = 0,
    .mode = 0,
    .stt_rpt_type = 0,
    .msg_num = 0,
    .bck_volt = 0.00,
    .power_volt = 0.00
};
