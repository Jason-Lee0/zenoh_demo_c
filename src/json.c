#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENDPOINTS 100 // 最大端點數量
#define MAX_ENDPOINT_LENGTH 50 // 每個端點的最大長度
#define MAX_FILE_SIZE 10000 // 文件的最大大小

// JSON5檔案的結構
typedef struct {
    char content[MAX_FILE_SIZE];
    int content_length;
    char mode[10];
    char listen_endpoints[MAX_ENDPOINTS][MAX_ENDPOINT_LENGTH];
    int num_endpoints;
} JsonData;

// 讀取JSON5檔案
void readJsonFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    char* output="../result.json5";
    FILE* output_file = fopen(output, "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }


    char line[100];
    char* ptr;
    while (fgets(line, sizeof(line), file)) {
        if (strchr(line,'[')!= NULL)
        {
            strcat(line, "\t\"tcp/169.254.28.145:7777\"");
            printf("%s\n",line);
        }
        fprintf(output_file,"%s",line);
        
    }

    fclose(file);
    fclose(output_file);
}

// 將JSON5數據寫回文件
void writeJsonFile(const char* filename, JsonData* jsonData) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    fprintf(file, "%s", jsonData->content);

    fclose(file);
}

// 釋放記憶體
void freeJsonData(JsonData* jsonData) {
    if (jsonData != NULL) {
        free(jsonData);
    }
}

int main() {
    const char* filename = "../sub.json5";
    readJsonFile(filename);

    return 0;
}