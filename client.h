#include <sys/socket.h>

struct client {
	int s;				/* socket fd */

	char *input;			/* input buffer */
	int in_filled;			/* Input buffer fullness */
	int in_ready;			/* Input is ready for processing */

	char *output;			/* Output buffer */
	int out_full;			/* Flag for purging output */
	struct sockaddr *sock_info;	/* ip etc */
	struct client *next;		/* next client */
};	

void client_handle(struct client *);
