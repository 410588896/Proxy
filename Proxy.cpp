#include "Proxy.h"

VOID start_worker_process()
{
	while(1)
	{
		printf("heheda\n");
		sleep(10);
	}
}

INT spawn_child(INT servsock)
{
	pid_t child;
	child = fork();	
	if(child < 0)
	{
#ifdef LOG
		LOG("spawn child: fork childerror!\n");
#endif
		return -1;
	}
	else if(child == 0)	//in child
	{
		start_worker_process();	
		return 0;
	}
	else			//in parent
	{
		//do some communication with child
		printf("%d\n",child);
		return 1;
	}
}

INT create_server_socket(INT port, INT listennum) 
{
	INT server_sock, optval;
	struct sockaddr_in server_addr;

	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		return SERVER_SOCKET_ERROR;

	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) 
		return SERVER_SETSOCKOPT_ERROR;
	
	if(fcntl(server_sock, F_SETFL, O_NONBLOCK) < 0)
		return SERVER_SETSOCKOPT_ERROR;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) 
		return SERVER_BIND_ERROR;

	if (listen(server_sock, listennum) < 0) 
		return SERVER_LISTEN_ERROR;

	return server_sock;
}
