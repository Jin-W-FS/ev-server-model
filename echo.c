#include "evops.h"

static void on_echo_recv(struct bufferevent *bev, void *arg)
{
	
	struct evbuffer* in = bufferevent_get_input(bev);
	struct evbuffer* out= bufferevent_get_output(bev);
	evbuffer_add_buffer(out, in);
}

struct evops proto = {
	.on_recv = on_echo_recv,
};

