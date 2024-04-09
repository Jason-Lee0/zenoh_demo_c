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
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#define sleep(x) Sleep(x * 1000)
#else
#include <unistd.h>
#endif
#include "zenoh.h"
#include "uthash.h"

typedef struct {
    char id[100];
    int axis;          // 键
    char *motor_status;   // 值
    UT_hash_handle hh ;  // 用于管理哈希表中的元素
} MyData;

const char *kind_to_str(z_sample_kind_t kind);

void clearScreen() {
    // 根据操作系统选择适当的清屏命令
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

MyData *device_info= NULL; 

void update_motor_status (const char device_info, const int axis_num,const char* motor_status)
{
    MyData *d = malloc(sizeof(MyData));




}

void data_handler(const z_sample_t *sample) {

    clearScreen();
    z_owned_str_t keystr = z_keyexpr_to_string(sample->keyexpr);

    //printf(">> [Subscriber] Received %s ('%s')\n", kind_to_str(sample->kind), keystr);
    //z_drop(z_move(keystr));

    char *title;

    char *axis;
    
    

    title = strrchr(keystr._cstr, '/');
    title ++;
    

    if (title != NULL) {

        char *axis = strchr(title, '_');
        char device_info[axis - title + 1]; 
        strncpy(device_info, title, axis - title);
        device_info[axis - title] = '\0'; // 添加字符串终止符
        printf("Prefix: %s\n", device_info);
        axis++;
        printf("axis : %s\n", axis);
    }
    else {
        printf("wrong_key_path\n");
        return;
    }
    printf("axis_status \n(position, velocity, motor_status, feedback_torque):\n%.*s \n",(int)sample->payload.len, sample->payload.start);
    




    // int tokens[1001][4]; // 存储分割后的整数子串
    // int count = 0; // 子串计数器

    // char* payload = (char*)malloc(sample->payload.len * sizeof(char)); // 留一个额外的空间用于终止字符
    // memcpy(payload, sample->payload.start, sample->payload.len);
    // payload[sample->payload.len] = '\0';



    // char *token = payload;
    // while (sscanf(token, " [ %d , %d , %d , %d ] ,", &tokens[count][0], &tokens[count][1], &tokens[count][2], &tokens[count][3]) == 4) {
    //     count++;
    //     token = strchr(token, ']') + 3; // 定位到下一个子串的起始位置
    // }

    // // 打印分割后的整数子串
    // printf("title: %s\n",title_key);
    // for (int i = 0; i < count; ++i) {
    //     printf("[ %d , %d , %d , %d ]\n", tokens[i][0], tokens[i][1], tokens[i][2], tokens[i][3]);
    // }

    // free(payload);
    

}

int main(int argc, char **argv) {



    char *expr = "device/motor_status/**";

    if (argc > 1) {
        expr = argv[1];
    }

   // z_owned_config_t config = z_config_default();
    // if (argc > 2) {
    //     if (zc_config_insert_json(z_loan(config), Z_CONFIG_LISTEN_KEY, argv[2]) < 0) {
    //         printf(
    //             "Couldn't insert value `%s` in configuration at `%s`. This is likely because `%s` expects a "
    //             "JSON-serialized list of strings\n",
    //             argv[2], Z_CONFIG_LISTEN_KEY, Z_CONFIG_LISTEN_KEY);
    //         exit(-1);
    //     }
    // }
    // // A probing procedure for shared memory is performed upon session opening. To enable `z_pub_shm` to operate
    // // over shared memory (and to not fallback on network mode), shared memory needs to be enabled also on the
    // // subscriber side. By doing so, the probing procedure will succeed and shared memory will operate as expected.
    // if (zc_config_insert_json(z_loan(config), "transport/shared_memory/enabled", "true") < 0) {
    //     printf("Error enabling Shared Memory");
    //     exit(-1);
    // }

    printf("Opening session...\n");

    z_owned_config_t config = z_config_default() ;
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }

    z_owned_closure_sample_t callback = z_closure(data_handler);
    printf("Declaring Subscriber on '%s'...\n", expr);
    z_owned_subscriber_t sub = z_declare_subscriber(z_loan(s), z_keyexpr(expr), z_move(callback), NULL);
    if (!z_check(sub)) {
        printf("Unable to declare subscriber.\n");
        exit(-1);
    }

   
    printf("Enter 'q' to quit...\n");
    char c = 0;
    while (c != 'q') {
        c = getchar();
        if (c == -1) {
            sleep(1);
        }
    }

    z_undeclare_subscriber(z_move(sub));
    z_close(z_move(s));
    return 0;
}

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
