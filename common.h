#pragma once
#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

#define len(a) (sizeof(a) / sizeof(a[0]))

typedef struct
{
    char *array;
    size_t used;
    size_t size;
} CharArray;

typedef struct
{
    int *array;
    size_t used;
    size_t size;
} IntArray;

int addrParse(const char *addrStr, const char *portStr, struct sockaddr_storage *storage);

void add2str(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portStr, struct sockaddr_storage *storage);

void myError(const char *msg);

int digits_only(const char *s);

FILE *getFile(char *fileName, char *mode);

void closeFile(FILE *file);

void initCharArray(CharArray *a);

void appendToCharArray(CharArray *a, char element);

void concatCharArray(CharArray *a, char *elements);

void initIntArray(IntArray *a);

void appendToIntArray(IntArray *a, int element);

void concatIntArray(IntArray *a, int *elements);

#endif