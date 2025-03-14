#ifndef MODULEDATA_H
#define MODULEDATA_H

#include <stdbool.h>

#define ANGLE_THRESHOLD 15.0  // Umbral de cambio de ángulo
//extern bool reportFastMode;
//extern int timeReport = 30;

void parseGPS(char *response);
//void updateReportRate(int seconds);
bool parsePSI(char *response);
void parseGSM(char *tokens);
void parseLTE(char *tokens);
void parseWCDMA(char *tokens);
void parseCDMA(char *tokens);
void parseEVDO(char *tokens);
#endif