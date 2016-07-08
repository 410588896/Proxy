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
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <wait.h>
#include <netdb.h>
#include <map>

#include "Type.h"

#define DEFAULT_PROXY_PORT 8080
#define DEFAULT_HTTPS_PORT 8000
#define HTTP_PORT 80
#define DEFAULT_MEMPOOL_SIZE 10240000
#define MAXLEN 1024
#define DEFAULT_PROCESSES 16
#define DEFAULT_LISTEN 512
#define DEFAULT_EVENTS_NUM 64
#define SERVER_SOCKET_ERROR -1
#define SERVER_SETSOCKOPT_ERROR -2
#define SERVER_BIND_ERROR -3
#define SERVER_LISTEN_ERROR -4
#define CLIENT_SOCKET_ERROR -5
#define CLIENT_RESOLVE_ERROR -6
#define CLIENT_CONNECT_ERROR -7

#define LOG(fmt...)  do { fprintf(stderr,"%s %s ",__DATE__,__TIME__); fprintf(stderr, ##fmt); } while(0)

using namespace std;

INT create_server_socket(INT port, INT listennum);

INT spawn_child(INT servsock, INT eventsnum);

VOID start_worker_process(INT servsock, INT eventsnum);

INT connect_isolate(CHAR *host, INT port);

#endif

