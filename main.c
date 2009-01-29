#include <stdio.h>
#include <signal.h>
#include "server.h"
#include "log.h"

sig_atomic_t quit; /* set this to non-zero to quit */

/* signal handler that sets the quit flag */
static void sh_quit(int s) {
	quit=1;
}

int main(void)
{
	int s;
	unsigned short port;

	signal(SIGINT, sh_quit);

	port = 3333;

	log_init();

	printf("Starting server on port %d...\n", port);
	log_inform("Started.\n");

	s = new_server(port);
	if(s < 0) {
		return 1;
	}

	printf("Server socket: %d\n", s);

	while(!quit)
	{
		server_wait_clients(s);
	}

	printf("Exiting.\n");
	log_inform("Closed.\n");
	return 0;
}
