#include <stdio.h>
#include "client.h"

void client_handle(struct client *p)
{
	printf("[%d] said: \"%s\"\n", p->s, p->input);
	p->in_filled = 0;
	p->input[0] = '\0';
	p->in_ready = 0;
}
