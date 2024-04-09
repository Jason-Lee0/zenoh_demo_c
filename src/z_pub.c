//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//
#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include "zenoh.h"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#define sleep(x) Sleep(x * 1000)
#else
#include <unistd.h>
#endif

#define MAX_COLS 4
#define MAX_ROWS 10000
#define MAX_LENGTH 10000
char ***csv_data;

void convert_to_char_array(int** array, int rows, int cols, char *char_array) {
  char *p = char_array;
    for (int i = 0; i < rows; ++i) {
        *p++ = '['; // 开始行
        for (int j = 0; j < cols; ++j) {
            if (j > 0) {
                *p++ = ',';
                *p++ = ' ';
            }
            // 将整数转换为字符并存储到字符数组中
            int value = array[i][j];
            int count = snprintf(p, 10, "%d", value); // 将整数转换为字符
            p += count;
        }
        *p++ = ']'; // 结束行
        if (i < rows - 1) {
            *p++ = '\n'; // 行之间用逗号分隔

        }
    }
    *p = '\0'; // 添加字符串结束符
}


char* generate_new_key(int axis_num, const char* mac_address, const char* keyexpr) {
    // 计算 axis_num 的字符串长度
    int str_len = snprintf(NULL, 0, "%d", axis_num) + 1;

    // 动态分配内存来存储 axis_num 的字符串表示
    char* axis = (char *)malloc(str_len * sizeof(char));

    if (axis == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    // 将 axis_num 转换为字符串并存储到 axis 中
    snprintf(axis, str_len, "%d", axis_num);

    // 计算新字符串的长度
    size_t key_len = strlen(keyexpr);
    size_t mac_len = strlen(mac_address);

    // 分配足够的内存来存储拼接后的字符串
    char* new_key = malloc(key_len + mac_len + str_len + 2); // 加 2 是为了存储下划线和终止符

    // 检查内存分配是否成功
    if (new_key == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(axis); // 释放已分配的内存
        return NULL;
    }

    // 拼接字符串
    strcpy(new_key, keyexpr);
    strcat(new_key, mac_address);
    strcat(new_key, "_");
    strcat(new_key, axis);

    // 释放动态分配的内存
    free(axis);

    return new_key;
}

char* declare_value(int data_size, int axis_num)
{
    int **array = (int **)malloc(axis_num * sizeof(int *));
    if (array == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }

    // 分配每行的内存
    for (int i = 0; i < axis_num; ++i) {
        array[i] = (int *)malloc(data_size * sizeof(int));
        if (array[i] == NULL) {
            printf("Memory allocation failed\n");
            // 如果分配失败，释放已分配的内存并退出
            for (int j = 0; j < i; ++j) {
                free(array[j]);
            }
            free(array);
            return 1;
        }
    }

    // data input 

    for (int i = 0; i < axis_num; ++i) {
    for (int j = 0; j < data_size; ++j) {
            
            array[i][j] = 1;
        }
    }

    char *value = (char *)malloc((axis_num * data_size *10 + axis_num +1) * sizeof(char));

    convert_to_char_array(array, axis_num, data_size, value);

    return value;

}

char *declare_value_csv_data(int data_size, int axis_num, char *csv_data[MAX_ROWS][MAX_COLS], int time) 
{
    char *value = (char *)malloc((axis_num * data_size * 20000 + axis_num + 1) * sizeof(char));
        if (value == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            return NULL;
        }

        char *ptr = value;
        for (int i = 0; i < axis_num; ++i) {
            *ptr++ = '[';
            for (int j = 0; j < data_size; ++j) {
                if (j > 0) {
                    *ptr++ = ',';
                    *ptr++ = ' ';
                }
                
                // Copy csv_data value to value buffer
                strcat(ptr, csv_data[time][j]); // Assuming csv_data is a 3D array of strings
                ptr += strlen(csv_data[time][j]);
            }
            *ptr++ = ']';
            *ptr++ = '\n';
        }
        *ptr = '\0';  // Add null terminator to indicate end of string

    return value;
}


int main(int argc, char **argv) {



    FILE *fp = fopen("../a.csv", "r");
    if (fp == NULL) {
        
        fprintf(stderr, "Failed to open file.\n");
        return 1;
    }


    //创建二维数组来存储 CSV 数据
    csv_data = (char ***)malloc(MAX_ROWS * sizeof(char **));
    if (csv_data == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    for (int i = 0; i < MAX_ROWS; i++) {
        csv_data[i] = (char **)malloc(MAX_COLS * sizeof(char *));
        if (csv_data[i] == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            // 释放已经分配的内存
            for (int j = 0; j < i; j++) {
                free(csv_data[j]);
            }
            free(csv_data);
            return 1;
        }
        for (int j = 0; j < MAX_COLS; j++) {
            csv_data[i][j] = (char *)malloc(MAX_LENGTH * sizeof(char));
            if (csv_data[i][j] == NULL) {
                fprintf(stderr, "Memory allocation failed.\n");
                // 释放已经分配的内存
                for (int k = 0; k < i; k++) {
                    for (int l = 0; l < j; l++) {
                        free(csv_data[k][l]);
                    }
                    free(csv_data[k]);
                }
                free(csv_data);
                return 1;
            }
        }
    } // 假设每个单元格最多有 100 个字符

    // 逐行读取 CSV 文件
    int row = 0;
      char line[MAX_LENGTH];
    while (fgets(line, sizeof(line), fp) != NULL && row < MAX_ROWS) {
        // 去掉换行符
        line[strlen(line) - 1] = '\0';

        // 使用逗号分割每行数据
        char *token = strtok(line, ",");
        int col = 0;
        while (token != NULL && col < MAX_COLS) {
            // 将数据存储到二维数组中
            strcpy(csv_data[row][col], token);
            token = strtok(NULL, ",");
            col++;
        }
        row++;
    }


    // 关闭文件
    fclose(fp);

    // 打印读取的 CSV 数据
    // for (int i = 0; i < 1; i++) {
    //     for (int j = 0; j < MAX_COLS; j++) {
    //         printf("%s\t", csv_data[i][j]);
    //     }
    //     printf("\n");
    // }



    char* keyexpr = "device/motor_status/";

    char *mac_address = "00:30:64:61:4b:41";

    int data_size = 4;
    int axis_num = 4 ;

    char* new_key = generate_new_key(axis_num, mac_address, keyexpr);
    

    //char* value = declare_value(data_size,axis_num);

    
    char *value = (char *)malloc((axis_num * data_size * 20000 + axis_num + 1) * sizeof(char));
    if (value == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    char *ptr = value;
    // for (int i = 0; i < axis_num; ++i) {
    //     *ptr++ = '[';
    //     for (int j = 0; j < data_size; ++j) {
    //         if (j > 0) {
    //             *ptr++ = ',';
    //             *ptr++ = ' ';
    //         }
            
    //         // Copy csv_data value to value buffer
    //         strcat(ptr, csv_data[0][j]); // Assuming csv_data is a 3D array of strings
    //         ptr += strlen(csv_data[0][j]);
    //     }
    //     *ptr++ = ']';
    //     *ptr++ = '\n';
    // }
    // *ptr = '\0';  // Add null terminator to indicate end of string


    

    // z_owned_config_t config = z_config_default();
    
    //     if (zc_config_insert_json(z_loan(config), Z_CONFIG_LISTEN_KEY, "[\"tcp/169.254.28.145:7777\"]" ) < 0) {
    //         printf("failed!!");
    //         exit(-1);
    //     }
    z_owned_config_t config = zc_config_from_file("result.json5") ;

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }

    printf("Declaring Publisher on '%s'...\n", new_key);
    z_owned_publisher_t pub = z_declare_publisher(z_loan(s), z_keyexpr(new_key), NULL);
    if (!z_check(pub)) {
        printf("Unable to declare Publisher for key expression!\n");
        exit(-1);
    }

    char buf[256];


    printf("\n");

    for (int index=0 ; index < MAX_ROWS ; index++){

         memset(value, '\0', (axis_num * data_size * 20000 + axis_num + 1) * sizeof(char));
        ptr = value;
        for (int i = 0; i < axis_num; ++i) {
            *ptr++ = '[';
            for (int j = 0; j < data_size; ++j) {
                if (j > 0) {
                    *ptr++ = ',';
                    *ptr++ = ' ';
                }
                
                // Copy csv_data value to value buffer
                strcat(ptr, csv_data[index][j]); // Assuming csv_data is a 3D array of strings
                ptr += strlen(csv_data[index][j]);
            }
        *ptr++ = ']';
        *ptr++ = '\n';
        }
        *ptr = '\0';  // Add null terminator to indicate end of string
       
        
        printf("Putting Data byte size... %d \n", strlen(value));

        z_publisher_put_options_t options = z_publisher_put_options_default();
        options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
        z_publisher_put(z_loan(pub), (const uint8_t *)value, strlen(value), &options);
        printf("index : %d\n", index);

        if (index +1 == MAX_ROWS){
            index = 0;
        }
        sleep(1);
        

        
    }
    

    z_undeclare_publisher(z_move(pub));
    free(csv_data);
    free(value);

    z_close(z_move(s));
    return 0;
}
