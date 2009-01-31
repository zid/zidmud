#include <stdio.h>
#include "server.h"

void client_handle(struct client *p)
{
	printf("[%d] said: \"%s\"\n", p->s, p->input);

	/* Reset the input buffer */
	p->in_filled = 0;
	p->input[0] = '\0';
	p->in_ready = 0;

	send_client(p, "Testing");
	send_client(p, "partial line output");
	send_client(p, "\n");
}
