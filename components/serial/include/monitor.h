#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

#define CMD_COUNT 12  // Número de comandos en la lista
#define SERIAL_DEBUG true
// Lista de comandos válidos

typedef enum {
    KLRP,
    PWMC,
    PWMS,
    RTMS,
    RTMC,
    DRNV,
    TMRP,
    TKRP,
    SVPT,
    CLOP,
    DVID,
    DVIM
} commands_gst_t;

void serialConsole_init();
#endif