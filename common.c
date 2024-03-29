#include "common.h"

void initFloatArray(FloatArray *a)
{
    size_t initialSize = 1;
    a->array = malloc(initialSize * sizeof(float));
    a->used = 0;
    a->size = initialSize;
}

void appendToFloatArray(FloatArray *a, float element)
{
    if (a->used == a->size)
    {
        ++a->size;
        a->array = realloc(a->array, a->size * sizeof(float));
    }
    a->array[a->used++] = element;
}

void prependToFloatArray(FloatArray *a, float element)
{
    if (a->used == a->size)
    {
        ++a->size;
        a->array = realloc(a->array, a->size * sizeof(float));
    }
    float *temp = malloc(a->size * sizeof(float));

    for (int i = 0; i < a->size; i++)
    {
        temp[i] = a->array[i];
    }

    for (int i = 0; i < a->size; i++)
    {
        if (i == 0)
        {
            a->array[i] = element;
        }
        else
        {
            a->array[i] = temp[i - 1];
        }
    }
    free(temp);
    a->used++;
}

void concatFloatArray(FloatArray *a, float *elements)
{
    int index = 0;
    while (*(elements + index))
    {
        appendToFloatArray(a, elements[index]);
        ++index;
    }
}

void initIntArray(IntArray *a)
{
    size_t initialSize = 1;
    a->array = malloc(initialSize * sizeof(int));
    a->used = 0;
    a->size = initialSize;
}

void appendToIntArray(IntArray *a, int element)
{
    if (a->used == a->size)
    {
        ++a->size;
        a->array = realloc(a->array, a->size * sizeof(int));
    }
    a->array[a->used++] = element;
}

void prependToIntArray(IntArray *a, int element)
{
    if (a->used == a->size)
    {
        ++a->size;
        a->array = realloc(a->array, a->size * sizeof(int));
    }
    int *temp = malloc(a->size * sizeof(int));

    for (int i = 0; i < a->size; i++)
    {
        temp[i] = a->array[i];
    }

    for (int i = 0; i < a->size; i++)
    {
        if (i == 0)
        {
            a->array[i] = element;
        }
        else
        {
            a->array[i] = temp[i - 1];
        }
    }
    free(temp);
    a->used++;
}

void concatIntArray(IntArray *a, int *elements)
{
    int index = 0;
    while (*(elements + index))
    {
        appendToIntArray(a, elements[index]);
        ++index;
    }
}

void initCharArray(CharArray *a)
{
    size_t initialSize = 1;
    a->array = malloc(initialSize * sizeof(char));
    a->used = 0;
    a->size = initialSize;
}

void appendToCharArray(CharArray *a, char element)
{
    if (a->used == a->size)
    {
        ++a->size;
        a->array = realloc(a->array, a->size * sizeof(char));
    }
    a->array[a->used++] = element;
}

void concatCharArray(CharArray *a, char *elements)
{
    int index = 0;
    while (*(elements + index))
    {
        appendToCharArray(a, elements[index]);
        ++index;
    }
}

void mySuccess(const char *msg, int socket)
{
    close(socket);
    printf("%s", msg);
    // exit(EXIT_SUCCESS);
}

void myError(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

FILE *getFile(char *fileName, char *mode)
{
    FILE *file;
    file = fopen(fileName, mode);
    if (file == NULL)
    {
        myError("Error to open file");
    }
    return file;
}

void closeFile(FILE *file)
{
    fflush(file);
    fclose(file);
}

int digits_only(const char *s)
{
    while (*s)
    {
        if (isdigit(*s++) == 0)
            return 0;
    }

    return 1;
}

int addrParse(const char *addrStr, const char *portStr, struct sockaddr_storage *storage)
{
    if (addrStr == NULL || portStr == NULL)
    {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portStr);

    if (port == 0)
    {
        return -1;
    }

    port = htons(port);

    struct in_addr inaddr4; // 32-bit IP address
    if (inet_pton(AF_INET, addrStr, &inaddr4))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; // 128-bit ipv6 address
    if (inet_pton(AF_INET6, addrStr, &inaddr6))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        // addr6->sin6_addr = inaddr6;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void add2str(const struct sockaddr *addr, char *str, size_t strsize)
{

    int version;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if (addr->sa_family == AF_INET)
    {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1))
        {
            myError("ntop");
        }
        port = ntohs(addr4->sin_port); // network to host short
    }
    else if (addr->sa_family == AF_INET6)
    {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1))
        {
            myError("ntop");
        }
        port = ntohs(addr6->sin6_port); // network to host short
    }
    else
    {
        myError("unknown protocol family");
    }

    if (str)
    {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

int server_sockaddr_init(const char *proto, const char *portStr, struct sockaddr_storage *storage)
{

    uint16_t port = (uint16_t)atoi(portStr);

    if (port == 0)
    {
        return -1;
    }

    port = htons(port);

    memset(storage, 0, sizeof(*storage));
    if (strcmp(proto, "v4") == 0)
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr.s_addr = INADDR_ANY;
        return 0;
    }
    else if (strcmp(proto, "v6") == 0)
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        addr6->sin6_addr = in6addr_any;
        return 0;
    }
    else
    {
        return -1;
    }
}