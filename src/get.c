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

#include "zenoh.h"

#include <time.h>
#include <sys/time.h>

long long current_time_millis() {
    struct timeval te;
    gettimeofday(&te, NULL);
    return te.tv_sec * 1000LL + te.tv_usec / 1000;
}
long long current_time_nanosec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

int main(int argc, char **argv) {
    char *expr = "device_command/123";
    char *value = NULL;
    int len = 128;
    switch (argc) {
        default:
        case 3:
            value = argv[2];
        case 2:
            len = atoi(argv[1]);
            printf("change len : 128 -> %d \n",len);
            break;
        case 1:
            // Do nothing
            break;
    }
    z_keyexpr_t keyexpr = z_keyexpr(expr);
    if (!z_check(keyexpr)) {
        printf("%s is not a valid key expression", expr);
        exit(-1);
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
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    
    char *value_test = (char *)malloc(len+1);

    memset(value_test, '1', len);
    value_test[len]='\0';

    printf("Sending Query '%s'...\n", expr);
    z_owned_reply_channel_t channel = zc_reply_fifo_new(16);
    z_get_options_t opts = z_get_options_default();

    //opts.value.payload = (z_bytes_t){.len = strlen(value_test), .start = (uint8_t *)value_test};


    double time_sum ;


    for (int i = 0; i < 1000 ; i++){ 
        channel = zc_reply_fifo_new(16);
   

        long long time_start = current_time_nanosec();
        z_get(z_loan(s), keyexpr,(char *)value_test, z_move(channel.send),
            &opts);  // here, the send is moved and will be dropped by zenoh when adequate
        z_owned_reply_t reply = z_reply_null();
        for (z_call(channel.recv, &reply); z_check(reply); z_call(channel.recv, &reply)) {
            if (z_reply_is_ok(&reply)) {
              long long time_end = current_time_nanosec();
              time_sum+= (time_end-time_start)/ 1000000000.0;
             
            } else {
                printf("Received an error\n");
            }
        }
        z_drop(z_move(reply));
        z_drop(z_move(channel));
        
    }
    printf( " average time : %f \n",time_sum / 1000.0);


    z_close(z_move(s));
    return 0;
}