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
#include <unistd.h>
#include "zenoh.h"
#include "uthash.h"
#include <time.h>
#include <threads.h>

#define MAX_PAYLOAD_SIZE 1000 

//char payload[MAX_PAYLOAD_SIZE + 1];
char *device_config_expr = "device_config/**";
char *device_command_expr = "device_command/00:30:64:61:4b:40";
char motor_status[10000];

// common_input_demo
int comm_input;

z_owned_session_t s ;
const char *kind_to_str(z_sample_kind_t kind);

const char *kind_to_str(z_sample_kind_t kind) {
    switch (kind) {
        case Z_SAMPLE_KIND_PUT:
            return "PUT";
        case Z_SAMPLE_KIND_DELETE:
            return "DELETE";
        default:
            return "UNKNOWN";
    }
}

#define MAX_MAC_LENGTH 18

typedef struct {
    char id[MAX_MAC_LENGTH];
    long int timer;            
    int online_status;
    char* axis_num[3];
    char* motor_info[10000];    
    UT_hash_handle hh; 
} Data_status;

Data_status *device_status = NULL; 


void add_user(char *id, long int timer ) {

    Data_status *device = (Data_status*)malloc(sizeof(Data_status));
    strncpy(device->id, id, sizeof(device->id) - 1);
    device->timer = timer;  
    device->online_status = 0; 
    // for (int i = 0; i < 100; i++)  device->axis_num[i] = NULL;
   
    // for (int i = 0; i < 10000; i++) device->motor_info[i] = NULL;  
        
    HASH_ADD_STR(device_status, id , device);  
}


Data_status *find_user(char *id) {
    Data_status *device;
    HASH_FIND_STR(device_status, id, device);  
    return device;
}


void clearScreen() 
{
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void check_device_status() 
{
    Data_status *device, *tmp;

    while(true)
    {
    time_t time_check = time(NULL);

    HASH_ITER(hh, device_status, device, tmp) {
        if ((time_check - device->timer) > 3 )
        {
            device->online_status = 0; 
        } else
        {
            device->online_status = 1; 
        }
        }
    sleep(1);
    }

}



void update_device_info(z_sample_t sample  )
{
    char* payload = (char*)malloc((sample.payload.len +1)* sizeof(char));
    memcpy(payload, sample.payload.start, sample.payload.len);
    payload[sample.payload.len] = '\0'; 
    time_t t;


    t = time(NULL);
    Data_status *device = find_user(payload);
    if(device != NULL)
    {
        
        device->timer = t;
    }
    else{
        printf("found new device , add : %s\n",payload);
        add_user(payload,t);
        

    }

}

void get_reply(z_owned_session_t s)
{
    z_owned_reply_channel_t channel = zc_reply_fifo_new(16);
     z_keyexpr_t keyexpr = z_keyexpr(device_config_expr);
     z_get_options_t opts = z_get_options_default();
    if (!z_check(keyexpr)) {
        printf("%s is not a valid key expression", device_config_expr);
        exit(-1);
    }
    z_get(z_loan(s), keyexpr, "", z_move(channel.send),
        &opts);  // here, the send is moved and will be dropped

    z_owned_reply_t reply = z_reply_null();
    for (z_call(channel.recv, &reply); z_check(reply); z_call(channel.recv, &reply)) {
        if (z_reply_is_ok(&reply)) {
            z_sample_t sample = z_reply_ok(&reply);
            z_owned_str_t keystr = z_keyexpr_to_string(sample.keyexpr);

            //printf("update - %.*s \n",(int)sample.payload.len, sample.payload.start);
            update_device_info(sample);
            z_drop(z_move(keystr));
        } else {
            printf("Received an error\n");
        }
    }
    z_drop(z_move(reply));
    z_drop(z_move(channel));
}


void get_common_reply(z_owned_session_t s, int case_num)
{
     z_owned_reply_channel_t channel = zc_reply_fifo_new(16);
     z_keyexpr_t keyexpr = z_keyexpr(device_command_expr);
     z_get_options_t opts = z_get_options_default();

    char* command = (char*)malloc(20 );
    if (!z_check(keyexpr)) {
        printf("%s is not a valid key expression", device_command_expr);
        exit(-1);
    }
    switch(case_num)
    {
        case 1 :
            strcpy(command,"pulse:1_1000");
            break;
        case 2 :
            strcpy(command,"velocity:2_10");
            break;
        case 3 :
            strcpy(command,"stop");
            break;
        case 4 :
            strcpy(command,"reset");
            break;


    }

    z_get(z_loan(s), keyexpr, command, z_move(channel.send),
        &opts);  // here, the send is moved and will be dropped

    z_owned_reply_t reply = z_reply_null();
     for (z_call(channel.recv, &reply); z_check(reply); z_call(channel.recv, &reply)) {
        if (z_reply_is_ok(&reply)) {

            z_sample_t sample = z_reply_ok(&reply);
            z_owned_str_t keystr = z_keyexpr_to_string(sample.keyexpr);
            printf(">> Received ('%s': '%.*s')\n", z_loan(keystr), (int)sample.payload.len, sample.payload.start);
            z_drop(z_move(keystr));
            
        } else {
            printf("Received an error\n");
        }
    }
   
    z_drop(z_move(reply));
    z_drop(z_move(channel));
    free(command);
}


void common_input()
{
    int comm_input;
    printf("\n\nCommand Demo : \n");
    printf("(1) Position command : <axis 1>_<position 1000> \n");
    printf("(2) Velocity command : <axis 2>_<velocity 10> \n");
    printf("(3) All axis stop \n");
    printf("(4) Reset all axis  \n");
    printf("Choose the command :");
    scanf("%d",&comm_input);

    switch(comm_input)
    {
        case 1 :
        get_common_reply(s,1);
        break;
        case 2 :
        get_common_reply(s,2);
        break;
        case 3 :
        get_common_reply(s,3);
        break;
        case 4 :
        get_common_reply(s,4);
        break;


    }
}
int main(int argc, char **argv) {
  
    char *expr = "device_config/**";
    char *value = NULL;
    thrd_t thread;
    switch (argc) {
        default:
        case 3:
            value = argv[2];
        case 2:
            expr = argv[1];
            break;
        case 1:
            // Do nothing
            break;
    }
   
    z_owned_config_t config = z_config_default();
    if (argc > 3) {
        if (zc_config_insert_json(z_loan(config), Z_CONFIG_CONNECT_KEY, argv[3]) < 0) {
            printf(
                "Couldn't insert value `%s` in configuration at `%s`. This is likely because `%s` expects a "
                "JSON-serialized list of strings\n",
                argv[3], Z_CONFIG_CONNECT_KEY, Z_CONFIG_CONNECT_KEY);
            exit(-1);
        }
    }

    printf("Opening session...\n");
    s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }

    printf("Sending Query '%s'...\n", expr);

    if (thrd_create(&thread, check_device_status, NULL) != thrd_success) {
        fprintf(stderr, "Error creating thread\n");
        return EXIT_FAILURE;
    }

    while(true)
    {
    get_reply(s);
    common_input();

    sleep(1);
    }

    z_close(z_move(s));
    return 0;
}