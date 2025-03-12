#include "serviceInfo.h"
#include <string.h>

// Inicializar los valores por defecto
serviceInfo_t serInf = {
    .sys_mode = "na",
    .oper_mode = "na",
    .mcc = 0,
    .mnc = 0,
    .lac_tac = "FFFF",
    .cell_id = "00000000",
    .rxlvl_rsrp = 999,
};