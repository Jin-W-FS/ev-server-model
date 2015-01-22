#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "services.h"

static struct sockaddr_in target = {
	.sin_family = AF_INET,
	.sin_port   = INET_PORT(10007),
};

struct pipe
{
	struct bufferevent* bev[2];
};

static void pipe_on_accept(struct bufferevent *bev, struct evops* ops)
{
	struct pipe* p = malloc(sizeof(*p));
	p->bev[0] = bev;
	p->bev[1] = NULL;
	if (evops_connect(bufferevent_get_base(bev), (struct sockaddr*)&target, sizeof(target), ops, p) < 0) {
		free(p);
		bufferevent_free(bev);
	}
}

static void pipe_on_connected(struct bufferevent* bev, struct evops* ops, void* arg)
{
	struct pipe* p = arg;
	p->bev[1] = bev;
	evops_register(p->bev[0], ops, p);
	evops_register(p->bev[1], ops, p);
}

static void pipe_on_recv(struct bufferevent* bev, void* arg)
{
	struct pipe* p = arg;
	struct bufferevent* dst = p->bev[bev == p->bev[0]];
	evbuffer_add_buffer(bufferevent_get_output(dst), bufferevent_get_input(bev));
}

static void pipe_on_connect_failed(struct bufferevent* bev, short events, void* arg)
{
	struct pipe* p = arg;
	bufferevent_free(bev);
	bufferevent_free(p->bev[0]);
	free(p);
}

static void pipe_on_failure(struct bufferevent* bev, short events, void* arg)
{
	struct pipe* p = arg;
	evops_unregister(p->bev[0]);
	evops_unregister(p->bev[1]);
	bufferevent_free(p->bev[0]);
	bufferevent_free(p->bev[1]);
	free(p);
}

static struct sockaddr_in sin = {
	.sin_family = AF_INET,
	.sin_port   = INET_PORT(10008),
};

struct evops pipe_proto = {
	.name = "pipe-to-echo 10008/tcp",
	.host = (struct sockaddr*)&sin,
	.sockaddr_len = sizeof(sin),
	.on_accept = pipe_on_accept,
	.on_connected = pipe_on_connected,
	.on_connect_failed = pipe_on_connect_failed,
	.on_recv = pipe_on_recv,
	.on_failure = pipe_on_failure,
};

DECLEAR_SERVICE(pipe_proto);
