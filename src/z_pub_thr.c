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

#include "zenoh.h"

#define MAX_COLS 4
#define MAX_ROWS 10000
#define MAX_LENGTH 10000
#define DATA_SIZE 4
#define AXIS_NUM 128

char ***csv_data;

int data_size = 4;
int axis_num = 128;

void load_csv_data(const char *path)
{
    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        
        fprintf(stderr, "Failed to open file.\n");
        return 1;
    }

    csv_data = (char ***)malloc(MAX_ROWS * sizeof(char **));
    if (csv_data == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    for (int i = 0; i < MAX_ROWS; i++) {
        csv_data[i] = (char **)malloc(MAX_COLS * sizeof(char *));
        if (csv_data[i] == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
           
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
    } 

   
    int row = 0;
    char line[MAX_LENGTH];
    while (fgets(line, sizeof(line), fp) != NULL && row < MAX_ROWS) {
        
        line[strlen(line) - 1] = '\0';

        char *token = strtok(line, ",");
        int col = 0;
        while (token != NULL && col < MAX_COLS) {

            strcpy(csv_data[row][col], token);
            token = strtok(NULL, ",");
            col++;
        }
        row++;
    }

    fclose(fp);

}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("USAGE:\n\tz_pub_thr <payload-size> [<zenoh-locator>]\n\n");
        exit(-1);
    }

     char *keyexpr = "test/thr";

    z_owned_config_t config = z_config_default();
    if (argc > 2) {
        if (zc_config_insert_json(z_loan(config), Z_CONFIG_CONNECT_KEY, argv[2]) < 0) {
            printf(
                "Couldn't insert value `%s` in configuration at `%s`. This is likely because `%s` expects a "
                "JSON-serialized list of strings\n",
                argv[2], Z_CONFIG_CONNECT_KEY, Z_CONFIG_CONNECT_KEY);
            exit(-1);
        }
    }

    axis_num = atoi(argv[1]);
    printf("change axis_num : %d \n", axis_num);

    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }

    z_publisher_options_t options = z_publisher_options_default();
    options.congestion_control = Z_CONGESTION_CONTROL_BLOCK;

    z_owned_publisher_t pub = z_declare_publisher(z_loan(s), z_keyexpr(keyexpr), &options);
    if (!z_check(pub)) {
        printf("Unable to declare publisher for key expression!\n");
        exit(-1);
    }

    load_csv_data("../a.csv");
    printf(" Load motion dataset\n");


    char *data = (char *)malloc((axis_num * data_size * 20000 + axis_num + 1) * sizeof(char));
    if (data == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    char *ptr = data;
    int len = 0;

    // publish csv data 

    for (int index=0 ; index < MAX_ROWS ; index++){

    memset(data, '\0', (axis_num * data_size * 20000 + axis_num + 1) * sizeof(char));
    ptr = data;
    for (int i = 0; i < axis_num; ++i) {
        *ptr++ = '[';
        for (int j = 0; j < data_size; ++j) {
            if (j > 0) {
                *ptr++ = ',';
                *ptr++ = ' ';
            }
            
            
            strcat(ptr, csv_data[index][j]); 
            ptr += strlen(csv_data[index][j]);
        }
    *ptr++ = ']';
    *ptr++ = '\n';
    }
    *ptr = '\0';  
    if (len < strlen(data)){ len = strlen(data);}




    }
    
    printf("Max data size : %d\n",len);


    char* value_test = (char *)malloc(len+1);
    memset(value_test, '1', len);
    value_test[len]='\0';
    while(1){
    
        z_publisher_put(z_loan(pub), (const char *)value_test, len, NULL);
        //printf("data_size : %d\n",strlen(value));

    }

    free(value_test);
    free(data);

    z_undeclare_publisher(z_move(pub));
    z_close(z_move(s));

}
