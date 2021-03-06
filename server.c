#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "log.h"
#include "server.h"

static int yes = 1;
static struct client *clients = NULL;
static struct client *client_head = NULL;
static int highest_fd = 0;

#define INBUF_MAX 256
#define OUBUF_MAX 4096

void send_client(struct client *p, const char *msg)
{
	int i = p->out_filled;
	int len = strlen(msg);

	if(i + len >= OUBUF_MAX)
	{
		log_inform("%d has a full output buffer!\n", p->s);
		return;
	}

	strcpy(&p->output[i], msg);

	p->out_filled += len;
}

static void free_client(struct client *p)
{
	if(p->input)
		free(p->input);

	if(p->output)
		free(p->output);

	if(p->sock_info)
		free(p->sock_info);

	close(p->s);
	free(p);
}

static void server_delete_client(int s)
{
	struct client *p, *t;

	/* Special case if the very first entry is the one we want */
	if(clients->s == s)
	{
		t = clients;
		if(clients == client_head)
			client_head = clients->next;
		clients = clients->next;
		free_client(t);
		return;
	}

	/* General case, search entry *ahead* of us */
	for(p = clients; p != NULL; p = p->next)
	{
		if(!p->next || p->next->s != s)
			continue;

		t = p->next;
		p->next = p->next->next;
		
		free_client(t);

		/* If p->next is null, we just removed the head */
		if(p->next == NULL)
			client_head = p;
	}

}

static void server_write_client(struct client *p)
{
	int len;

	len = send(p->s, p->output, p->out_filled, 0);
	if(len == p->out_filled)
		p->out_filled = 0;
	else
		server_delete_client(p->s);		
}

static void server_read_client(struct client *p)
{
	char buf[INBUF_MAX], *nl;
	int l;

	l = recv(p->s, buf, INBUF_MAX, 0);
	if(l == 0)
	{
		log_inform("Removed client %d\n", p->s);
		server_delete_client(p->s);
		return;
	}

	if((nl = strchr(buf, '\r')) || (nl = strchr(buf, '\n')))
	{
		*nl = '\0';
		p->in_ready = 1;
	}

	if((nl = strchr(buf, '\n')))
	{
		p->in_ready = 1;
	}

	/* 
	 * Fill buffer from input data.
	 * If the buffer is filled by this action, this will discard
	 * all new data until a newline appears in the input.
	 * If the buffer is not filled by this action, check for 
	 * the newline (in_ready) and chop it out of the stream.
	 * The former case will never need the \n stripping.
	 */
	if((p->in_filled + l) >= INBUF_MAX)
	{
		memcpy(&p->input[p->in_filled], buf, INBUF_MAX - p->in_filled);
		p->in_filled = INBUF_MAX;
		p->input[INBUF_MAX-1] = 0;
	} else {
		memcpy(p->input, buf, l);
		p->in_filled += l;
	}

	if(p->in_ready)
		client_handle(p);
}

static void server_add_client(int s)
{
	struct client *c;
	struct sockaddr_storage sock_info;
	int new_client;
	unsigned int len = sizeof sock_info;

	new_client = accept(s, (struct sockaddr*)&sock_info, &len);

	if(new_client < 0) {
		if(errno == EAGAIN) {
			log_warn("accept: %s\n", strerror(errno));
			return;
		}
		die("accept()");
	}

	log_inform("New client: %d.\n", new_client);

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
	c->sock_info = malloc(len);
	c->input = malloc(INBUF_MAX);
	c->in_filled = 0;
	c->output = malloc(OUBUF_MAX);
	c->out_filled = 0;

	memcpy(c->sock_info, &sock_info, len);

	client_head = c;

	if(new_client > highest_fd)
		highest_fd = new_client;
}

void server_wait_clients(int s)
{
	fd_set in_fds, out_fds;
	struct client *p, *t;
	int res;

	FD_ZERO(&in_fds);
	FD_ZERO(&out_fds);

	FD_SET(s, &in_fds);

	for(p = clients; p != NULL; p = p->next)
	{
		/* Add every socket to the read set */
		FD_SET(p->s, &in_fds);

		/* Add clients with pending data to write set */
		if(!p->out_filled)
			continue;
		FD_SET(p->s, &out_fds);
	}

	if(highest_fd == 0)
		highest_fd = s;

	res = select(highest_fd+1, &in_fds, &out_fds, NULL, NULL);

	/* EINTR is acceptable, everything else is an error */
	if(res < 0)
	{
		if(errno == EINTR)
			return;
		die("select()");
	}

	if(FD_ISSET(s, &in_fds))
	{
		/* Server socket is in the list, accept() it */
		server_add_client(s);
		res--;
	}

	/* allow 'res' to bail the loop out early if we've serviced all */
	for(p = clients; res > 0 && p != NULL; p = t)
	{
		/* Store p->next incase the client is removed */
		t = p->next;

		/* Check for client sockets with data to be read */
		if(FD_ISSET(p->s, &in_fds))
		{
			server_read_client(p);
			res--;
			/* p might be dead now, service writes later */
			continue;
		}
		/* Check clients with pending data for writeability */
		if(FD_ISSET(p->s, &out_fds))
		{
			server_write_client(p);
			res--;
		}
	}
}

static int make_socket(unsigned short port)
{
	int r, s = 0;
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

	r = fcntl(s, F_SETFL, O_NONBLOCK);
	if(r == -1)
		die("Unable to set non-blocking on bound socket.\n");
}

int new_server(int port)
{
	int s;

	s = make_socket(port);
	listen_on(s);

	return s;
}
