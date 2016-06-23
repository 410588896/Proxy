#ifndef PROXY_H
#define PROXY_H

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h> 

#include "Type.h"

#define DEFAULT_PROXY_PORT 8080
#define HTTP_PORT 80
#define DEFAULT_MEMPOOL_SIZE 10240000
#define DEFAULT_PROCESSES 16
#define SERVER_SOCKET_ERROR -1
#define SERVER_SETSOCKOPT_ERROR -2
#define SERVER_BIND_ERROR -3
#define SERVER_LISTEN_ERROR -4
#define CLIENT_SOCKET_ERROR -5
#define CLIENT_RESOLVE_ERROR -6
#define CLIENT_CONNECT_ERROR -7

#define LOG(fmt...)  do { fprintf(stderr,"%s %s ",__DATE__,__TIME__); fprintf(stderr, ##fmt); } while(0)

INT create_server_socket(INT port);

INT spawn_child();

VOID start_worker_process();

#endif

