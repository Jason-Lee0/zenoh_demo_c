#include <stdio.h>

int main() {
    // 開啟原始 PEM 檔案
    FILE *inputFile = fopen("minica.pem", "r");
    if (!inputFile) {
        printf("無法打開檔案\n");
        return 1;
    }


    // 移動檔案指標到檔案結尾，以確定檔案大小
    fseek(inputFile, 0, SEEK_END);
    long fileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);

    // 讀取 PEM 檔案的內容
    char *pemContent = (char *)malloc(fileSize + 1);
    fread(pemContent, 1, fileSize, inputFile);
    pemContent[fileSize] = '\0';
    fclose(inputFile);

    // 開啟目標 PEM 檔案
    FILE *outputFile = fopen("example_written.pem", "w");
    if (!outputFile) {
        printf("無法打開檔案\n");
        free(pemContent);
        return 1;
    }

    // 將 PEM 內容寫入目標檔案
    fputs(pemContent, outputFile);
    fclose(outputFile);
    free(pemContent);

    return 0;
}
