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
char *command_expr = "device_command/";
const char *value ;
z_keyexpr_t config_key,command_key;


char* keyexpr = "device/motor_status/";
int data_size = 4;
int axis_num = 4;

bool connection = false;

typedef struct {
    z_owned_session_t session;
} ThreadArgs;

int motor_data_test[4][4] = {
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
};



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


char* format_motor_data() {
    char* motor_data_str = malloc(10000 * sizeof(char)); 

    if (motor_data_str == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    char* ptr = motor_data_str;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (j == 0) {
                sprintf(ptr, "[");
                ptr += 1;
            }
            sprintf(ptr, "%d", motor_data_test[i][j]);
            ptr += snprintf(NULL, 0, "%d", motor_data_test[i][j]);
            if (j != 3) {
                sprintf(ptr, ",");
                ptr += 1;
            } else {
                sprintf(ptr, "]");
                ptr += 1;
            }
        }
        sprintf(ptr, "\n");
        ptr += 1;
    }


    return motor_data_str;
}


void publish_motor_status_thread(void *args)
{
    ThreadArgs *arg= (ThreadArgs *)args;
    char* new_key = generate_new_key(axis_num,config_eth.mac_address, keyexpr);


    printf("Declaring Publisher on '%s'...\n", new_key);
    z_owned_publisher_t pub = z_declare_publisher(z_loan(arg->session), z_keyexpr(new_key), NULL);
    if (!z_check(pub)) {
        printf("Unable to declare Publisher for key expression!\n");
        exit(-1);
    }

    // publish csv data 
    while (true) {

        char *data_value = format_motor_data(motor_data_test);
        z_publisher_put_options_t options = z_publisher_put_options_default();
        options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
        z_publisher_put(z_loan(pub), (const uint8_t *)data_value, strlen(data_value), &options);
        usleep(10000);
    
    }
      z_undeclare_publisher(z_move(pub));

}

void decode_command(char *payload ){

    // decode the payload
    

    if (strcmp(payload, "stop")==0)
    {
        strcpy(func,"stop");
        printf("func: %s\n",func);
    }
    else if (strcmp(payload, "reset")==0)
    {
        strcpy(func,"stop");
        printf("func: reset\n");
        for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
        motor_data_test[i][j] = 0;
    }
}

    }
    else{
    

    char* comm = strchr(payload, ':');

    comm++;
    char* action =strchr(comm, '_');
    char axis_id[action - comm +1];
    strncpy(axis_id, comm , action - comm);
    axis_id[action - comm]='\0';


    axis = atoi(axis_id);
    action++;
    command = atoi(action);

    strncpy(func,payload,comm-1-payload);
    func[comm-1-payload]='\0';

    printf("func: %s, axis_id : %d , action : %d\n",func,axis, command);
    }


}

// Function input (APS function)
void function_process()
{
    // Define motor_data
    int slope = 1;
    
    while(true){

    if (strcmp(func , "pulse")==0)

    {

        while((motor_data_test[axis][0] != command) && (strcmp(func , "stop")!=0))
        {
            if (motor_data_test[axis][0] > command)
            {   slope = -1; }
            else{ slope = 1; }
            motor_data_test[axis][1] =(10*slope);
            motor_data_test[axis][0] +=(10*slope);
            usleep(100000);

        }
        motor_data_test[axis][1] = 0 ;
        strcpy(func, "stop");
        
    }
    else if (strcmp(func,"velocity")==0)
    {
        while(strcmp(func , "stop")!=0)
        {
            motor_data_test[axis][1] =command;
            motor_data_test[axis][0] +=command;
            usleep(100000);
        }
        motor_data_test[axis][1] = 0;
        
    }

    }
    
}
void clearScreen() 
{
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}




void command_handler(const z_query_t *query, void *context) {
    z_owned_str_t keystr = z_keyexpr_to_string(z_query_keyexpr(query));
    z_bytes_t pred = z_query_parameters(query);

     char* payload = (char*)malloc(((int)pred.len +1)* sizeof(char));
    memcpy(payload,pred.start,(int)pred.len);
    payload[(int)pred.len]='\0';

    // command format : <fn> : <axis_num>_<action>

    decode_command(payload);

    char* reply = "Received successful!";

    
    z_query_reply_options_t options = z_query_reply_options_default();
    options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
    z_query_reply(query, z_keyexpr((const char *)context), (const unsigned char *)reply, strlen(reply), &options);
    
    z_drop(z_move(keystr));

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

    printf("%s\n",config_eth.ipv4_address);



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
    size_t command_expr_len = strlen(command_expr);

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




    char *command_path = (char*)malloc(command_expr_len + mac_len + 1);
    strcpy(command_path, command_expr);
    strcat(command_path,config_eth.mac_address);

    command_key= z_keyexpr(command_path);
    if (!z_check(command_key)) {
        printf("%s is not a valid key expression", command_path);
        exit(-1);
    }

    printf("Declaring Queryable on '%s'...\n", command_path);
    z_owned_closure_query_t common_callback = z_closure(command_handler, NULL, command_path);
    z_owned_queryable_t common_qable = z_declare_queryable(z_loan(s), command_key, z_move(common_callback), NULL);
    if (!z_check(common_qable)) {
        printf("Unable to create queryable.\n");
        exit(-1);
    }


    ThreadArgs args = {s};

    // query runtime

    if (thrd_create(&thread, function_process, NULL) != thrd_success) {
    fprintf(stderr, "Error creating thread\n");
    return EXIT_FAILURE;
    }
    if (thrd_create(&thread, publish_motor_status_thread, &args) != thrd_success) {
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
    z_undeclare_queryable(z_move(common_qable));
    
    z_close(z_move(s));
    free(config_path);

    return 0;
}
