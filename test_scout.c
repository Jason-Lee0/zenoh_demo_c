
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
#include <time.h>
#include <sys/time.h>

#define MAX_PATH_LENGTH 256
#define MAX_MAC_LENGTH 18
#define IPV4_LENGTH 15

//pub_test
#define MAX_COLS 4
#define MAX_ROWS 10000
#define MAX_LENGTH 10000
#define DATA_SIZE 4
#define AXIS_NUM 128

char ***csv_data;

char mac_address[MAX_MAC_LENGTH];
char ipv4_address[IPV4_LENGTH];

char *expr = "device_config/";
const char *value = mac_address;
z_keyexpr_t key;
char mac_address[MAX_MAC_LENGTH];

char* keyexpr = "device/motor_status/";
int data_size = 4;
int axis_num = 4;

bool connection = false;

typedef struct {
    z_owned_session_t session;
} ThreadArgs;

long long current_time_millis() {
    struct timeval te;
    gettimeofday(&te, NULL);
    return te.tv_sec * 1000LL + te.tv_usec / 1000;
}

char* generate_new_key(int axis_num, const char* mac_address, const char* keyexpr) {
    
    int str_len = snprintf(NULL, 0, "%d", axis_num) + 1;

    char* axis = (char *)malloc(str_len * sizeof(char));

    if (axis == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    snprintf(axis, str_len, "%d", axis_num);
    size_t key_len = strlen(keyexpr);
    size_t mac_len = strlen(mac_address);


    char* new_key = malloc(key_len + mac_len + str_len + 2); 
    if (new_key == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(axis);
        return NULL;
    }

    strcpy(new_key, keyexpr);
    strcat(new_key, mac_address);
    strcat(new_key, "_");
    strcat(new_key, axis);

    free(axis);

    return new_key;
}


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
                //printf("IPv4_address : %s\n", ipv4_address);
                
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
                //printf("IPv6 address: %s\n", ipv6);
                if (find_mac_address(ipv6) != 0)
                {
                    return -1;
                }
                else
                {
                    //printf("mac_address : %s\n",mac_address);
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
    else {
        (*(int *)context)++; 
        connection = true;}

}

void drop(void *context, void *find) {
    printf("\n");
    int count = *(int *)context;
    free(context);
    if (!count) {
        printf("Did not find any zenoh process.\n");
    }

}



void scout(void *args){

    int *context = malloc(sizeof(int));
    *context = 0;


    z_owned_scouting_config_t scout_config = z_scouting_config_default();
    z_owned_closure_hello_t closure = z_closure(scout_callback, drop, context);
    z_scout(z_move(scout_config), z_move(closure)) ;
    
    
}

int main(int argc, char **argv) {

    int repeat_time = 5;
    
    if (argv[1])
    {
        repeat_time = atoi(argv[1]);
        printf("change axis_num\n");
    }



     double session1_sum , scout_sum, session2_sum , total_time ;


    int repeat_start = 0;
    while (repeat_start < repeat_time ){
        printf("repeat_time : %d", repeat_start);
    
        long long session1_time_start = current_time_millis();
        z_owned_config_t config = z_config_default();

        //printf("Opening session...\n");
        z_owned_session_t s = z_open(z_move(config));
        if (!z_check(s)) {
            printf("Unable to open session!\n");
            exit(-1);
        }
        long long session1_time_end = current_time_millis();

        thrd_t thread;

        //printf("Scouting...\n");
        
        long long scout_time_start = current_time_millis();
        //open scout thread 
        if (thrd_create(&thread, scout, NULL) != thrd_success) {
        fprintf(stderr, "Error creating thread\n");
        return EXIT_FAILURE;
        }
        thrd_join(thread, NULL);

        long long scout_time_end = current_time_millis();

        if (connection == false)
        {
            printf("Could not find any Local session to find ip address. \n");
            exit(-1);
        }

        //printf("\nClose current session. Reopen Session by using Link-local IPv4 address.\n");
        

        z_close(z_move(s));

        long long session2_start = current_time_millis();
        config = z_config_default();


        char listen_port[50];
        sprintf(listen_port, "[\"tcp/%s:7777\"]",ipv4_address);


        //printf("add listen_port : %s \n", listen_port);

        if (zc_config_insert_json(z_loan(config), Z_CONFIG_LISTEN_KEY, listen_port) < 0) {
                printf(
                    "Couldn't insert value  in configuration at `%s`. This is likely because `%s` expects a "
                    "JSON-serialized list of strings\n",
                    Z_CONFIG_LISTEN_KEY, Z_CONFIG_LISTEN_KEY);
                exit(-1);
            }

        //printf("Opening session by using Link-local IPv4 address - %s \n\n",ipv4_address);



        s = z_open(z_move(config));
        if (!z_check(s)) {
            printf("Unable to open session!\n");
            exit(-1);
        }
        long long session2_end = current_time_millis();


        double session1_time_spent = (session1_time_end - session1_time_start) / 1000.0;
        double scout_time_spent = (scout_time_end - scout_time_start)/1000.0;
        double session2_time_spent = (session2_end - session2_start)/1000.0;\
        double total_spent = (session2_end-session1_time_start)/1000.0;



        //printf("Time spent: %f  seconds\n\n", time_spent);

        z_close(z_move(s));

        
        session1_sum += session1_time_spent;
        session2_sum += session2_time_spent;
        scout_sum += scout_time_spent;
        total_time += total_spent;
        repeat_start++;


    }

    

   

    printf(" session1_Average : %f \n", session1_sum / repeat_time);
    printf(" session2_Average : %f \n", session2_sum / repeat_time);
    printf(" scout_Average : %f \n", scout_sum / repeat_time);
    printf(" total_Average : %f \n", total_time / repeat_time);

}