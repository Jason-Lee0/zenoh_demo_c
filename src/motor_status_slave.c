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

#include <stdio.h>
#include <string.h>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#define sleep(x) Sleep(x * 1000)
#else
#include <unistd.h>
#endif
#include "zenoh.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <threads.h>
#include <time.h>
#include <sys/time.h>
#include "scout_eth.h"


#define MAX_FUNC_LENGTH 20

//pub_test
#define MAX_COLS 4
#define MAX_ROWS 10000
#define MAX_LENGTH 10000
#define DATA_SIZE 4
#define AXIS_NUM 128

char ***csv_data;
addr_config config_eth;


char func[MAX_FUNC_LENGTH];
int axis;
int command;

char *config_expr = "device_config/";

const char *value ;
z_keyexpr_t config_key;


char* keyexpr = "device/motor_status/";
int data_size = 4;
int axis_num = 4;

bool connection = false;

typedef struct {
    z_owned_session_t session;
} ThreadArgs;



char* generate_new_key(int axis_num, const char* mac_address, const char* keyexpr) {
    
    int str_len = snprintf(NULL, 0, "%d", axis_num) + 1;

    char* axis = (char *)malloc(str_len * sizeof(char));

    if (axis == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    snprintf(axis, str_len, "%d", axis_num);
    size_t key_len = strlen(keyexpr);
    size_t mac_len = strlen(mac_address);


    char* new_key = malloc(key_len + mac_len + str_len + 2); 
    if (new_key == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(axis);
        return NULL;
    }

    strcpy(new_key, keyexpr);
    strcat(new_key, mac_address);
    strcat(new_key, "_");
    strcat(new_key, axis);

    free(axis);

    return new_key;
}

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

void publish_motor_status_thread_dataset(void *args)
{
    ThreadArgs *arg= (ThreadArgs *)args;

    load_csv_data("../dataset.csv");

    
    char *value = (char *)malloc((axis_num * data_size * 20000 + axis_num + 1) * sizeof(char));
    if (value == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    char *ptr = value;

    char* new_key = generate_new_key(axis_num, config_eth.mac_address, keyexpr);


    printf("Declaring Publisher on '%s'...\n", new_key);
    z_owned_publisher_t pub = z_declare_publisher(z_loan(arg->session), z_keyexpr(new_key), NULL);
    if (!z_check(pub)) {
        printf("Unable to declare Publisher for key expression!\n");
        exit(-1);
    }

    // publish csv data 
    while (true) {

        if (connection==true){
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
                    
                    
                    strcat(ptr, csv_data[index][j]); 
                    ptr += strlen(csv_data[index][j]);
                }
            *ptr++ = ']';
            *ptr++ = '\n';
            }
            *ptr = '\0';  
            

            z_publisher_put_options_t options = z_publisher_put_options_default();
            options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
            z_publisher_put(z_loan(pub), (const uint8_t *)value, strlen(value), &options);
            
            if (index +1 == MAX_ROWS){
                index = 0;
            }

            usleep(10000);     
            }
        }
    }
    
    z_undeclare_publisher(z_move(pub));
    free(csv_data);
    free(value);

}

void config_handler(const z_query_t *query, void *context) {
    z_owned_str_t keystr = z_keyexpr_to_string(z_query_keyexpr(query));
    z_bytes_t pred = z_query_parameters(query);
    z_value_t payload_value = z_query_value(query);
    value = config_eth.mac_address;

    
   
    z_query_reply_options_t options = z_query_reply_options_default();
    options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
    z_query_reply(query, z_keyexpr((const char *)context), (const unsigned char *)value, strlen(value), &options);

    z_drop(z_move(keystr));

    //printf("connection_status : %d\n",connection);
    connection = true;
}


long long current_time_millis() {
    struct timeval te;
    gettimeofday(&te, NULL);
    return te.tv_sec * 1000LL + te.tv_usec / 1000;
}

int main(int argc, char **argv) {

    if (argv[1])
    {
        axis_num = atoi(argv[1]);
        printf("change axis_num\n");
    }
    
    long long time_start = current_time_millis();
 
    z_owned_config_t config = z_config_default();

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }

    thrd_t thread;

    printf("Scouting...\n");

    config_eth = scout_eth();


    printf("\nClose current session. Reopen Session by using Link-local IPv4 address.\n");

    z_close(z_move(s));


    config = z_config_default();

    char listen_port[50];
    sprintf(listen_port, "[\"tcp/%s:7777\"]",config_eth.ipv4_address);


    printf("add listen_port : %s \n", listen_port);

    if (zc_config_insert_json(z_loan(config), Z_CONFIG_LISTEN_KEY, listen_port) < 0) {
            printf(
                "Couldn't insert value  in configuration at `%s`. This is likely because `%s` expects a "
                "JSON-serialized list of strings\n",
                Z_CONFIG_LISTEN_KEY, Z_CONFIG_LISTEN_KEY);
            exit(-1);
        }

    printf("Opening session by using Link-local IPv4 address - %s \n\n",config_eth.ipv4_address);



    s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    long long time_end = current_time_millis();
    double time_spent = (time_end - time_start) / 1000.0;
    printf("Time spent: %f  seconds\n\n", time_spent);


    
    size_t config_expr_len = strlen(config_expr);


    size_t mac_len = strlen(config_eth.mac_address);


    char *config_path = (char *)malloc(config_expr_len + mac_len + 1);

    if (config_path == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    strcpy(config_path, config_expr);
    strcat(config_path, config_eth.mac_address);
    
    config_key = z_keyexpr(config_path);
    if (!z_check(config_key)) {
        printf("%s is not a valid key expression", config_path);
        exit(-1);
    }

    printf("Declaring Queryable on '%s'...\n", config_path);
    z_owned_closure_query_t config_callback = z_closure(config_handler, NULL, config_path);
    z_owned_queryable_t config_qable = z_declare_queryable(z_loan(s), config_key, z_move(config_callback), NULL);
    if (!z_check(config_qable)) {
        printf("Unable to create queryable.\n");
        exit(-1);
    }


    ThreadArgs args = {s};

    //publish dataset
    
    if (thrd_create(&thread, publish_motor_status_thread_dataset, &args) != thrd_success) {
    fprintf(stderr, "Error creating thread\n");
    return EXIT_FAILURE;
    }


    printf("Enter 'q' to quit...\n");
    char c = 0;
    while (c != 'q') {
        c = getchar();
        if (c == -1) {
            sleep(1);
        }
    }

    z_undeclare_queryable(z_move(config_qable));

    
    z_close(z_move(s));
    free(config_path);

    return 0;
}
