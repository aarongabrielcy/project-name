#ifndef UTILITIES_H
#define UTILITIES_H
char* cleanResponse(const char *response);
char *cleanData(char *response, const char *command);
char* cleanATResponse(const char *input);
char *formatDate(const char *date);
char *formatTime(const char *utcTime);
char *formatCoordinates(double coord, char direction);
char *getFormatUTC(const char* input);
const char *formatDevID(const char *input);
char* removeHexPrefix(const char *hexValue);
#endif