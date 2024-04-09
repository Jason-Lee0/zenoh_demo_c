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

typedef struct {
    char id[100];          // 键
    long int timer;   // 值
    UT_hash_handle hh;  // 用于管理哈希表中的元素
} MyData;

MyData *device_map= NULL;  // 哈希表

// 插入新的键值对到哈希表中
void add_user(char *id, long int timer ) {
    MyData *s = (MyData*)malloc(sizeof(MyData));
    strncpy(s->id, id, sizeof(s->id) - 1);  // 复制 id 到 s->id 中
    //s->id[sizeof(s->id) - 1] = '\0';  // 确保字符串以 null 结尾
    s->timer = timer;  // 将 timer 赋值给 s->timer
    HASH_ADD_STR(device_map, id, s);  // 使用 s->id 作为键，添加到哈希表中
}

// 根据键查找值
MyData *find_user(char *id) {
    MyData *s;
    HASH_FIND_STR(device_map, id, s);  // 根据 id 查找元素
    return s;
}

// 删除指定键的元素
// void delete_user(MyData *user) {
//     HASH_DEL(users, user);  // 从哈希表中删除指定的元素
//     free(user);             // 释放内存
// }
void clearScreen() {
    // 根据操作系统选择适当的清屏命令
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}
void print_all_keys() {
    MyData *s, *tmp;

    while(true){
    time_t time_check = time(NULL);


    HASH_ITER(hh, device_map, s, tmp) {
        if ((time_check - s->timer) > 3 )
        {
            printf("%s offline (connect lost > 3s) \n", s->id);
        }
        else
        {
            printf("%s online\n", s->id);
        }
    }

    sleep(1);
    clearScreen();
    }

}
void update_device_info(z_sample_t sample  )
{

    memcpy(payload, sample.payload.start, sample.payload.len);
    payload[sample.payload.len] = '\0'; // 添加字符串结束符
    time_t t;


    t = time(NULL);
    MyData *device = find_user(payload);
    if(device != NULL)
    {
        
        device->timer = t;
    }
    else{
        printf("found new device , add : %s\n",payload);
        add_user(payload,t);
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

    printf("Sending Query '%s'...\n", expr);
    z_owned_reply_channel_t channel = zc_reply_fifo_new(16);
    z_get_options_t opts = z_get_options_default();
    if (value != NULL) {
        opts.value.payload = (z_bytes_t){.len = strlen(value), .start = (uint8_t *)value};
    }
    if (pthread_create(&thread,NULL, print_all_keys, NULL) != 0) {
        fprintf(stderr, "Error creating thread\n");
        return EXIT_FAILURE;
    }

// 将示例数据的 payload 复制到字符数组中

    while(true)
    {
    z_owned_reply_channel_t channel = zc_reply_fifo_new(16);
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
    sleep(1);
    }

    z_close(z_move(s));
    return 0;
}