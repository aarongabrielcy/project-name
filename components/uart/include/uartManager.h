#ifndef UART_MANAGER_H
#define UART_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

#define UART_SIM UART_NUM_1  // Usamos UART1
#define TXD_PIN 4
#define RXD_PIN 5
#define BUF_SIZE (1024)

void uart_init();
void uartManager_start();
int uartManager_readEvent(char *buffer, int max_length);
void uartManager_sendCommand(const char *command);
bool uartManager_sendReadUart(const char *command);
#endif
