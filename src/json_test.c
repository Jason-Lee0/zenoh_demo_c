#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"

int main() {
    // 讀取 JSON5 檔案
    FILE *file = fopen("pub.json5", "r");
    if (!file) {
        printf("無法打開檔案\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = (char *)malloc(length + 1);
    fread(data, 1, length, file);
    fclose(file);
    data[length] = '\0';

    // 解析 JSON5
    cJSON *json = cJSON_Parse(data);
    free(data);

    if (!json) {
        printf("解析 JSON5 失敗\n");
        return 1;
    }

    // 在 JSON 對象中增加一個新的鍵值對
    cJSON_AddStringToObject(json, "new_key", "new_value");

    // 將更新後的 JSON 對象轉換回字串
    char *updated_data = cJSON_Print(json);
    cJSON_Delete(json); // 釋放 JSON 對象佔用的記憶體

    if (!updated_data) {
        printf("轉換 JSON 字串失敗\n");
        return 1;
    }

    // 寫入到 JSON5 檔案
    FILE *updated_file = fopen("example.json5", "w");
    if (!updated_file) {
        printf("無法寫入檔案\n");
        free(updated_data);
        return 1;
    }
    fputs(updated_data, updated_file);
    fclose(updated_file);
    free(updated_data);

    return 0;
}