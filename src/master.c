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
#include <pthread.h>

#define MAX_PAYLOAD_SIZE 1000 

char payload[MAX_PAYLOAD_SIZE + 1];
char *device_config_expr = "device_config/**";
char motor_status[10000];

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


typedef struct {
    char id[100];
    long int timer;            
    int online_status;
    char* axis_num[100];
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

void data_handler(const z_sample_t *sample, void *arg) {
    
    
    z_owned_str_t keystr = z_keyexpr_to_string(sample->keyexpr);
    char *title,*axis;

    title = strrchr(keystr._cstr, '/');
    title ++;
    

    if (title != NULL) {

        char *axis = strchr(title, '_');
        char device_info[axis - title + 1]; 
        strncpy(device_info, title, axis - title);
        device_info[axis - title] = '\0'; 
        
        axis++;
        
        Data_status *d = find_user(device_info);

        if (d != NULL) {
            strcpy(d->axis_num,axis);
            memcpy(motor_status, sample->payload.start, sample->payload.len);
            motor_status[sample->payload.len] = '\0'; 
            strcpy(d->motor_info, motor_status);

            
        }
       

        //printf("motor_info : %s\n",d->motor_info);
        

    }
    else {
        printf("wrong_key_path\n");
        return;
    }
}


void get_motor_status(z_owned_session_t zenoh_session)
{

    char *motor_info = "device/motor_status/**";


    z_owned_closure_sample_t callback = z_closure(data_handler);
    printf("Declaring Subscriber on '%s'...\n", motor_info);
    z_owned_subscriber_t sub = z_declare_subscriber(z_loan(zenoh_session), z_keyexpr(motor_info), z_move(callback), NULL);
    if (!z_check(sub)) {
        printf("Unable to declare subscriber.\n");
        exit(-1);
    }
    
    
}

void update_device_info(z_sample_t sample  )
{

    memcpy(payload, sample.payload.start, sample.payload.len);
    payload[sample.payload.len] = '\0'; // 添加字符串结束符
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

void print_device_info() {
    Data_status *d, *tmp;
    
    while (true) {
        clearScreen();
        HASH_ITER(hh, device_status, d, tmp) {
            if (d->online_status == 1) {
                printf("ID: %s [online!!] \n", d->id);

                printf("Axis_num : %s\n",d->axis_num);
                printf("Motor_status:\n%s\n", d->motor_info);
               
                printf("\n");
            }
            else 
            {
                printf("ID: %s [offline!!] \n", d->id);
            }
        }
        usleep(10000);
    }
}
int main(int argc, char **argv) {
  
    char *expr = "device_config/**";
    char *value = NULL;
    pthread_t thread;
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

    if (pthread_create(&thread,NULL, check_device_status, NULL)  != 0) {
        fprintf(stderr, "Error creating thread\n");
        return EXIT_FAILURE;
    }

    if (pthread_create(&thread,NULL, print_device_info, NULL)  != 0) {
        fprintf(stderr, "Error creating thread\n");
        return EXIT_FAILURE;
    }

    get_motor_status(s);

    while(true)
    {
    get_reply(s);

    sleep(1);
    }

    z_close(z_move(s));
    return 0;
}