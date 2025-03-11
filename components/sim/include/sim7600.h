#ifndef SIM7600_H
#define SIM7600_H

#include <stdbool.h>

void sim7600_init(const char *command);
void sim7600_basic_config();
void sim7600_sendATCommand(const char *command);
int sim7600_readResponse(char *buffer, int max_length);
#endif // SIM7600_H
