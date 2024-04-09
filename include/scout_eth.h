#include "zenoh.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>

#define IPV4_LENGTH 15
#define MAX_MAC_LENGTH 18
#define MAX_PATH_LENGTH 256

typedef struct 
{
    char* mac_address;
    char* ipv4_address;
}addr_config;

void get_mac_address(const char* , char *);
int find_ipv4_address(const char *);
int find_mac_address(const char *);
int decode_scout_message(FILE *, const z_str_array_t *);
void scout_callback(z_owned_hello_t *, void *, void *);
void drop(void *, void *);
addr_config scout_eth( );

