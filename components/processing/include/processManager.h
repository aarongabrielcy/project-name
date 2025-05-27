#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <stdbool.h>

#define ANGLE_THRESHOLD 15.0  // Umbral de cambio de ángulo
#define TRACKING_SPEED 200.0
//extern bool reportFastMode;
//extern int timeReport = 30;

bool parseGPS(char *response);
//void updateReportRate(int seconds);
bool parsePSI(char *response);
void parseGSM(char *tokens);
void parseLTE(char *tokens);
void parseWCDMA(char *tokens);
void parseCDMA(char *tokens);
void parseEVDO(char *tokens);
#endif