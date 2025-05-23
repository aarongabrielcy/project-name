#include "cellnetData.h"
#include <string.h>

// Inicializar los valores por defecto
cellnetData_t cpsi = {
    .sys_mode = "na",
    .oper_mode = "na",
    .mcc = 0,
    .mnc = 0,
    .lac_tac = "FFFF",
    .cell_id = "00000000",
    .rxlvl_rsrp = 999,
    .frequency_band = "NO BAND"
};