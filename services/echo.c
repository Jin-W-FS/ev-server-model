#include <arpa/inet.h>
#include <netinet/in.h>
#include "services.h"

static void on_echo_recv(struct bufferevent* bev, void *arg)
{
	struct evbuffer* in = bufferevent_get_input(bev);
	struct evbuffer* out= bufferevent_get_output(bev);
	evbuffer_add_buffer(out, in);
}

static struct sockaddr_in sin = {
	.sin_family = AF_INET,
	.sin_port   = INET_PORT(10007),
};

struct evops echo_proto = {
	.name = "echo 10007/tcp",
	.host = (struct sockaddr*)&sin,
	.sockaddr_len = sizeof(sin),
	.on_recv = on_echo_recv,
};

DECLEAR_SERVICE(echo_proto);

