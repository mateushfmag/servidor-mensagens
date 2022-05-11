#pragma once
#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>
#include <arpa/inet.h>

#define len(a) (sizeof(a) / sizeof(a[0]))

int addrParse(const char *addrStr, const char *portStr, struct sockaddr_storage *storage);

void add2str(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portStr, struct sockaddr_storage *storage);

void myError(const char *msg);

int digits_only(const char *s);

#endif