#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "log.h"
#include "server.h"
#include "client.h"

static int yes = 1;
static struct client *clients;
static struct client *client_head;

void server_dump_clients()
{
	struct client *c;
	
	for(c = clients; c; c = c->next)
	{
		printf("sock: %d, ip: %d\n", c->s, 
((struct sockaddr_in *)c)->sin_addr.s_addr);
	}

}

static int make_socket(unsigned short port)
{
	int r, s;
	struct addrinfo hints, *serv, *p;
	char portstr[8];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	sprintf(portstr, "%u", port);

	r = getaddrinfo(NULL, portstr, &hints, &serv);
	if(r != 0)
	{
		log_warn("getaddinfo: %s\n", gai_strerror(r));
		return -1;
	}

	/* Find something we can bind a socket on */
	for(p = serv; p != NULL; p = p->ai_next)
	{
		s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(s == -1)
		{
			log_warn("socket: %d\n", errno);
			continue;
		}
		
		r = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if(r == -1)
		{
			log_warn("setsockopt: %d\n", errno);
			continue;
		}

		r = bind(s, p->ai_addr, p->ai_addrlen);
		if(r == -1)
		{
			log_warn("bind: %d\n", errno);
			close(s);
			continue;
			
		}
		break;	/* Tests passed, Got one! */
	}

	if(p == NULL)
		die("Unable to bind a socket to listen on.\n");
	
	freeaddrinfo(serv);

	return s;
}

static void listen_on(int s)
{
	int r;

	r = listen(s, 10);
	if(r == -1)
		die("Unable to listen on bound socket.\n");
	
}

int new_server(int port)
{
	int s;

	s = make_socket(port);
	listen_on(s);
	
	return s;
}

void server_add_client(int s, struct sockaddr *sock_info)
{
	struct client *c;

	printf("New client: %d...\n", s);
	
	/* First client ever */
	if(!client_head) {
		clients = malloc(sizeof(*clients));
		c = clients;
	} else {
		client_head->next = malloc(sizeof(*clients));
		c = client_head->next;
	}

	c->next = NULL;
	c->s = s;
	c->sock_info = malloc(sizeof(struct sockaddr));
	memcpy(c->sock_info, sock_info, sizeof(*sock_info));

	client_head = c;

	printf("\tRegistered.\n");
}

