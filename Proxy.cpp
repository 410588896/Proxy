#include "Proxy.h"

static INT make_socket_non_blocking (INT sfd)  
{  
	INT flags, s;  

	//得到文件状态标志  
	flags = fcntl (sfd, F_GETFL, 0);  
	if (flags == -1)  
	{  
#ifdef DEBUG
		printf("Fcntl Error!\n");  
#endif
		return -1;  
	}  

	//设置文件状态标志  
	flags |= O_NONBLOCK;  
	s = fcntl (sfd, F_SETFL, flags);  
	if (s == -1)  
	{  
#ifdef DEBUG
		printf("Fcntl Error!\n"); 
#endif 
		return -1;  
	}  
	return 0;  
}  

INT connect_isolate(CHAR *host, INT port)
{
	INT sockfd;    
    struct hostent *he;    
    struct sockaddr_in server;
    
    if((he = gethostbyname(host)) == NULL)
    {
#ifdef DEBUG
        printf("gethostbyname() error\n");
#endif 
		return 0;
    }
    
    if((sockfd=socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
#ifdef DEBUG
        printf("socket() error\n");
#endif
		return 0;
    }
    bzero(&server,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr = *((struct in_addr *)he->h_addr);
    if(connect(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
#ifdef DEBUG
		printf("connect() error\n");
#endif        
		return 0;
	}

	return sockfd;
}

VOID start_worker_process(INT servsock, INT eventsnum)
{
	INT efd, ret;
	map<INT, INT> sockfd_map;
	struct epoll_event event;  
	struct epoll_event *events;  	
	efd = epoll_create1(0);  
	if(-1 == efd)  
	{  
#ifdef DEBUG
		printf("Epoll_Create Error!\n");  
#endif
		_exit(0);  
	}  
	event.data.fd = servsock;  
	event.events = EPOLLIN | EPOLLET;		//Edge Triggered  
	ret = epoll_ctl (efd, EPOLL_CTL_ADD, servsock, &event);  
	if (-1 == ret)  
	{  
#ifdef DEBUG
		printf("Epoll_Ctl Error!\n");  
#endif
		_exit(0);  
	} 
	events = (struct epoll_event *)calloc (eventsnum, sizeof(struct epoll_event)); 

 	//The event loop
	while(1)  
	{  
		INT num;  
		num = epoll_wait(efd, events, eventsnum, -1);  
		for(INT i = 0; i < num; i++)  
		{  
			if((events[i].events & EPOLLERR) || 
			   (events[i].events & EPOLLHUP) || 
			   (events[i].events == (EPOLLOUT | EPOLLHUP)) ||
			   (events[i].events == (EPOLLIN | EPOLLERR | EPOLLHUP)) ||
			   ((!(events[i].events & EPOLLIN))))
			{  
				/* An error has occured on this fd, or the socket is not 
				   ready for reading (why were we notified then?) */  
#ifdef DEBUG
				printf ("Epoll Error!0x%x\n", events[i].events);  
#endif
				epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
				close (events[i].data.fd);  
				continue;  
			}  

			else if(servsock == events[i].data.fd)  
			{  
				/* We have a notification on the listening socket, which 
				   means one or more incoming connections. */  
				while(1)  
				{  
					struct sockaddr in_addr;  
					socklen_t in_len;  
					INT infd;  
					CHAR hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];  

					in_len = sizeof in_addr;  
					infd = accept(servsock, &in_addr, &in_len);  
					if (infd == -1)  
					{  
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK))  
						{  
							/* We have processed all incoming 
							   connections. */  
							break;  
						}  
						else  
						{  
#ifdef DEBUG
							printf("Accept Error!\n");  
#endif
							break;  
						}  
					}  

					//将地址转化为主机名或者服务名  
					ret = getnameinfo(&in_addr, in_len,  
							hbuf, sizeof hbuf,  
							sbuf, sizeof sbuf,  
							NI_NUMERICHOST | NI_NUMERICSERV);//flag参数:以数字名返回  
					//主机地址和服务地址  

					if (ret == 0)  
					{  
#ifdef DEBUG
						printf("Accepted connection on descriptor %d "  
								"(host=%s, port=%s)\n", infd, hbuf, sbuf);  
#endif
					}  

					/* Make the incoming socket non-blocking and add it to the 
					   list of fds to monitor. */  
					ret = make_socket_non_blocking (infd);  
					if(ret == -1)  
					{
						close(infd);
						continue;
					}  

					event.data.fd = infd;  
					event.events = EPOLLIN | EPOLLET;  
					ret = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);  
					if(ret == -1)  
					{  
#ifdef DEBUG
						printf("Epoll_Ctl Error!\n");  
#endif
						close(infd);
						continue;	
					}  
				}  
				continue;  
			}  
			else  
			{  
				/* We have data on the fd waiting to be read. Read and 
				   display it. We must read whatever data is available 
				   completely, as we are running in edge-triggered mode 
				   and won't get a notification again for the same 
				   data. */  
				INT done = 0, flag = 0, remotesock;  
				ssize_t count;  
				CHAR head[MAXLEN];  
				CHAR *hosttmp, *tmp;
				CHAR host[128] = {0};
				CHAR porttmp[16] = {0};
				INT port;
				count = read(events[i].data.fd, head, sizeof(head));  
				if(hosttmp = strstr(head, "Host: "))
				{
					tmp = strstr(hosttmp, "\r\n");
					memcpy(host, hosttmp + 6, tmp - (hosttmp + 6));
#ifdef PRINTCONTENT
					ret = write(1, head, count);  
					if(ret == -1)  
					{  
						printf("print error!\n");  
					}
#endif
					CHAR *pportblank;
					if(pportblank = strstr(head, " HTTP/1."))
					{
						CHAR *pport;
						if(pport = strstr(head, ":"))
							if(pport[1] != '\\' && *(pport - 1) != 'p')
							{
								memcpy(porttmp, pport + 1, pportblank - (pport + 1));
								port = atoi(porttmp);
							}
							else
								port = HTTP_PORT;
						else
							port = HTTP_PORT;
					}
					else
						port = HTTP_PORT;
#ifdef DEBUG
					printf("######Host:%s,Port:%d\n", host, port);
#endif
					remotesock = connect_isolate(host, port);
					if(remotesock == 0)
					{
						epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
						close(events[i].data.fd);
						continue;
					}
					event.data.fd = remotesock;  
					event.events = EPOLLIN | EPOLLET;  
					ret = epoll_ctl(efd, EPOLL_CTL_ADD, remotesock, &event);  
					if(ret == -1)  
					{  
#ifdef DEBUG
						printf("Epoll_Ctl Error!\n");  
#endif
						close(remotesock);
						continue;	
					}  
					sockfd_map[remotesock] = events[i].data.fd;
					sockfd_map[events[i].data.fd] = remotesock;
					flag = 1;
					ret = write(remotesock, head, count);
					if(ret == -1)
					{
						epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
						epoll_ctl(efd, EPOLL_CTL_DEL, remotesock, &events[i]);  
						close(events[i].data.fd);  
						close(remotesock);
						map<INT, INT>::iterator pos = sockfd_map.find(events[i].data.fd);
						if(pos != sockfd_map.end())
						{
							sockfd_map.erase(pos);
						}	
						pos = sockfd_map.find(remotesock);
						if(pos != sockfd_map.end())
						{
							sockfd_map.erase(pos);
						}
#ifdef DEBUG
						printf("write error	1!\n");
#endif
						break;
					}
				}
				else
				{
					ret = write(sockfd_map[events[i].data.fd], head, count);
					if(ret == -1)
					{
						epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
						epoll_ctl(efd, EPOLL_CTL_DEL, sockfd_map[events[i].data.fd], &events[i]);  
						close(events[i].data.fd);  
						close(sockfd_map[events[i].data.fd]);
						map<INT, INT>::iterator pos = sockfd_map.find(events[i].data.fd);
						if(pos != sockfd_map.end())
						{
							sockfd_map.erase(pos);
						}	
						pos = sockfd_map.find(sockfd_map[events[i].data.fd]);
						if(pos != sockfd_map.end())
						{
							sockfd_map.erase(pos);
						}
#ifdef DEBUG
						printf("write error 2!\n");
#endif
						break;
					}

				}
				while (1)  
				{  
					ssize_t count;  
					CHAR buf[MAXLEN];  

					count = read(events[i].data.fd, buf, sizeof(buf));  
					if(count == -1)  
					{  
						/* If errno == EAGAIN, that means we have read all 
						   data. So go back to the main loop. */  
						if(errno != EAGAIN)  
						{ 
#ifdef DEBUG	
							printf("########Read Error!\n");
#endif	
							done = 1;  
						}  
						break;  
					}  
					else if(count == 0)  
					{  
						/* End of file. The remote has closed the 
						   connection. */  
						done = 1;  
						break;  
					}  

					/* Write the buffer to standard output */  
#ifdef PRINTCONTENT
					ret = write(1, buf, count);  
					if(ret == -1)  
					{  
						printf("print error!\n");  
					}
#endif	
					if(flag == 1)
					{
						ret = write(remotesock, buf, count);
						if(ret == -1)
						{
							epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
							epoll_ctl(efd, EPOLL_CTL_DEL, remotesock, &events[i]);  
							close(events[i].data.fd);  
							close(remotesock);
							map<INT, INT>::iterator pos = sockfd_map.find(events[i].data.fd);
							if(pos != sockfd_map.end())
							{
								sockfd_map.erase(pos);
							}	
							pos = sockfd_map.find(remotesock);
						   	if(pos != sockfd_map.end())
							{
								sockfd_map.erase(pos);
							}
#ifdef DEBUG
							printf("write error 3!\n");
#endif
							break;
						}
					}
					else
					{
						ret = write(sockfd_map[events[i].data.fd], buf, count);
						if(ret == -1)
						{
							epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
							epoll_ctl(efd, EPOLL_CTL_DEL, sockfd_map[events[i].data.fd], &events[i]);  
							close(events[i].data.fd);  
							close(sockfd_map[events[i].data.fd]);
							map<INT, INT>::iterator pos = sockfd_map.find(events[i].data.fd);
							if(pos != sockfd_map.end())
							{
								sockfd_map.erase(pos);
							}	
							pos = sockfd_map.find(sockfd_map[events[i].data.fd]);
						   	if(pos != sockfd_map.end())
							{
								sockfd_map.erase(pos);
							}
#ifdef DEBUG
							printf("write error 4!\n");
#endif
							break;
						}
					}
				}  

				if (done)  
				{  
#ifdef DEBUG
					printf("Closed connection on descriptor %d\n",  
							events[i].data.fd);  
#endif
					/* Closing the descriptor will make epoll remove it 
					   from the set of descriptors which are monitored. */  
					if(flag == 0)
					{
						epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
						epoll_ctl(efd, EPOLL_CTL_DEL, sockfd_map[events[i].data.fd], &events[i]);
						close(events[i].data.fd);  
						close(sockfd_map[events[i].data.fd]);
						map<INT, INT>::iterator pos = sockfd_map.find(events[i].data.fd);
						if(pos != sockfd_map.end())
						{
							sockfd_map.erase(pos);
						}	
						pos = sockfd_map.find(sockfd_map[events[i].data.fd]);
						if(pos != sockfd_map.end())
						{
							sockfd_map.erase(pos);
						}
					}
				}  
			}  
		}  
	}  

	free(events);  

	close(servsock);  

	return; 
}

INT spawn_child(INT servsock, INT eventsnum)
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
		start_worker_process(servsock, eventsnum);	
		return 0;
	}
	else			//in parent
	{
		//do some communication with child
		return 1;
	}
}

INT create_server_socket(INT port, INT listennum) 
{
	INT server_sock, optval;
	struct sockaddr_in server_addr;

	if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		return SERVER_SOCKET_ERROR;

	if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) 
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
