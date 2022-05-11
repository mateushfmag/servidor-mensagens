#include "common.h"
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>

void myError(const char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

int digits_only(const char *s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return 0;
    }

    return 1;
}

int addrParse(const char *addrStr, const char *portStr, struct sockaddr_storage *storage){
    if(addrStr == NULL || portStr == NULL){
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portStr);

    if(port == 0){
        return -1;
    }

    port = htons(port);

    struct in_addr inaddr4;// 32-bit IP address
    if(inet_pton(AF_INET,addrStr,&inaddr4)){    
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4-> sin_addr = inaddr4;
        return 0;
    }


    struct in6_addr inaddr6;// 128-bit ipv6 address
    if(inet_pton(AF_INET6,addrStr,&inaddr6)){
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        // addr6->sin6_addr = inaddr6;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;

}

void add2str(const struct sockaddr *addr, char *str, size_t strsize){

    int version;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if(addr->sa_family == AF_INET){
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if(!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN+1)){
            myError("ntop");
        }
        port = ntohs(addr4->sin_port); // network to host short
    }else if(addr->sa_family == AF_INET6){
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if(!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN+1)){
            myError("ntop");
        }
        port = ntohs(addr6->sin6_port); // network to host short
    }else{
        myError("unknown protocol family");
    }

   if(str){
       snprintf(str,strsize,"IPv%d %s %hu", version,addrstr,port);
   }

}

int server_sockaddr_init(const char *proto, const char *portStr, struct sockaddr_storage *storage){

    uint16_t port = (uint16_t)atoi(portStr);

    if(port == 0){
        return -1;
    }

    port = htons(port);

    memset(storage,0,sizeof(*storage));
    if(strcmp(proto,"v4") == 0){
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4-> sin_addr.s_addr = INADDR_ANY;
        return 0;
    }else if(strcmp(proto,"v6") == 0){
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        addr6-> sin6_addr = in6addr_any;
        return 0;
    }else{
        return -1;
    }

}