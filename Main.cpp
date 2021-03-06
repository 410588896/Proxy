#include <stdio.h>
#include <stdlib.h>

#include "Type.h"
#include "Proxy.h"
#include "ReadConf.h"
#include "MemoryPool.h"
#include "Global.h"
#include "Https.h"

VOID Stop_Server(INT signo)
{
	ReleaseMemoryPool(memorypool) ; 
	printf("\033[1;32;1m########Delete Share Memory and Memory Pool!\n\033[0m");
	exit(0);
}

INT main(INT argc, CHAR *argv[])
{
//Init
/**************************PORT INIT****************************/	
	CHAR proxyport[8] = {0};
	CHAR proxyhttpsport[8] = {0};
	CHAR memsize[16] = {0};
	CHAR processes[8] = {0};
	CHAR listen[8] = {0};
	CHAR events[8] = {0};
	INT port, httpsport, mempoolsize, processnum, listennum, eventsnum;
	read_conf(proxyport, proxyhttpsport, memsize, processes, listen, events);
	if(0 == atoi(proxyport))
		port = DEFAULT_PROXY_PORT;
	else
		port = atoi(proxyport);
	if(0 == atoi(memsize))
		mempoolsize = DEFAULT_MEMPOOL_SIZE;
	else
		mempoolsize = atoi(memsize);
	if(0 == atoi(processes))
		processnum = DEFAULT_PROCESSES;
	else
		processnum = atoi(processes);
	if(0 == atoi(listen))
		listennum = DEFAULT_LISTEN;
	else
		listennum = atoi(listen);
	if(0 == atoi(events))
		eventsnum = DEFAULT_EVENTS_NUM;
	else
		eventsnum = atoi(events);
	if(0 == atoi(proxyhttpsport))
		httpsport = DEFAULT_HTTPS_PORT;
	else
		httpsport = atoi(proxyhttpsport);

#ifdef DEBUG
	printf("\033[1;32;1mPORT:%d\nHTTPSPORT:%d\nMEMPOOLSIZE:%d\nPROCESSES:%d\nLISTENNUM:%d\nEVENTSNUM:%d\n########Read conf done!\033[0m\n", port, httpsport, mempoolsize, processnum, listennum, eventsnum);
#endif
/***************************************************************/

/**************************MEMORY POOL INIT*********************/
	memorypool = (VOID*)malloc(sizeof(CHAR) * mempoolsize);
	PMEMORYPOOL MemoryPool = CreateMemoryPool(memorypool, mempoolsize);
#ifdef DEBUG
	printf("\033[1;32;1m########Memory Pool Init done!\033[0m\n");
#endif
/***************************************************************/	

/**************************SSL INIT*****************************/
	SSL_CTX *ctx; 
	ctx = SSL_Init();
/***************************************************************/

/**************************SOCKET CREATE************************/
	INT servsock = create_server_socket(port, listennum);
	if(servsock < 0)
	{
#ifdef DEBUG
		printf("\033[1;32;1m########Server Socket Init failed!\033[0m\n");
#endif
	}

#ifdef HTTPS
	INT sslsock = create_server_socket(httpsport, listennum);
	if(sslsock < 0)
	{
#ifdef DEBUG
		printf("\033[1;32;1m########HTTPS Server Socket Init failed!\033[0m\n");
#endif
	}
#endif

#ifdef DEBUG
	printf("\033[1;32;1m########Server Socket Init successed!\033[0m\n");
#endif
/***************************************************************/

/************************Create http muti process***************/
	INT pid, ret, status;	
	for(INT i = 0; i < processnum; i++)
	{
		ret = spawn_child(servsock, eventsnum);
		if(ret == 0)
			return 0;	//return for child
		else if(ret == 1)
		{
#ifdef DEBUG
			printf("\033[1;32;1m########Process[%d] was born!\033[0m\n", i);
#endif
			alive_process++;
		}
	}
	close(servsock);	
/***************************************************************/

/**********************Create https muti process****************/
#ifdef HTTPS
	for(INT i = 0; i < processnum; i++)
	{
		ret = spawn_child_ssl(sslsock, eventsnum, ctx);
		if(ret == 0)
			return 0;	//return for child
		else if(ret == 1)
		{
#ifdef DEBUG
			printf("\033[1;32;1m########Process[%d] was born!\033[0m\n", i);
#endif
			alive_process++;

		}
	}
	close(sslsock);
	SSL_CTX_free(ctx);  
#endif
/***************************************************************/
	signal(SIGINT, Stop_Server);
	while(pid = wait(&status) > 0)
	{
#ifdef DEBUG
		if(WIFEXITED(status))
		{
			printf("child exit: <%d> exit, normal termination, exit status = %d\n", pid, WEXITSTATUS(status));
		}
		else if(WIFSIGNALED(status))
		{
			printf("child exit: <%d> exit, abnormal termination, signal number = %d%s\n", pid, WTERMSIG(status), "");
		}
		else if(WIFSTOPPED(status))
		{
			printf("child exit: <%d> exit, child stopped, signal number = %d\n", pid, WSTOPSIG(status));
		}		
		printf("\033[1;32;1m########pid = %d\n\033[0m\n", pid);
#endif
		sleep(10);
	}
	return 0;
}

