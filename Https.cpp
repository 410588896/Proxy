#include "Https.h"
#include "Proxy.h"

SSLEPOLL::SSLEPOLL(SSL *ssl_, SSL_CTX *ctx_, INT fd_)
{
	memset(this, 0, sizeof(*this));
	ssl = ssl_;
	ctx = ctx_;
	fd = fd_;	
}

SSLEPOLL::~SSLEPOLL()
{
	close(fd);
	if(ssl)
	{
		SSL_shutdown (ssl);
		SSL_free(ssl);
		ssl = NULL;
	}
	if(ctx)
	{
		SSL_CTX_free(ctx); 
		ctx = NULL;
	}
}

SSL_CTX *SSL_Init()
{
	SSL_CTX *ctx;
	SSL_library_init();  
	OpenSSL_add_all_algorithms();  
	SSL_load_error_strings();  
	ctx = SSL_CTX_new(SSLv23_server_method());  
	if(ctx == NULL) 
	{  
		ERR_print_errors_fp(stdout);  
		exit(1);  
	}  
	if(SSL_CTX_use_certificate_file(ctx, SSL_SERVER_RSA_CERT, SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stdout);  
		exit(1);  
	}  
	if(SSL_CTX_use_PrivateKey_file(ctx, SSL_SERVER_RSA_KEY, SSL_FILETYPE_PEM) <= 0) 
	{  
		ERR_print_errors_fp(stdout);  
		exit(1);  
	}  
	if(!SSL_CTX_check_private_key(ctx)) 
	{  
		ERR_print_errors_fp(stdout);  
		exit(1);  
	}
	return ctx;	
}

VOID ShowCerts(SSL *ssl)  
{  
	X509 *cert;  
	CHAR *line;  
	  
	cert = SSL_get_peer_certificate(ssl);  
	if(cert != NULL) 
	{  
		printf("数字证书信息:\n");  
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);  
		printf("证书: %s\n", line);  
		free(line);  
		line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);  
		printf("颁发者: %s\n", line);  
		free(line);  
		X509_free(cert);  
	} 
	else  
		printf("无证书信息！\n");  
}

static INT make_socket_non_blocking_ssl(INT sfd)  
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

INT connect_isolate_ssl(CHAR *host, INT port, SSL *&ssl, SSL_CTX *&ctx)
{
	INT sockfd, len;  
	struct sockaddr_in dest;  

	SSL_library_init();  
	OpenSSL_add_all_algorithms();  
	SSL_load_error_strings();  
	ctx = SSL_CTX_new(SSLv23_client_method());  
	if(ctx == NULL) 
	{  
		ERR_print_errors_fp(stdout);  
		return 0;  
	}  

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{  
#ifdef DEBUG
		printf("Socket error!\n");  
#endif
		return 0;  
	}  
#ifdef DEBUG
	printf("Client Socket Created!\n");  
#endif

	bzero(&dest, sizeof(dest));  
	dest.sin_family = AF_INET;  
	dest.sin_port = htons(port);  
	if(inet_aton(host, (struct in_addr *)&dest.sin_addr.s_addr) == 0) 
	{  
#ifdef DEBUG 
		printf("inet_aton %s error!\n", host);  
#endif
		SSL_CTX_free(ctx);
		return 0;  
	}  
#ifdef DEBUG
	printf("Address Created!\n");  
#endif
	if(connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) != 0) 
	{  
#ifdef DEBUG
		printf("Connect error!\n");
#endif	
		SSL_CTX_free(ctx);
		return 0;  
	}  
#ifdef DEBUG
	printf("Server Connected!\n");  
#endif
	ssl = SSL_new(ctx);  
	SSL_set_fd(ssl, sockfd);  

	if (SSL_connect(ssl) == -1)  
	{
		SSL_shutdown(ssl);  
		SSL_free(ssl);  
		close(sockfd);
		SSL_CTX_free(ctx);
		ERR_print_errors_fp(stderr);  
	}
	else 
	{  
#ifdef DEBUG
		printf("Connected with %s encryption\n", SSL_get_cipher(ssl));  
		ShowCerts(ssl);  
#endif
	} 
	return sockfd;	
}

