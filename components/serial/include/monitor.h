#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

#define CMD_COUNT 5  // Número de comandos en la lista

// Lista de comandos válidos

void serialConsole_init();
void parseCommand(const char *input);
#endif