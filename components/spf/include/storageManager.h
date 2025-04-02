#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <stdbool.h>

void storage_init();
int get_sorted_block_list(int *blockNumbers, int maxBlocks);
void spiffs_append_record(const char *record); //CREATE
char *spiffs_process_blocks_buffer(int blockNumber); //READ
int spiffs_delete_block(int blockNumber); //DELETE
void spiffs_process_and_delete_all_blocks();
int get_total_block_count(void);
int get_first_block_number(void);
#endif // STORAGE_MANAGER_H