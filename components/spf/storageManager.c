#include "storageManager.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "dirent.h"  // Para listar archivos en SPIFFS

#define TAG "storageManager"

#define MAX_BLOCK_SIZE 2048
#define BASE_PATH "/spiffs"
static int currentBlockNumber = 1;
#define MAX_BLOCK_FILES 100 // Límite de archivos esperados

static long get_file_size(const char *file_path) {
    struct stat st;
    if (stat(file_path, &st) == 0) {
        return st.st_size;
    }
    return 0;
}
int get_sorted_block_list(int *blockNumbers, int maxBlocks) {
    DIR *dir = opendir(BASE_PATH);
    if (dir == NULL) {
        ESP_LOGE(TAG, "No se pudo abrir el directorio %s", BASE_PATH);
        return 0;
    }

    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        int blockNum;
        if (sscanf(entry->d_name, "block_%d.txt", &blockNum) == 1) {
            if (count < maxBlocks) {
                blockNumbers[count++] = blockNum;
            }
        }
    }
    closedir(dir);

    // Ordenar los bloques en orden ascendente
    qsort(blockNumbers, count, sizeof(int), (int (*)(const void *, const void *)) strcmp);
    
    return count;  // Retorna la cantidad de bloques encontrados
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
    //fprintf(fp, "%s\n", record); /// guarda con salto de linea
    fprintf(fp, "%s", record); /// guarda SIN salto de linea
    fclose(fp);
    ESP_LOGI(TAG, "Record appended to %s", filePath);
}
char* spiffs_process_blocks_buffer(int blockNumber) {
    // Buffer estático de tamaño máximo 2048 bytes.
    static char buffer[2049];
    char filePath[64];

    snprintf(filePath, sizeof(filePath), BASE_PATH "/block_%d.txt", blockNumber);

    FILE *fp = fopen(filePath, "r");
    if (fp == NULL) {
        ESP_LOGW(TAG, "No se pudo abrir el archivo %s para procesamiento.", filePath);
        return NULL;
    }

    ESP_LOGI(TAG, "Procesando %s", filePath);

    // Leer hasta 2047 bytes para dejar espacio para el carácter nulo.
    size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytesRead] = '\0';  // Termina la cadena con nulo

    fclose(fp);
    return buffer;
}
/*int spiffs_process_blocks(int blockNumber) {
    char filePath[64];
    snprintf(filePath, sizeof(filePath), BASE_PATH "/block_%d.txt", blockNumber);
    FILE *fp = fopen(filePath, "r");
    if (fp == NULL) {
        ESP_LOGW(TAG, "No se pudo abrir el archivo %s para procesamiento.", filePath);
        return 0;
    }

    ESP_LOGI(TAG, "Procesando %s", filePath);
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp)) {
        //ESP_LOGI(TAG, "Registro: %s", buffer);
        printf(buffer);
        // Aquí puedes añadir el código para procesar cada registro según tus necesidades.
    }
    fclose(fp);
    return 1;
}*/
int spiffs_delete_block(int blockNumber) {
    char filePath[64];
    snprintf(filePath, sizeof(filePath), BASE_PATH "/block_%d.txt", blockNumber);
    if (remove(filePath) == 0) {
        ESP_LOGI(TAG, "Eliminado %s", filePath);
        return 1;
    } else {
        ESP_LOGE(TAG, "Error al eliminar %s", filePath);
        return 0;
    }
}

void spiffs_process_and_delete_all_blocks() {
    int blockNumbers[MAX_BLOCK_FILES];
    int totalBlocks = get_sorted_block_list(blockNumbers, MAX_BLOCK_FILES);
    
    if (totalBlocks == 0) {
        ESP_LOGI(TAG, "No hay bloques para procesar.");
        return;
    }

    ESP_LOGI(TAG, "Total de bloques encontrados: %d", totalBlocks);

    for (int i = 0; i < totalBlocks; i++) {
        int blockNumber = blockNumbers[i];
        ESP_LOGI(TAG, "Procesando bloque %d...", blockNumber);
        // Procesar el bloque
        if (spiffs_process_blocks_buffer(blockNumber) != NULL) {
            // Si el bloque se procesó correctamente, lo eliminamos
            ESP_LOGI(TAG, "Borrando bloque %d...", blockNumber);
            spiffs_delete_block(blockNumber);
        }
    }
    ESP_LOGI(TAG, "Todos los bloques han sido procesados y eliminados.");
}

int get_total_block_count(void) {
    DIR *dir = opendir(BASE_PATH);
    if (dir == NULL) {
        ESP_LOGE(TAG, "No se pudo abrir el directorio: %s", BASE_PATH);
        return 0;
    }
    
    int count = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        int blockNum;
        if (sscanf(entry->d_name, "block_%d.txt", &blockNum) == 1) {
            count++;
        }
    }
    closedir(dir);
    
    return count;
}

int get_first_block_number(void) {
    DIR *dir = opendir(BASE_PATH);
    if (dir == NULL) {
        ESP_LOGE(TAG, "No se pudo abrir el directorio: %s", BASE_PATH);
        return -1;
    }
    
    int minBlock = MAX_BLOCK_FILES;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        int blockNum;
        if (sscanf(entry->d_name, "block_%d.txt", &blockNum) == 1) {
            if (blockNum < minBlock) {
                minBlock = blockNum;
            }
        }
    }
    closedir(dir);
    
    if (minBlock == MAX_BLOCK_FILES) {
        ESP_LOGI(TAG, "No se encontraron archivos block_*.txt en %s", BASE_PATH);
        return -1;
    }
    return minBlock;
}

