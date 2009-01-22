#include <stdio.h>
#include "server.h"


int main(void)
{
	int s, quit = 0;
	unsigned short port;

	port = 3333;

	printf("Starting server on port %d...\n", port);
	s = new_server(port);
	if(s < 0) {
		return 1;
	}
	printf("Server socket: %d\n", s);
	while(!quit)
	{
		server_wait_clients(s);
	}
	server_dump_clients();
	printf("Exiting.\n");
	return 0;
}
