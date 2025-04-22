#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

//#define CMD_COUNT 16  // Número de comandos en la lista
#define SERIAL_DEBUG true
// Lista de comandos válidos

typedef enum {
    KLRP = 11,
    PWMC = 12, 
    PWMS = 13,
    RTMS = 14,
    RTMC = 15,
    DRNV = 16,
    TMRP = 17,
    TKRP = 18,
    SVPT = 19,
    CLOP = 20,
    DVID = 21,
    DVIM = 22,
    WTBF = 23,
    DLBF = 24,
    RABF = 25,
    DBMD = 26,
    CLDT = 27,
    CLRP = 28,
    OUT1 = 29,
    RTCT = 30,
} commands_gst_t;

void serialConsole_init();
int validCommand(const char *input);
#endif