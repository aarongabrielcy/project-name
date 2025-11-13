#ifndef SIM7600_H
#define SIM7600_H

#include <stdbool.h>
#include <stdint.h>

void sim7600_init(const char *command);
void sim7600_basic_config();
void sim7600_sendATCommand(const char *command);
int sim7600_readResponse(char *buffer, int max_length, int timeout_ms);
int sim7600_readBinary(uint8_t *buffer, int max_length, int timeout_ms);
void sim7600_reconnect_tcp_service();
void sim7600_reconnect_tcp_server();
bool sim7600_sendReadCommand(const char *command);
#endif // SIM7600_H
