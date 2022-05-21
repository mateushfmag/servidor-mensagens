#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define BUFFSIZE 1024
#define NUMBER_OF_SENSORS 4
#define MAX_SENSORS 15

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
    int validCommand = 1;
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
                validCommand = 0;
                break;
                // myError("TODO: HANDLE ERROR COMMAND NOT FOUND\n");
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
                    while (query.sensorIds[idx] != 0)
                    {
                        ++idx;
                    }
                    if (idx < arraySize)
                    {
                        query.sensorIds[idx] = clientMessage;
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

    if (validCommand)
    {
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
    }
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

/**
 * result[0] == 0 -> no errors, result[1,n] -> values
 * result[0] == 1 -> has errors, result[1,n] -> not installed sensors
 */
FloatArray getSensorValue(Query query)
{
    FILE *file = getFile("equipments", "r");
    char buffer[BUFFSIZE];
    FloatArray sensors;
    initFloatArray(&sensors);
    FloatArray notInstalledSensors;
    initFloatArray(&notInstalledSensors);

    while (fscanf(file, "%s", buffer) != EOF)
    {
        if (isEquipmentEqual(buffer, query.targetEquipment))
        {
            int beginOfSensorsIndex = 2;
            for (int i = 0; i < len(query.sensorIds); i++)
            {
                if (query.sensorIds[i])
                {
                    int sensorId = atoi(query.sensorIds[i]);
                    if (buffer[beginOfSensorsIndex + (sensorId - 1)] == '1')
                    {
                        float random = (rand() % 1000) + 1;
                        float sensorValue = random / 100;
                        appendToFloatArray(&sensors, sensorValue); // TODO: random number
                    }
                    else
                    {
                        appendToFloatArray(&notInstalledSensors, sensorId);
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }

    closeFile(file);

    if (notInstalledSensors.used == 0)
    {
        prependToFloatArray(&sensors, 0);
        return sensors;
    }
    else
    {
        prependToFloatArray(&notInstalledSensors, 1);
        return notInstalledSensors;
    }
}

/**
 * int[query.sensorIds.length]
 * 1 = success
 * -1 = ADD = already exists | REMOVE = does not exist
 * -2 = ADD = limit exceeded
 * -3 = invalid sensor
 * */
IntArray toggleSensor(Query query, char value)
{
    IntArray sensors;
    initIntArray(&sensors);

    int areSensorsValid = 0;
    int amountOfSensors = 0;
    int sensorIdsTrueSize = 0;
    int beginOfSensorsIndex = 2;

    for (int i = 0; i < len(query.sensorIds); i++)
    {
        if (query.sensorIds[i])
        {
            int compare[] = {
                strcmp(query.sensorIds[i], "01"),
                strcmp(query.sensorIds[i], "02"),
                strcmp(query.sensorIds[i], "03"),
                strcmp(query.sensorIds[i], "04"),
            };
            for (int j = 0; j < len(compare); j++)
            {
                if (compare[j] == 0)
                {
                    areSensorsValid = 1;
                    break;
                }
            }

            if (areSensorsValid == 0)
            {
                break;
            }

            ++sensorIdsTrueSize;
        }
        else
        {
            break;
        }
    }

    if (areSensorsValid == 0)
    {
        appendToIntArray(&sensors, -3);
        return sensors;
    }

    FILE *file = getFile("equipments", "r");
    FILE *temp = getFile("temp", "w");
    char buffer[BUFFSIZE];

    /* gets amountOfSensors */
    while (fscanf(file, "%s", buffer) != EOF)
    {
        for (int i = 0; i < NUMBER_OF_SENSORS; i++)
        {
            if (buffer[beginOfSensorsIndex + i] == '1')
            {
                ++amountOfSensors;
            }
        }
    }
    closeFile(file);
    file = getFile("equipments", "r");
    memset(buffer, 0, BUFFSIZE);
    /* gets amountOfSensors */

    /* create sensors array from file */
    while (fscanf(file, "%s", buffer) != EOF)
    {
        if (isEquipmentEqual(buffer, query.targetEquipment))
        {
            for (int i = 0; i < sensorIdsTrueSize; i++)
            {
                if (query.sensorIds[i] && (amountOfSensors < 15 || value == '0'))
                {
                    ++amountOfSensors;
                    int sensorIndex = atoi(query.sensorIds[i]) - 1;

                    if (buffer[beginOfSensorsIndex + sensorIndex] == value)
                    {
                        appendToIntArray(&sensors, -1);
                    }
                    else
                    {
                        appendToIntArray(&sensors, 1);
                    }
                    buffer[beginOfSensorsIndex + sensorIndex] = value;
                }
            }
        }
        fprintf(temp, "%s\n", buffer);
    }
    closeFile(temp);
    closeFile(file);
    /* create sensors array from file */

    temp = getFile("temp", "r");
    file = getFile("equipments", "w");

    /* wirte sensors to file */
    while (fscanf(temp, "%s", buffer) != EOF)
    {
        fprintf(file, "%s\n", buffer);
    }
    closeFile(temp);
    remove("temp");
    closeFile(file);
    /* wirte sensors to file */

    // check errors
    if (value == '1' && amountOfSensors >= MAX_SENSORS)
    {
        prependToIntArray(&sensors, -2);
    }
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

    if (result.array[0] == -2)
    {
        concatCharArray(&feedback, "limit exceeded\n");
        return feedback;
    }
    else if (result.array[0] == -3)
    {
        concatCharArray(&feedback, "invalid sensor\n");
        return feedback;
    }

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
        concatCharArray(&feedback, "none\n");
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
    if (result.array[0] == -3)
    {
        concatCharArray(&feedback, "invalid sensor\n");
        return feedback;
    }
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

CharArray readCommandFeedback(Query query, FloatArray result)
{

    CharArray feedback;
    initCharArray(&feedback);
    if (result.array[0] == 0.0)
    { // no errors
        char buffer[320];
        for (int i = 1; i < result.size; i++)
        {
            memset(buffer, 0, 320);
            sprintf(buffer, "%.2f", result.array[i]);
            concatCharArray(&feedback, buffer);
            appendToCharArray(&feedback, ' ');
        }
        feedback.array[feedback.size - 1] = '\n';
    }
    else
    { // has errors
        concatCharArray(&feedback, "sensors(s) ");
        for (int i = 1; i < result.size; i++)
        {
            appendToCharArray(&feedback, '0');
            appendToCharArray(&feedback, '0' + result.array[i]);
            appendToCharArray(&feedback, ' ');
        }
        concatCharArray(&feedback, "not installed\n");
    }
    return feedback;
}

int main(int argc, char **argv)
{
    srand(time(0));
    int serverSocket = initServerSocket(argc, argv);

    while (1)
    {

        int clientSocket = initClientSocket(serverSocket);
        char buf[BUFFSIZE];
        int stop = 0;
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

            // if (strncmp(clientMessage, "kill", 4) == 0)
            if (!query.command)
            {
                stop = 1;
                mySuccess("INVALID COMMAND\n", clientSocket);
                break;
            }

            CharArray feedback;
            if (strcmp(query.command, "add") == 0)
            {
                printf("CALLING ADD SENSOR\n");
                IntArray sensors = toggleSensor(query, '1');
                feedback = addCommandFeedback(query, sensors);
            }
            else if (strcmp(query.command, "list") == 0)
            {
                printf("CALLING LIST SENSOR\n");
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
                FloatArray sensors = getSensorValue(query);
                feedback = readCommandFeedback(query, sensors);
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

        if (!stop)
        {
            // sprintf(buf, "remote endpoint: %.1000s\n", clientAddrStr);
            count = send(clientSocket, buf, strlen(buf) + 1, 0);
            if (count != strlen(buf) + 1)
            {
                myError("send");
            }
            close(clientSocket);
        }
    }
}
