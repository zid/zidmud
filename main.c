#include <stdio.h>
#include "server.h"


int main(void)
{
	int s, port;

	port = 3333;

	printf("Starting server on port %d...\n", port);
	new_server(port);

	printf("Exiting.\n");
	return s;
}

