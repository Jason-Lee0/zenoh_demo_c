#include "scout_eth.h"

char mac_address[MAX_MAC_LENGTH];
char ipv4_address[IPV4_LENGTH];


void get_mac_address(const char *ethername, char *mac_address) {
    FILE *fp;
    char path[MAX_PATH_LENGTH];

    snprintf(path, sizeof(path), "/sys/class/net/%s/address", ethername);

    
    fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error opening file: %s\n", path);
        exit(EXIT_FAILURE);
    }

   
    if (fgets(mac_address, MAX_MAC_LENGTH, fp) == NULL) {
        fprintf(stderr, "Error reading MAC address from file: %s\n", path);
        exit(EXIT_FAILURE);
    }

    
    mac_address[strcspn(mac_address, "\n")] = '\0';
    fclose(fp);
}

int find_ipv4_address(const char *ethername)
{

    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char ipv4_host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        family = ifa->ifa_addr->sa_family;
            if ((family == AF_INET) && strcmp(ifa->ifa_name, ethername) == 0) {
                
                
                if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ipv4_host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) != 0) {
                    fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
                    return -1;
                }

                strcpy(ipv4_address,ipv4_host);
                printf("IPv4_address : %s\n", ipv4_address);
                
                return 0;

            }   

    }


    
    fprintf(stderr, "%s not found ipv4 address \n", ethername);
    freeifaddrs(ifaddr);
    return -1;
}

int find_mac_address(const char *ipv6_address) {

    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char ipv6_host[NI_MAXHOST];
    char ipv4_host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        family = ifa->ifa_addr->sa_family;
            if (family == AF_INET6) {
                
                if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), ipv6_host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) != 0) {
                    fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
                    return -1;
                }
                char *percent_sign = strchr(ipv6_host, '%');
                if (percent_sign != NULL) {
                    *percent_sign = '\0';  
                    }

                // check the match of ipv6 address  
                if (strcmp(ipv6_host, ipv6_address) == 0) {

                    get_mac_address(ifa->ifa_name,mac_address);
                    find_ipv4_address(ifa->ifa_name);

                    return 0;
                }
            }   
    }
    fprintf(stderr, "IPv6 address %s not found\n", ipv6_address);
    freeifaddrs(ifaddr);
    return -1;
}

int decode_scout_message(FILE *stream, const z_str_array_t *locs) {

    
    char *start = strstr(locs->val[0], "["); 
    if (start != NULL) {
        start++; 
        char *end = strstr(start, "]"); 
        if (end != NULL) {
            char ipv6[INET6_ADDRSTRLEN];

            size_t length = end - start; 
            if (length <= INET6_ADDRSTRLEN) {
                strncpy(ipv6, start, length);   //obtain ipv6 address 
                ipv6[length] = '\0'; 
                printf("IPv6 address: %s\n", ipv6);
                if (find_mac_address(ipv6) != 0)
                {
                    return -1;
                }
                else
                {
                    printf("mac_address : %s\n",mac_address);
                    return 0;
                }
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

void scout_callback(z_owned_hello_t *hello, void *context, void *find) {

    if ((*(int *)context)==1){return;}
    z_hello_t lhello = z_loan(*hello);
    if (decode_scout_message(stdout, &lhello.locators) != 0)
    {
        printf("Not local address, check next session!!\n");
    }
    else {(*(int *)context)++; }

}

void drop(void *context, void *find) {
    printf("\n");
    int count = *(int *)context;
    free(context);
    if (!count) {
        printf("Did not find any zenoh process.\n");
    }

}

addr_config scout_eth(){
    
    addr_config config;
    int *context = malloc(sizeof(int));
    *context = 0;

    z_owned_scouting_config_t scout_config = z_scouting_config_default();
    z_owned_closure_hello_t closure = z_closure(scout_callback, drop, context);
    z_scout(z_move(scout_config), z_move(closure)) ;

    config.ipv4_address = ipv4_address;
    config.mac_address = mac_address;

    return config;

}