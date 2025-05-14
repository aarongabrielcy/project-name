#include "gnssData.h"
#include <string.h>

// Inicializar los valores por defecto
gnssData_t gnss = {
    .mode = 0,
    .gps_svs = 0,
    .glss_svs = 0,
    .beid_svs = 0,
    .lat = 0.0,
    .ns = 'N',
    .lon = 0.0,
    .ew = 'W',
    .date = "00000000",
    .utctime = "00:00:00",
    .alt = 0.0,
    .speed = 0.00,
    .course = 0.00,    
    .pdop = 0.0,
    .hdop = 0.0,
    .vdop = 0.0,
    .fix = 0,
};