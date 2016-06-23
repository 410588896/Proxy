#include <stdio.h>
#include <stdlib.h>

#include "Type.h"
#include "Proxy.h"
#include "ReadConf.h"
#include "MemoryPool.h"

INT main(INT argc, CHAR *argv[])
{
//Init
/**************************PORT INIT****************************/	
	CHAR proxyport[8] = {0};
	CHAR memsize[16] = {0};
	INT port, mempoolsize;
	read_conf(proxyport, memsize);
	if(0 == atoi(proxyport))
		port = DEFAULT_PROXY_PORT;
	else
		port = atoi(proxyport);
	if(0 == atoi(memsize))
		mempoolsize = DEFAULT_MEMPOOL_SIZE;
	else
		mempoolsize = atoi(memsize);
	printf("\033[1;32;1m########Read conf done!\033[0m\n");
/***************************************************************/

/**************************MEMORY POOL INIT*********************/
	VOID *memorypool = (VOID*)malloc(sizeof(CHAR) * mempoolsize);
	PMEMORYPOOL MemoryPool = CreateMemoryPool(memorypool, mempoolsize);
	printf("\033[1;32;1m########Memory Pool Init done!\033[0m\n");
/***************************************************************/	
}

