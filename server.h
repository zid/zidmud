#include "client.h"

void server_wait_clients(int s);
int new_server(int port);
void send_client(struct client *, const char *);
