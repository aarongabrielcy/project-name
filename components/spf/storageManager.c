#include "storageManager.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_err.h"

#define TAG "storageManager"

#define MAX_BLOCK_SIZE 2048
#define BASE_PATH "/spiffs"
static int currentBlockNumber = 1;

static long get_file_size(const char *file_path) {
    struct stat st;
    if (stat(file_path, &st) == 0) {
        return st.st_size;
    }
    return 0;
}
void storage_init() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = BASE_PATH,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error mounting SPIFFS: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS mounted at %s", BASE_PATH);
    }
}

void spiffs_append_record(const char *record) {
    char filePath[64];
    snprintf(filePath, sizeof(filePath), BASE_PATH"/block_%d.txt", currentBlockNumber);

    // Calculate record length including the newline character.
    size_t recordLen = strlen(record) + 1; // +1 for newline

    // Check the current block file size.
    long currentSize = get_file_size(filePath);
    if ((currentSize + recordLen) > MAX_BLOCK_SIZE) {
        // The block is full; increment block number.
        currentBlockNumber++;
        snprintf(filePath, sizeof(filePath), BASE_PATH"/block_%d.txt", currentBlockNumber);
        ESP_LOGI(TAG, "Switching to new block: %s", filePath);
    }

    // Open the file in append mode.
    FILE *fp = fopen(filePath, "a");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open file %s for appending", filePath);
        return;
    }
    // Write the record followed by a newline.
    fprintf(fp, "%s\n", record);
    fclose(fp);
    ESP_LOGI(TAG, "Record appended to %s", filePath);
}
void spiffs_process_and_delete_blocks(void) {
    int blockNumber = 1;
    char filePath[64];
    while (true) {
        snprintf(filePath, sizeof(filePath), BASE_PATH"/block_%d.txt", blockNumber);
        FILE *fp = fopen(filePath, "r");
        if (fp == NULL) {
            // No more block files found.
            break;
        }
        ESP_LOGI(TAG, "Processing %s", filePath);
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), fp)) {
            ESP_LOGI(TAG, "Record: %s", buffer);
        }
        fclose(fp);
        // Delete the block file after processing.
        if (remove(filePath) == 0) {
            ESP_LOGI(TAG, "Deleted %s", filePath);
        } else {
            ESP_LOGE(TAG, "Error deleting %s", filePath);
        }
        blockNumber++;
    }
    // Reset the block counter for new records.
    currentBlockNumber = 1;
}