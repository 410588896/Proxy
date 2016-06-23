#include <stdio.h>
#include <stdlib.h>

#include "Type.h"
#include "Proxy.h"
#include "ReadConf.h"
#include "MemoryPool.h"
#include "Global.h"

INT main(INT argc, CHAR *argv[])
{
//Init
/**************************PORT INIT****************************/	
	CHAR proxyport[8] = {0};
	CHAR memsize[16] = {0};
	CHAR processes[8] = {0};
	INT port, mempoolsize, processnum;
	read_conf(proxyport, memsize, processes);
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
#ifdef DEBUG
	printf("\033[1;32;1m########Read conf done!\033[0m\n");
#endif
/***************************************************************/

/**************************MEMORY POOL INIT*********************/
	VOID *memorypool = (VOID*)malloc(sizeof(CHAR) * mempoolsize);
	PMEMORYPOOL MemoryPool = CreateMemoryPool(memorypool, mempoolsize);
#ifdef DEBUG
	printf("\033[1;32;1m########Memory Pool Init done!\033[0m\n");
#endif
/***************************************************************/	
	INT servsock = create_server_socket(port);
	if(servsock < 0)
	{
#ifdef DEBUG
		printf("\033[1;32;1m########Server Socket Init failed!\033[0m\n");
#endif
	}	
	for(INT i = 0; i < processnum; i++)
	{
		INT ret = spawn_child();
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
}

