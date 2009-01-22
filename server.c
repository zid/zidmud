#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "log.h"
#include "server.h"
#include "client.h"

static int yes = 1;
static struct client *clients;
static struct client *client_head;
static int highest_fd = 0;

static void server_handle(int s)
{
	char buf[512];
	int l;

	l = recv(s, buf, 512, 0);
	if(l == 0){
		/* FIXME: Delete client here */
		close(s);
	}
	buf[l] = 0;

	printf("[%d]: %s\n", s, buf);
}

static void server_add_client(int s)
{
	struct client *c;
	struct sockaddr sock_info;
	int new_client;
	unsigned int len;

	new_client = accept(s, &sock_info, &len);

	printf("New client: %d...\n", new_client);
	
	/* First client ever */
	if(!client_head) {
		clients = malloc(sizeof(*clients));
		c = clients;
	} else {
		client_head->next = malloc(sizeof(*clients));
		c = client_head->next;
	}

	c->next = NULL;
	c->s = new_client;
	c->sock_info = malloc(sizeof(struct sockaddr));
	memcpy(c->sock_info, &sock_info, sizeof(sock_info));

	client_head = c;

	if(new_client > highest_fd)
		highest_fd = new_client;

	printf("\tRegistered.\n");
}

void server_wait_clients(int s)
{
	fd_set fds;
	struct client *p;

	FD_ZERO(&fds);
	FD_SET(s, &fds);

	for(p = clients; p != NULL; p = p->next)
	{
		FD_SET(p->s, &fds);
	}
	
	if(highest_fd == 0)
		highest_fd = s;

	select(highest_fd+1, &fds, NULL, NULL, NULL);

	if(FD_ISSET(s, &fds))
	{
		/* Server socket is in the list, accept() it */
		server_add_client(s);
	}

	for(p = clients; p != NULL; p = p->next)
	{
		/* Check for client sockets with data to be read */
		if(FD_ISSET(p->s, &fds))
			server_handle(p->s);
	}
}

void server_dump_clients()
{
	struct client *c;
	int ip;

	for(c = clients; c; c = c->next)
	{
		ip = ((struct sockaddr_in *)c->sock_info)->sin_addr.s_addr;
		
		printf("sock: %d, ip: %d.%d.%d.%d\n", 
			c->s,
			ip&0xFF,
			(ip&0xFF00)>>8,
			(ip&0xFF0000)>>16,
			(ip&0xFF000000)>>24
		);
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
		die("getaddinfo: %s\n", gai_strerror(r));
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
	{
		die("Unable to bind a socket to listen on.\n");
		return -1; 	/* make gcc shut up about not using s */
	}
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

