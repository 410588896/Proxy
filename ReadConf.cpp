#include "ReadConf.h"

/******************************
 * Name		:Read_conf
 * Function :read the conf file
 * Args		:
 * Return	:
 * PS		:
******************************/
VOID read_conf(CHAR *port, CHAR *size)
{
	CHAR buffer[256] = {0};  
	
	std::ifstream in("PROXY.CONF");  
	
	if(!in.is_open())  
	{ 
		std::cout<<"read conf error"<<std::endl; 
		
		exit (1); 
	}  

	while(!in.eof())  
	{  
		memset(buffer, 0, 256);

		in.getline (buffer,128);

		if(buffer[0] == '#')
			continue;	
		
		if(strstr(buffer, "PORT") != NULL)
		{
			CHAR *p_start = NULL;
			
			CHAR *p_end = NULL;

			p_start = strstr(buffer, "=");
			
			p_start = p_start + 2;
			
			p_end = strstr(p_start, ";");
			
			memcpy(port, p_start, p_end - p_start);

			continue;
		}
		else if(strstr(buffer, "MEMORYPOOLSIZE") != NULL)
		{
			CHAR *p_start = NULL;
			
			CHAR *p_end = NULL;

			p_start = strstr(buffer, "=");
			
			p_start = p_start + 2;
			
			p_end = strstr(p_start, ";");
			
			memcpy(size, p_start, p_end - p_start);

			continue;

		}
	}  
	in.close(); 
}

