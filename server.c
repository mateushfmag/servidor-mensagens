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
    char *sensorIds[BUFFSIZE];
    char *targetEquipment;
} Query;

Query str2query(char *clientMessage)
{
    char *commands[4] = {"add", "list", "remove", "read"};
    Query query;
    for (int i = 0; i < len(query.sensorIds); i++)
    {
        query.sensorIds[i] = 0;
    }
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
                myError("TODO: HANDLE ERROR COMMAND NOT FOUND\n");
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
                    size_t arraySize = len(query.sensorIds);
                    printf("arraySize: %ld\n", arraySize);
                    while (query.sensorIds[idx] != 0)
                    {
                        ++idx;
                    }
                    if (idx < arraySize)
                    {
                        query.sensorIds[idx] = clientMessage;
                        printf("sensor added: %s\nindex:%d\n", clientMessage, idx);
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

    for (int i = 0; i < len(query.sensorIds); i++)
    {
        if (query.sensorIds[i])
        {

            printf(" %s", query.sensorIds[i]);
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

int isEquipmentEqual(char *buffer, char *targetEquipment)
{
    char binaryEquipmentId[3] = {buffer[0], buffer[1], '\0'};
    int equipmentId = (int)strtol(binaryEquipmentId, NULL, 2) + 1;
    return equipmentId == atoi(targetEquipment);
}

int *getSensorValue(Query query)
{
    FILE *file = getFile("equipments", "r");
    char buffer[BUFFSIZE];
    int numberOfSensors = 0;
    while (fscanf(file, "%s", buffer) != EOF)
    {
        if (isEquipmentEqual(buffer, query.targetEquipment))
        {
            int beginOfSensorsIndex = 1;
            for (int i = 0; i < len(query.sensorIds); i++)
            {
                if (query.sensorIds[i])
                {
                    int sensorId = atoi(query.sensorIds[i]);
                    if (buffer[beginOfSensorsIndex + sensorId] == '1')
                    {
                        ++numberOfSensors;
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
    int *values;
    values = malloc(sizeof(int) * numberOfSensors);

    for (int i = 0; i < numberOfSensors; i++)
    {
        values[i] = 5; // TODO: random number
    }

    closeFile(file);
    return values;
}

/**
 * int[query.sensorIds.length]
 * 1 = success
 * -1 = ADD = already exists | REMOVE = does not exist
 * */
IntArray toggleSensor(Query query, char value)
{
    FILE *file = getFile("equipments", "r");
    FILE *temp = getFile("temp", "w");
    char buffer[BUFFSIZE];
    IntArray sensors;
    initIntArray(&sensors);

    int sensorIdsTrueSize = 0;
    for (int i = 0; i < len(query.sensorIds); i++)
    {
        if (query.sensorIds[i])
        {
            ++sensorIdsTrueSize;
        }
        else
        {
            break;
        }
    }

    while (fscanf(file, "%s", buffer) != EOF)
    {
        if (isEquipmentEqual(buffer, query.targetEquipment))
        {
            int beginOfSensorsIndex = 1;
            for (int i = 0; i < sensorIdsTrueSize; i++)
            {
                if (query.sensorIds[i])
                {
                    int sensorId = atoi(query.sensorIds[i]);
                    if (buffer[beginOfSensorsIndex + sensorId] == value)
                    {
                        appendToIntArray(&sensors, -1);
                    }
                    else
                    {
                        appendToIntArray(&sensors, 1);
                    }
                    buffer[beginOfSensorsIndex + sensorId] = value;
                }
            }
        }
        fprintf(temp, "%s\n", buffer);
    }
    closeFile(temp);
    closeFile(file);

    temp = getFile("temp", "r");
    file = getFile("equipments", "w");

    while (fscanf(temp, "%s", buffer) != EOF)
    {
        fprintf(file, "%s\n", buffer);
    }

    closeFile(temp);
    remove("temp");
    closeFile(file);
    return sensors;
}

IntArray listSensors(Query query)
{
    FILE *file = getFile("equipments", "r");
    char buffer[BUFFSIZE];
    IntArray sensors;
    initIntArray(&sensors);
    while (fscanf(file, "%s", buffer) != EOF)
    {
        char binaryEquipmentId[3] = {buffer[0],
                                     buffer[1], '\0'};
        int equipmentId = (int)strtol(binaryEquipmentId, NULL, 2) + 1;

        if (equipmentId == atoi(query.targetEquipment))
        {
            int beginOfSensorsIndex = 2;
            for (int i = 0; i < 4; i++)
            {
                if (buffer[i + beginOfSensorsIndex] == '1')
                {
                    appendToIntArray(&sensors, 1);
                }
                else
                {
                    appendToIntArray(&sensors, 0);
                }
            }
        }
    }
    closeFile(file);
    return sensors;
}

CharArray addCommandFeedback(Query query, IntArray result)
{
    CharArray feedback;
    initCharArray(&feedback);
    concatCharArray(&feedback, "sensor ");

    int alreadyExists = 0;
    for (int i = 0; i < result.size; i++)
    {
        if (result.array[i] == -1)
        {
            alreadyExists = 1;
            break;
        }
    }

    if (alreadyExists)
    {

        for (int i = 0; i < result.size; i++)
        {
            if (result.array[i] == -1)
            {
                concatCharArray(&feedback, query.sensorIds[i]);
                appendToCharArray(&feedback, ' ');
            }
        }
        concatCharArray(&feedback, "already exists in ");
        concatCharArray(&feedback, query.targetEquipment);
    }
    else
    {
        printf("resultLength: %ld\n", result.size);
        for (int i = 0; i < result.size; i++)
        {
            printf("sensor ids: %s\n", query.sensorIds[i]);
            concatCharArray(&feedback, query.sensorIds[i]);
            appendToCharArray(&feedback, ' ');
        }
        concatCharArray(&feedback, "added");
    }
    appendToCharArray(&feedback, '\n');
    return feedback;
}

CharArray listCommandFeedback(Query query, IntArray sensors)
{
    CharArray feedback;
    initCharArray(&feedback);
    int operations = 0;
    for (int i = 0; i < sensors.size; i++)
    {
        if (sensors.array[i] == 1)
        {
            appendToCharArray(&feedback, '0');
            appendToCharArray(&feedback, (i + 1) + '0');
            appendToCharArray(&feedback, ' ');
            operations += 3;
        }
    }

    if (operations == 0)
    {
        concatCharArray(&feedback, "no sensors found\n");
    }
    else
    {
        feedback.array[operations - 1] = '\n';
    }
    return feedback;
}

CharArray removeCommandFeedback(Query query, IntArray result)
{
    CharArray feedback;
    initCharArray(&feedback);
    concatCharArray(&feedback, "sensor ");

    int doesNotExist = 0;
    for (int i = 0; i < result.size; i++)
    {
        if (result.array[i] == -1)
        {
            doesNotExist = 1;
            break;
        }
    }

    if (doesNotExist)
    {
        for (int i = 0; i < result.size; i++)
        {
            if (result.array[i] == -1)
            {
                concatCharArray(&feedback, query.sensorIds[i]);
                appendToCharArray(&feedback, ' ');
            }
        }
        concatCharArray(&feedback, "does not exist in ");
        concatCharArray(&feedback, query.targetEquipment);
    }
    else
    {
        for (int i = 0; i < result.size; i++)
        {
            printf("sensor ids: %s\n", query.sensorIds[i]);
            concatCharArray(&feedback, query.sensorIds[i]);
            appendToCharArray(&feedback, ' ');
        }
        concatCharArray(&feedback, "removed");
    }
    appendToCharArray(&feedback, '\n');
    return feedback;
}

int main(int argc, char **argv)
{

    int serverSocket = initServerSocket(argc, argv);

    while (1)
    {

        int clientSocket = initClientSocket(serverSocket);
        char buf[BUFFSIZE];
        /**
         * one recv call is not enough to get all data sent from send()
         * **/
        size_t count = 1;
        while (count > 0)
        {
            memset(buf, 0, BUFFSIZE);
            count = recv(clientSocket, buf, BUFFSIZE, 0);
            if (count <= 0)
            {
                break;
            }
            char *clientMessage = strtok(buf, " ");
            Query query = str2query(clientMessage);
            CharArray feedback;
            if (strcmp(query.command, "add") == 0)
            {
                printf("CALLING ADD SENSOR\n");
                IntArray sensors = toggleSensor(query, '1');
                feedback = addCommandFeedback(query, sensors);
            }
            else if (strcmp(query.command, "list") == 0)
            {
                IntArray sensors = listSensors(query);
                feedback = listCommandFeedback(query, sensors);
            }
            else if (strcmp(query.command, "remove") == 0)
            {
                printf("CALLING REMOVE SENSOR\n");
                IntArray sensors = toggleSensor(query, '0');
                feedback = removeCommandFeedback(query, sensors);
            }
            else if (strcmp(query.command, "read") == 0)
            {
                printf("CALLING GET SENSOR VALUE\n");
                int *values = getSensorValue(query);
                while (*values++)
                {
                    printf("values: %d\n", *(values - 1));
                }
            }
            else
            {
                myError("UNKNOWN COMMAND\n");
            }
            count = send(clientSocket, feedback.array, feedback.size, 0);
            if (count != feedback.size)
            {
                myError("send");
            }
        }
        printf("[msg] %d bytes: %s", (int)count, buf);

        // sprintf(buf, "remote endpoint: %.1000s\n", clientAddrStr);
        count = send(clientSocket, buf, strlen(buf) + 1, 0);
        if (count != strlen(buf) + 1)
        {
            myError("send");
        }
        close(clientSocket);
    }
}
