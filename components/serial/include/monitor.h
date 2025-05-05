#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

//#define CMD_COUNT 16  // Número de comandos en la lista
#define SERIAL_DEBUG true
// Lista de comandos válidos

/*typedef enum {
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
    OPCT = 29,
    RTCT = 30,
    IGST = 31,
    SIID = 32
} commands_gst_t;*/
typedef enum {
    KPRP = 11,
    RTMS = 12,
    RTMC = 13,
    SVPT = 14,
    TMRP = 15,
    DLBF = 16,
    CLRP = 17,
    RTDV = 18,
    OPCT = 19,
  } cmd_action_t;
  
  typedef enum {
    TKRP = 21,
    CLOP = 22,
    DVID = 23,
    DVIM = 24,
    CLDT = 25,
    RTCT = 26,
    IGST = 27,
    SIID = 28,
    OPST = 29
  } cmd_query_t;

  typedef enum {
    EMPTY = 0,
    QUERY_WITHOUT_VALUE = 1,
    QUERY_WITH_VALUE = 2,
    ACTION = 3,
    INVALID_CMD = 4,
    INVALID_SYMBOL = 5,
    INVALID_ACTION = 6,
    INVALID_NUMBER = 7,
    INVALID_QUERY_VALUE = 8,
    INVALID_END_SYMBOL = 9
} type_command_t;

void serialConsole_init();
char *processCmd(const char *command);
//int validCommand(const char *input);
#endif