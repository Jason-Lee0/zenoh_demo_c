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

#define MAX_PATH_LENGTH 256
#define MAX_MAC_LENGTH 18
char mac_address[MAX_MAC_LENGTH];
bool find=false;
char *expr = "device_config/";
const char *value = mac_address;
z_keyexpr_t keyexpr;
char mac_address[MAX_MAC_LENGTH];

void get_mac_address(const char *ethername, char *mac_address) {
    FILE *fp;
    char path[MAX_PATH_LENGTH];

    // 构建文件路径
    snprintf(path, sizeof(path), "/sys/class/net/%s/address", ethername);

    // 打开文件
    fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error opening file: %s\n", path);
        exit(EXIT_FAILURE);
    }

    // 读取 MAC 地址
    if (fgets(mac_address, MAX_MAC_LENGTH, fp) == NULL) {
        fprintf(stderr, "Error reading MAC address from file: %s\n", path);
        exit(EXIT_FAILURE);
    }

    // 去除末尾的换行符
    mac_address[strcspn(mac_address, "\n")] = '\0';

    // 关闭文件
    fclose(fp);
}
int find_mac_address(const char *ipv6_address) {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];


    // 获取系统上所有网络接口的信息
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    // 遍历所有网络接口，找到包含指定 IPv6 地址的网络接口，并获取其 IPv4 地址
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        family = ifa->ifa_addr->sa_family;
        

        // 判断是否是 AF_INET6 地址族，并且是指定的网络接口
        if (family == AF_INET6) {
            
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
                return -1;
            }
             char *percent_sign = strchr(host, '%');
             if (percent_sign != NULL) {
                *percent_sign = '\0';  // 将 '%' 及其后面的部分替换为字符串结束符
                }

            // 检查是否是指定的 IPv6 地址
            if (strcmp(host, ipv6_address) == 0) {

                get_mac_address(ifa->ifa_name,mac_address);

                return 0;
            }
        }
    }

    // 未找到指定的 IPv6 地址对应的 IPv4 地址
    fprintf(stderr, "IPv6 address %s not found\n", ipv6_address);
    freeifaddrs(ifaddr);
    return -1;
}


void fprintlocators(FILE *stream, const z_str_array_t *locs) {
char *start = strstr(locs->val[0], "["); // 查找 '[' 字符的位置
    if (start != NULL) {
        start++; // 移动到 '[' 字符的下一个位置
        char *end = strstr(start, "]"); // 查找 ']' 字符的位置
        if (end != NULL) {
            char ipv6[INET6_ADDRSTRLEN];
            size_t length = end - start; // 计算中间的 IPv6 地址的长度
            if (length <= INET6_ADDRSTRLEN) {
                strncpy(ipv6, start, length); // 将中间的部分复制到 ipv6 变量中
                ipv6[length] = '\0'; // 添加字符串结束符
                printf("IPv6 address: %s\n", ipv6);
                find_mac_address(ipv6);
                printf("mac_address : %s\n",mac_address);
            } else {
                fprintf(stderr, "IPv6 address is too long\n");
            }
        } else {
            fprintf(stderr, "']' character not found\n");
        }
    } else {
        fprintf(stderr, "'[' character not found\n");
    }
}

void fprinthello(FILE *stream, const z_hello_t hello) {

    fprintlocators(stream, &hello.locators);

}

void callback(z_owned_hello_t *hello, void *context) {
    if (*(int *)context == 1){return;}
    z_hello_t lhello = z_loan(*hello);
    fprinthello(stdout, lhello);
    fprintf(stdout, "\n");
    (*(int *)context)++;
}

void drop(void *context) {
    printf("Dropping scout\n");
    int count = *(int *)context;
    free(context);
    if (!count) {
        printf("Did not find any zenoh process.\n");
    }
    else
    {
        find=true;
        printf("find zenoh session!\n");
    }

}

void query_handler(const z_query_t *query, void *context) {
    z_owned_str_t keystr = z_keyexpr_to_string(z_query_keyexpr(query));
    z_bytes_t pred = z_query_parameters(query);
    z_value_t payload_value = z_query_value(query);
    if (payload_value.payload.len > 0) {
        printf(">> [Queryable ] Received Query '%s?%.*s' with value '%.*s'\n", z_loan(keystr), (int)pred.len,
               pred.start, (int)payload_value.payload.len, payload_value.payload.start);
    } else {
        printf(">> [Queryable ] Received Query '%s?%.*s'\n", z_loan(keystr), (int)pred.len, pred.start);
    }
    z_query_reply_options_t options = z_query_reply_options_default();
    options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
    z_query_reply(query, z_keyexpr((const char *)context), (const unsigned char *)value, strlen(value), &options);
    z_drop(z_move(keystr));
}

void scout(void *args){
    int *context = malloc(sizeof(int));
    *context = 0;

    z_owned_scouting_config_t scout_config = z_scouting_config_default();
    z_owned_closure_hello_t closure = z_closure(callback, drop, context);
    z_scout(z_move(scout_config), z_move(closure)) ;
    
}


int main(int argc, char **argv) {
    if (argc > 1) {
        expr = argv[1];
    }
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

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }


    thrd_t thread;

    printf("Scouting...\n");
    


    

    if (thrd_create(&thread, scout, NULL) != thrd_success) {
    fprintf(stderr, "Error creating thread\n");
    return EXIT_FAILURE;
    }
    thrd_join(thread, NULL);

    
    size_t expr_len = strlen(expr);

    size_t mac_len = strlen(mac_address);
    

    // 分配内存空间来存储连接后的字符串




   char *result = (char *)malloc(expr_len + mac_len + 1); // 加 1 是为了存储字符串结束符 '\0'

    if (result == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // 将 "device_config/" 字符串复制到结果字符串中
    strcpy(result, expr);

    // 连接 MAC 地址到结果字符串中
    strcat(result, mac_address);
    
    keyexpr = z_keyexpr(result);
    if (!z_check(keyexpr)) {
        printf("%s is not a valid key expression", result);
        exit(-1);
    }
    
    printf("Declaring Queryable on '%s'...\n", result);
    
    z_owned_closure_query_t callback = z_closure(query_handler, NULL, result);
    z_owned_queryable_t qable = z_declare_queryable(z_loan(s), keyexpr, z_move(callback), NULL);
    if (!z_check(qable)) {
        printf("Unable to create queryable.\n");
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

    z_undeclare_queryable(z_move(qable));

    
    z_close(z_move(s));
    return 0;
}