VOID start_worker_process_ssl(INT sslsock, INT eventsnum, SSL_CTX *ctx)
{
	INT efd, ret;
	map<SSLEPOLL*, SSLEPOLL*> sockfd_map;
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
	SSLEPOLL *sslsockstruct = new SSLEPOLL(NULL, ctx, sslsock);
	event.data.ptr = (VOID*)sslsockstruct;  
	event.events = EPOLLIN | EPOLLET;		//Edge Triggered  
	ret = epoll_ctl(efd, EPOLL_CTL_ADD, sslsock, &event);  
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
				epoll_ctl(efd, EPOLL_CTL_DEL, ((SSLEPOLL*)(events[i].data.ptr))->fd, &events[i]);
				delete (SSLEPOLL*)events[i].data.ptr;
				continue;  
			}  

			else if(sslsock == ((SSLEPOLL*)(events[i].data.ptr))->fd)  
			{  
				/* We have a notification on the listening socket, which 
				   means one or more incoming connections. */  
				while(1)  
				{  
					struct sockaddr in_addr;  
					socklen_t in_len;  
					INT infd;  
					SSL *ssl;
					CHAR hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];  

					in_len = sizeof in_addr;  
					infd = accept(sslsock, &in_addr, &in_len);  
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
					//CONNECT request
					CHAR connect[512] = {0};
					ret = read(infd, connect, sizeof(connect)); 			
					if(ret == -1)
					{
						close(infd);
						continue;
					}
					memset(connect, 0, sizeof(connect));
					strcpy(connect, "HTTP/1.1 200 Connection Established\r\n\r\n");
					ret = write(infd, connect, strlen(connect));
					if(ret == -1)
					{
						close(infd);
						SSL_shutdown(ssl);  
						SSL_free(ssl);	 
						continue;
					}

					ssl = SSL_new(ctx);  
					SSL_set_fd(ssl, infd);  
					/* Make the incoming socket non-blocking and add it to the 
					   list of fds to monitor. */  	 
			//		ret = make_socket_non_blocking_ssl(infd);  
					if(ret == -1)  
					{
						close(infd);
						SSL_shutdown(ssl);  
						SSL_free(ssl);	  
						continue;
					} 
					ret = SSL_accept(ssl);
					if(ret == -1)
					{
#ifdef DEBUG
						printf("SSL accept error\n");  
#endif
						close(infd);
						SSL_shutdown(ssl);  
						SSL_free(ssl);	  
						continue;  
					}
					//judge ret
					INT sslerr = SSL_get_error(ssl, ret);
					if(sslerr != SSL_ERROR_WANT_READ && sslerr != SSL_ERROR_WANT_WRITE)
					{
						close(infd);
						SSL_shutdown(ssl);  
						SSL_free(ssl);
						continue;	
					}
					SSLEPOLL *tmp = new SSLEPOLL(ssl, ctx, infd);
					event.data.ptr = (VOID*)tmp;
					event.events = EPOLLIN | EPOLLET;  
					ret = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);  
					if(ret == -1)  
					{  
#ifdef DEBUG
						printf("Epoll_Ctl Error!\n");  
#endif
						delete tmp;
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
				SSL *sslclient;
				SSL_CTX *ctxclient;
				SSLEPOLL *epolltmp;
				ssize_t count;  
				CHAR head[MAXLEN];  
				CHAR *hosttmp, *tmp;
				CHAR host[128] = {0};
				CHAR porttmp[16] = {0};
				INT port;
				count = SSL_read(((SSLEPOLL*)(events[i].data.ptr))->ssl, head, sizeof(head));  
				printf("###########@@@@@@@:%s\n", head);

				count = SSL_read(((SSLEPOLL*)(events[i].data.ptr))->ssl, head, sizeof(head));  
				printf("!!!!!!!!!!!!!!1%s\n", head);
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
								port = HTTPS_PORT;
						else
							port = HTTPS_PORT;
					}
					else
						port = HTTPS_PORT;
#ifdef DEBUG
					printf("######Host:%s,Port:%d\n", host, port);
