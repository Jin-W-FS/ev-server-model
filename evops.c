#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <signal.h>

#include "evops.h"

#define CHECK_AND_CALL(fn, ...)	if (fn) fn(__VA_ARGS__)

#define BEV_OPT_INIT_OPT	BEV_OPT_CLOSE_ON_FREE
#define BEV_EVENT_EVENTS (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT | BEV_EVENT_CONNECTED)

/* connect helper struct */
struct _conn_helper
{
	struct evops* ops;
	void* arg;
};

static inline struct _conn_helper* _new_conn_helper(struct evops* ops, void* arg) 
{
	struct _conn_helper* p = malloc(sizeof(*p));
	p->ops = ops;
	p->arg = arg;
	return p;
}

static inline void _del_conn_helper(struct _conn_helper* p) 
{
	free(p);
}

/*  */
static void evops_on_connect(struct bufferevent* bev, short events, void* arg);
int evops_connect(struct event_base* base, struct sockaddr* target, int socket_len, struct evops* ops, void* opsarg)
{
	struct bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_INIT_OPT);
	if (!bev) return -1;
	struct _conn_helper* arg = _new_conn_helper(ops, opsarg);
	if (arg == NULL) return -2;
	bufferevent_setcb(bev, NULL, NULL, evops_on_connect, arg);
	if (bufferevent_socket_connect(bev, target, socket_len) < 0) return -3;
	return 0;
}
static void evops_on_connect(struct bufferevent* bev, short events, void* arg)
{
	struct _conn_helper* p = arg;
	struct evops* ops = p->ops;
	if ((events & BEV_EVENT_EVENTS) == BEV_EVENT_CONNECTED) {
		if (ops->on_connected) {
			ops->on_connected(bev, ops, p->arg);
		} else {
			evops_register(bev, ops, p->arg);
		}
	} else {
		if (ops->on_connect_failed) {
			ops->on_connect_failed(bev, events, p->arg);
		} else {
			bufferevent_free(bev);
		}
	}
	_del_conn_helper(p);
}

/* accept */
void evops_on_accept(struct event_base* base, evutil_socket_t fd, struct evops* ops)
{
	struct bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_INIT_OPT);
	if (!bev) {
		evutil_closesocket(fd);
	} else if (!ops->on_accept) {
		evops_register(bev, ops, NULL);
	} else {
		ops->on_accept(bev, ops);
	}
}

/* close: ack as default on_failure */
void evops_close(struct bufferevent* bev, short events, void* arg)
{
	bufferevent_free(bev);
}

/* whole service */
static void evops_listener_cb(struct evconnlistener*, evutil_socket_t, struct sockaddr*, int socklen, void*);
static void evops_signal_cb(evutil_socket_t, short, void* );

int evops_start_service(struct event_base* _base, struct evops* template) 
{
	struct evops* templates[] = { template, NULL };
	return evops_start_services(_base, templates);
}

int evops_start_services(struct event_base* _base, struct evops* templates[])
{
	int ret = -1;
	struct event_base* base = _base ? _base : event_base_new();
	if (!base) goto clear_0;

	struct event* signal_event = evsignal_new(base, SIGINT, evops_signal_cb, (void* )base);
	if (!signal_event || event_add(signal_event, NULL) < 0) goto clear_1;

	struct evops** entry;
	for (entry = &templates[0];* entry; entry++) {
		struct evops* p =* entry;
		if (p->name) printf("start service: %s\n", p->name);
		p->listener = evconnlistener_new_bind(base, evops_listener_cb, p,
			LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, p->host, p->sockaddr_len);
		if (!p->listener) {
			perror("Err: create listener");
			goto clear_2;
		}
	}
	
	ret = event_base_dispatch(base);

clear_2:
	for (--entry; entry >= &templates[0]; --entry) {
		struct evops* p =* entry;
		evconnlistener_free(p->listener);
	}

	event_free(signal_event);
clear_1:
	if (!_base) event_base_free(base);
clear_0:
	return ret;
}

static void evops_listener_cb(struct evconnlistener* listener, evutil_socket_t fd,
			      struct sockaddr* sa, int socklen, void* arg)
{
	evops_on_accept(evconnlistener_get_base(listener), fd, arg);
}

static void evops_signal_cb(evutil_socket_t sig, short events, void* user_data)
{
	struct event_base* base = user_data;
	struct timeval delay = { 2, 0 };
	event_base_loopexit(base, &delay);
}
