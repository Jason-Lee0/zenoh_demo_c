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
#include <time.h>
#include "scout_eth.h"
#include "zenoh.h"

#define N 50000

typedef struct {
    volatile unsigned long count;
    volatile unsigned long finished_rounds;
    struct timespec start;
    struct timespec first_start;
} z_stats_t;
z_stats_t *stats;

z_stats_t *z_stats_make() {
    z_stats_t *stats = malloc(sizeof(z_stats_t));
    stats->count = 0;
    stats->finished_rounds = 0;
    stats->first_start.tv_nsec = 0;
    return stats;
}

static inline double get_elapsed_s(const struct timespec *start, const struct timespec *end) {
    return (double)(end->tv_sec - start->tv_sec) + (double)(end->tv_nsec - start->tv_nsec) / 1.0E9;
}

double elapsed_s;
int connection = 0;

void on_sample(const z_sample_t *sample, void *context) {
    stats = (z_stats_t *)context;
    if (stats->count == 0) {
        clock_gettime(CLOCK_MONOTONIC, &stats->start);
        if (stats->first_start.tv_nsec == 0) {
            connection = 1;
            stats->first_start = stats->start;

        }
        stats->count++;
    } else if (stats->count < N) {
        stats->count++;
    } else {
        struct timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);
        stats->finished_rounds++;
        printf("%f msg/s\n", (double)N / get_elapsed_s(&stats->start, &end));
        stats->count = 0;
    }
}
void drop_stats(void *context) {
    const z_stats_t *stats = (z_stats_t *)context;
    const unsigned long sent_messages = N * stats->finished_rounds + stats->count;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    printf("Stats being dropped after unsubscribing: sent %ld messages over %f seconds (%f ms/msg)\n", sent_messages,
           elapsed_s, (double) elapsed_s*1000/sent_messages);
    free(context);
}

int main(int argc, char **argv) {
    z_owned_config_t config = z_config_default();
    if (argc > 1) {
        if (zc_config_insert_json(z_loan(config), Z_CONFIG_CONNECT_KEY, argv[1]) < 0) {
            printf(
                "Couldn't insert value `%s` in configuration at `%s`. This is likely because `%s` expects a "
                "JSON-serialized list of strings\n",
                argv[1], Z_CONFIG_CONNECT_KEY, Z_CONFIG_CONNECT_KEY);
            exit(-1);
        }
    }

    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }

    addr_config config_eth = scout_eth();

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

    z_owned_keyexpr_t ke = z_declare_keyexpr(z_loan(s), z_keyexpr("test/thr"));

    z_stats_t *context = z_stats_make();
    z_owned_closure_sample_t callback = z_closure(on_sample, drop_stats, context);
    z_owned_subscriber_t sub = z_declare_subscriber(z_loan(s), z_loan(ke), z_move(callback), NULL);
    if (!z_check(sub)) {
        printf("Unable to create subscriber.\n");
        exit(-1);
    }

    char c = 0;
    while (true){
    if (connection == 1){
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_s = get_elapsed_s(&stats->first_start, &end);
    if (elapsed_s > 100.0){break;}
    }


    }
    z_undeclare_subscriber(z_move(sub));
    z_undeclare_keyexpr(z_loan(s), z_move(ke));
    z_close(z_move(s));
    return 0;
}
