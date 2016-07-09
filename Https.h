#ifndef HTTPS_H
#define HTTPS_H

#include <netdb.h>  
#include <resolv.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <netinet/in.h>  
#include <sys/socket.h>  
#include <sys/wait.h>  
#include <arpa/inet.h>  
#include <openssl/bio.h>  
#include <openssl/err.h>  
#include <openssl/ssl.h>  
#include <sys/epoll.h>
#include <map>

#include "Type.h"

#define SSL_SERVER_RSA_CERT	"./cacert/cacert.pem"
#define SSL_SERVER_RSA_KEY	"./cacert/privkey.pem"

#define HTTPS_PORT 443

using namespace std;

struct SSLEPOLL
{
	SSL *ssl;
	SSL_CTX *ctx;
	INT fd;
	SSLEPOLL(SSL *ssl_, SSL_CTX *ctx_, INT fd_);
	~SSLEPOLL();
};

SSL_CTX *SSL_Init();

VOID ShowCerts(SSL *ssl);  

INT spawn_child_ssl(INT sslsock, INT eventsnum, SSL_CTX *ctx);

VOID start_worker_process_ssl(INT sslsock, INT eventsnum, SSL_CTX *ctx);

#endif
