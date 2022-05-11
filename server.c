#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFSIZE 1024

typedef struct Query
{
    char *command;
    char *sensorsList[BUFFSIZE];
    char *targetEquipment;
} Query;

Query str2query(char *clientMessage)
{
    char *commands[2] = {"add", "list"};
    Query query;
    int index = 0;
    int isSensor = 1;
    while (clientMessage)
    {
        clientMessage[strcspn(clientMessage, "\n")] = 0;
        // verify if first token is a valid command
        if (index == 0)
        {
            int found = 0;
            for (int i = 0; i < len(commands); i++)
            {
                // equal
                if (strcmp(clientMessage, commands[i]) == 0)
                {
                    query.command = commands[i];
                    found = 1;
                }
            }
            if (found == 0)
            {
                printf("TODO: HANDLE ERROR COMMAND NOT FOUND\n");
                exit(EXIT_FAILURE);
            }
        }
        else // other tokens
        {
            if (strcmp(clientMessage, "in") == 0 || strcmp(clientMessage, "at") == 0 || strcmp(clientMessage, "on") == 0)
            {
                isSensor = 0;
            }

            if (digits_only(clientMessage))
            {

                if (isSensor)
                {
                    /**
                     * find last element from array
                     */
                    int idx = 0;
                    size_t arraySize = len(query.sensorsList);
                    while (query.sensorsList[idx] != 0)
                    {
                        ++idx;
                    }
                    if (idx < arraySize)
                    {
                        query.sensorsList[idx] = clientMessage;
                    }
                    /**
                     * find last element from array
                     */
                }
                else
                { // is equipment
                    query.targetEquipment = clientMessage;
                }
            }
        }
        ++index;
        clientMessage = strtok(NULL, " "); // verify next token
    }
    printf("\n[QUERY RESULT]\n\nCommand: %s\nSensors List:", query.command);

    for (int i = 0; i < len(query.sensorsList); i++)
    {
        if (query.sensorsList[i])
        {

            printf(" %s", query.sensorsList[i]);
        }
        else
        {
            break;
        }
    }
    printf("\nTarget Equipment: %s\n\n", query.targetEquipment);
    return query;
}

void usageTerms(int argc, char **argv)
{
    printf("usage: %s <v4|v6> <server_port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int initServerSocket(int argc, char **argv)
{
    if (argc < 3)
    {
        usageTerms(argc, argv);
    }

    struct sockaddr_storage storage;

    if (server_sockaddr_init(argv[1], argv[2], &storage) != 0)
    {
        usageTerms(argc, argv);
    }

    int serverSocket;
    serverSocket = socket(storage.ss_family, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        myError("socket error");
    }

    int enable = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0)
    {
        myError("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);

    if (bind(serverSocket, addr, sizeof(storage)) != 0)
    {
        myError("bind");
    }

    if (listen(serverSocket, 10) != 0)
    {
        myError("listen");
    }
    char addrstr[BUFFSIZE];

    add2str(addr, addrstr, BUFFSIZE);
    printf("bound to %s, waiting connection\n", addrstr);

    return serverSocket;
}

int initClientSocket(int serverSocket)
{
    struct sockaddr_storage clientStorage;
    struct sockaddr *clientAddr = (struct sockaddr *)&clientStorage;
    socklen_t clientAddrLen = sizeof(clientStorage);
    int clientSocket = accept(serverSocket, clientAddr, &clientAddrLen);

    if (clientSocket == -1)
    {
        myError("accept");
    }
    char clientAddrStr[BUFFSIZE];
    add2str(clientAddr, clientAddrStr, BUFFSIZE);
    printf("[log] connection from %s\n", clientAddrStr);
    return clientSocket;
}

int main(int argc, char **argv)
{

    int serverSocket = initServerSocket(argc, argv);

    while (1)
    {

        int clientSocket = initClientSocket(serverSocket);
        char buf[BUFFSIZE];
        size_t count = recv(clientSocket, buf, BUFFSIZE, 0);

        /**
         * one recv call is not enough to get all data sent from send()
         * **/
        unsigned total = 0;
        while (1)
        {
            printf("[msg] %d bytes: %s", (int)count, buf);

            char *clientMessage = strtok(buf, " ");
            Query query = str2query(clientMessage);

            count = recv(clientSocket, buf + total, BUFFSIZE - total, 0);
            if (count == -1 || count == 0)
            {
                break;
            }
            total += count;

            printf("MESSAGE SENT: %s\n", buf);
        }

        printf("END OF LOOP");

        memset(buf, 0, BUFFSIZE);

        // sprintf(buf, "remote endpoint: %.1000s\n", clientAddrStr);
        count = send(clientSocket, buf, strlen(buf) + 1, 0);
        if (count != strlen(buf) + 1)
        {
            myError("send");
        }
        close(clientSocket);
    }
}