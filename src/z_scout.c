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

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#define sleep(x) Sleep(x * 1000)
#else
#include <unistd.h>
#endif

#include "zenoh.h"

void fprintpid(FILE *stream, z_id_t pid) {
    int len = 0;
    for (int i = 0; i < 16; i++) {
        if (pid.id[i]) {
            len = i + 1;
        }
    }
    if (!len) {
        fprintf(stream, "None");
    } else {
        fprintf(stream, "Some(");
        for (unsigned int i = 0; i < len; i++) {
            fprintf(stream, "%02X", (int)pid.id[i]);
        }
        fprintf(stream, ")");
    }
}

void fprintwhatami(FILE *stream, unsigned int whatami) {
    if (whatami == Z_ROUTER) {
        fprintf(stream, "\"Router\"");
    } else if (whatami == Z_PEER) {
        fprintf(stream, "\"Peer\"");
    } else {
        fprintf(stream, "\"Other\"");
    }
}

void fprintlocators(FILE *stream, const z_str_array_t *locs) {
    fprintf(stream, "[");
    for (unsigned int i = 0; i < locs->len; i++) {
        fprintf(stream, "\"");
        fprintf(stream, "%s", locs->val[i]);
        fprintf(stream, "\"");
        if (i < locs->len - 1) {
            fprintf(stream, ", ");
        }
    }
    fprintf(stream, "]");
}

void fprinthello(FILE *stream, const z_hello_t hello) {
    fprintf(stream, "Hello { pid: ");
    fprintpid(stream, hello.pid);
    fprintf(stream, ", whatami: ");
    fprintwhatami(stream, hello.whatami);
    fprintf(stream, ", locators: ");
    fprintlocators(stream, &hello.locators);
    fprintf(stream, " }");
}

void callback(z_owned_hello_t *hello, void *context) {
    z_hello_t lhello = z_loan(*hello);
    fprinthello(stdout, lhello);
    fprintf(stdout, "\n");
    (*(int *)context)++;
}

void drop1(void *context) {
    printf("Dropping scout\n");
    int count = *(int *)context;
    free(context);
    if (!count) {
        printf("Did not find any zenoh process.\n");
    }
}

int main(int argc, char **argv) {
    int *context = malloc(sizeof(int));
    *context = 0;

     z_owned_config_t config = z_config_default();



    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    
    z_owned_scouting_config_t scout_config = z_scouting_config_default();
    z_owned_closure_hello_t closure = z_closure(callback, drop1, context);
    printf("Scouting...\n");
    z_scout(z_move(scout_config), z_move(closure));
    sleep(1);
    return 0;
}