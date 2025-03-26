#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <stdbool.h>

void storage_init();

void spiffs_append_record(const char *record);

void spiffs_process_and_delete_blocks(void);

#endif // STORAGE_MANAGER_H