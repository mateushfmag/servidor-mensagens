#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFSIZE 1024

void usageTerms(int argc, char **argv){
    printf("usage: %s", "<server_ip> <server_port>\n",argv[0]);
    printf("example: %s", "127.0.0.1 51511\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv){
    if(argc < 3){
        usageTerms(argc, argv);
    }


    struct sockaddr_storage storage;

    if(addrParse(argv[1], argv[2],&storage) != 0){
        usageTerms(argc,argv);
    }



    int mySocket;
    mySocket = socket(storage.ss_family,SOCK_STREAM, 0);
    if(mySocket == -1){
        myError("socket error");
    }


    struct sockaddr *addr = (struct sockaddr *)(&storage);


    if(connect(mySocket, addr, sizeof(storage)) != 0 ){
        myError("connect error");
    }

    char addrstr[BUFFSIZE];
    add2str(addr,addrstr, BUFFSIZE);

    printf("connected to %s\n", addrstr);
    char buf[BUFFSIZE];
    memset(buf,0,BUFFSIZE);
    printf("mensagem > ");
    fgets(buf,BUFFSIZE-1,stdin);
    size_t count = send(mySocket,buf,strln(buf)+1,0);
    if(count != strln(buf)+1){
        myError("communication error");
    }

    memset(buf,0,BUFFSIZE);

    unsigned total = 0;

    while(1){
        count = recv(mySocket, buf + total, BUFFSIZE - total,0);
        if(count == -1 || count == 0){
            break;
        }
        total += count;
    }

    printf("Received %d bytes\n",total);

    close(mySocket);

    exit(EXIT_SUCCESS);

}