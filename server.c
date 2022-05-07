#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFSIZE 1024



void usageTerms(int argc, char **argv){
    printf("usage: %s <v4|v6> <server_port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}


int main(int argc, char **argv){
    if(argc < 3){
        usageTerms(argc,argv);
    }



    struct sockaddr_storage storage;

    if(server_sockaddr_init(argv[1], argv[2],&storage) != 0){
        usageTerms(argc,argv);
    }



    int mySocket;
    mySocket = socket(storage.ss_family,SOCK_STREAM, 0);
    if(mySocket == -1){
        myError("socket error");
    }

    int enable = 1;
    if(setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR,&enable,sizeof(int)) != 0){
        myError("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);

    if(bind(mySocket, addr, sizeof(storage)) != 0){
        myError("bind");
    }


    if(listen(mySocket, 10) != 0){
        myError("listen");
    }

    char addrstr[BUFFSIZE];
    add2str(addr,addrstr, BUFFSIZE);
    printf("bound to %s, waiting connection\n", addrstr);

    while(1){

        struct sockaddr_storage clientStorage;
        struct sockaddr *clientAddr = (struct sockaddr *)&clientStorage;

        socklen_t clientAddrLen = sizeof(clientStorage);


        int clientSocket = accept(mySocket, clientAddr, &clientAddrLen);

        if(clientSocket == -1){
            myError("accept");
        }
        
        char clientAddrStr[BUFFSIZE];
        add2str(clientAddr,clientAddrStr, BUFFSIZE);
        printf("[log] connection from %s\n", clientAddrStr);

        char buf[BUFFSIZE];
        size_t count = recv(clientSocket, buf,BUFFSIZE,0);
        memset(buf,0,BUFFSIZE);
        printf("[msg] %s, %d bytes: %s\n", clientAddrStr, (int)count, buf);

        sprintf(buf,"remote endpoint: %.1000s\n", clientAddrStr);
        count = send(clientSocket, buf, strlen(buf)+1,0);
        if(count != strlen(buf)+1){
            myError("send");
        }
        close(clientSocket);

    }


}