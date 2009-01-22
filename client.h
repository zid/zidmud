#include <sys/socket.h>

struct client {
	int s;				/* socket fd */
	struct sockaddr *sock_info;	/* ip etc */
	struct client *next;		/* next client */
};	