#endif
					remotesock = connect_isolate_ssl(host, port, sslclient, ctxclient);
					if(remotesock == 0)
					{
						epoll_ctl(efd, EPOLL_CTL_DEL, ((SSLEPOLL*)(events[i].data.ptr))->fd, &events[i]);
						delete (SSLEPOLL*)events[i].data.ptr;
						continue;
					}
					epolltmp = new SSLEPOLL(sslclient, ctxclient, remotesock);
					event.data.ptr = (VOID*)epolltmp;  
					event.events = EPOLLIN | EPOLLET;  
					ret = epoll_ctl(efd, EPOLL_CTL_ADD, remotesock, &event);  
					if(ret == -1)  
					{  
#ifdef DEBUG
						printf("Epoll_Ctl Error!\n");  
#endif
						delete epolltmp;
						continue;	
					}  
					sockfd_map[epolltmp] = (SSLEPOLL*)events[i].data.ptr;
					sockfd_map[(SSLEPOLL*)events[i].data.ptr] = epolltmp;
					flag = 1;
					ret = SSL_write(sslclient, head, count);
					if(ret == -1)
					{
						epoll_ctl(efd, EPOLL_CTL_DEL, ((SSLEPOLL*)(events[i].data.ptr))->fd, &events[i]);
						epoll_ctl(efd, EPOLL_CTL_DEL, remotesock, &events[i]);  
						map<SSLEPOLL*, SSLEPOLL*>::iterator pos = sockfd_map.find((SSLEPOLL*)events[i].data.ptr);
						delete (SSLEPOLL*)events[i].data.ptr;
						if(pos != sockfd_map.end())
						{
							sockfd_map.erase(pos);
						}	
						pos = sockfd_map.find(epolltmp);
						delete epolltmp;
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
					ret = SSL_write((sockfd_map[(SSLEPOLL *)events[i].data.ptr])->ssl, head, count);
					if(ret == -1)
					{
						epoll_ctl(efd, EPOLL_CTL_DEL, ((SSLEPOLL*)(events[i].data.ptr))->fd, &events[i]);
						struct epoll_event ev;
						ev.events = EPOLLIN | EPOLLET;
						ev.data.fd = (sockfd_map[(SSLEPOLL *)events[i].data.ptr])->fd;
						epoll_ctl(efd, EPOLL_CTL_DEL, sockfd_map[(SSLEPOLL *)events[i].data.ptr]->fd, &ev);  
						map<SSLEPOLL*, SSLEPOLL*>::iterator pos = sockfd_map.find((SSLEPOLL*)events[i].data.ptr);
						delete (SSLEPOLL *)events[i].data.ptr;
						if(pos != sockfd_map.end())
						{
							sockfd_map.erase(pos);
						}	
						pos = sockfd_map.find(sockfd_map[(SSLEPOLL*)events[i].data.ptr]);
						delete sockfd_map[(SSLEPOLL*)events[i].data.ptr];
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
					count = SSL_read(((SSLEPOLL *)(events[i].data.ptr))->ssl, buf, sizeof(buf));  
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
						ret = SSL_write(sslclient, buf, count);
						if(ret == -1)
						{
							epoll_ctl(efd, EPOLL_CTL_DEL, ((SSLEPOLL*)(events[i].data.ptr))->fd, &events[i]);
							struct epoll_event ev;
							ev.events = EPOLLIN | EPOLLET;
							ev.data.fd = remotesock;
							epoll_ctl(efd, EPOLL_CTL_DEL, remotesock, &ev);  
							map<SSLEPOLL*, SSLEPOLL*>::iterator pos = sockfd_map.find((SSLEPOLL *)events[i].data.ptr);
							delete (SSLEPOLL *)events[i].data.ptr;
							if(pos != sockfd_map.end())
							{
								sockfd_map.erase(pos);
							}	
							pos = sockfd_map.find(epolltmp);
							delete epolltmp;
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
						ret = SSL_write(sockfd_map[(SSLEPOLL *)events[i].data.ptr]->ssl, buf, count);
						if(ret == -1)
						{
							epoll_ctl(efd, EPOLL_CTL_DEL, ((SSLEPOLL *)events[i].data.ptr)->fd, &events[i]);
							struct epoll_event ev;
							ev.events = EPOLLIN | EPOLLET;
							ev.data.fd = sockfd_map[(SSLEPOLL *)events[i].data.ptr]->fd;
							epoll_ctl(efd, EPOLL_CTL_DEL, sockfd_map[(SSLEPOLL *)events[i].data.ptr]->fd, &ev);  
							map<SSLEPOLL *, SSLEPOLL *>::iterator pos = sockfd_map.find((SSLEPOLL *)events[i].data.ptr);
							delete (SSLEPOLL *)events[i].data.ptr;
							if(pos != sockfd_map.end())
							{
								sockfd_map.erase(pos);
							}	
							pos = sockfd_map.find(sockfd_map[(SSLEPOLL *)events[i].data.ptr]);
							delete sockfd_map[(SSLEPOLL *)events[i].data.ptr];
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
							((SSLEPOLL *)events[i].data.ptr)->fd);  
#endif

					/* Closing the descriptor will make epoll remove it 
					   from the set of descriptors which are monitored. */  
					if(flag == 0)
					{
						epoll_ctl(efd, EPOLL_CTL_DEL, ((SSLEPOLL *)events[i].data.ptr)->fd, &events[i]);
						struct epoll_event ev;
						ev.events = EPOLLIN | EPOLLET;
						ev.data.fd = sockfd_map[(SSLEPOLL *)events[i].data.ptr]->fd;	
						epoll_ctl(efd, EPOLL_CTL_DEL, sockfd_map[(SSLEPOLL *)events[i].data.ptr]->fd, &ev);
						map<SSLEPOLL *, SSLEPOLL *>::iterator pos = sockfd_map.find((SSLEPOLL *)events[i].data.ptr);
						delete (SSLEPOLL *)events[i].data.ptr;
						if(pos != sockfd_map.end())
						{
							sockfd_map.erase(pos);
						}	
						pos = sockfd_map.find(sockfd_map[(SSLEPOLL *)events[i].data.ptr]);
						delete sockfd_map[(SSLEPOLL *)events[i].data.ptr];
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

	close(sslsock);  
	SSL_CTX_free(ctx);  

	return; 

}

INT spawn_child_ssl(INT sslsock, INT eventsnum, SSL_CTX *ctx)
{
	pid_t child;
	child = fork();	
	if(child < 0)
	{
#ifdef LOG
		LOG("spawn child ssl: fork childerror!\n");
#endif
		return -1;
	}
	else if(child == 0)	//in child
	{
		start_worker_process_ssl(sslsock, eventsnum, ctx);	
		return 0;
	}
	else			//in parent
	{
		//do some communication with child
		return 1;
	}

}
