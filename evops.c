#include <stdlib.h>

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
static void evops_on_connect(struct bufferevent *bev, short events, void *arg);
int evops_connect(struct event_base *base, struct sockaddr* target, int socket_len, struct evops* ops, void* opsarg)
{
	struct bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_INIT_OPT);
	if (!bev) return -1;
	struct _conn_helper* arg = _new_conn_helper(ops, opsarg);
	if (arg == NULL) return -2;
	bufferevent_setcb(bev, NULL, NULL, evops_on_connect, arg);
	if (bufferevent_socket_connect(bev, target, socket_len) < 0) return -3;
	return 0;
}
static void evops_on_connect(struct bufferevent *bev, short events, void *arg)
{
	struct _conn_helper* p = arg;
	struct evops* ops = p->ops;
	if ((events & BEV_EVENT_EVENTS) != BEV_EVENT_CONNECTED) {
		CHECK_AND_CALL(ops->on_connect_failed, bev, events, p->arg);
	} else {
		CHECK_AND_CALL(ops->on_connected, bev, p->arg);
	}
	_del_conn_helper(p);
}

/* accept */
int evops_on_accept(struct event_base* base, evutil_socket_t fd, struct evops* ops)
{
	struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_INIT_OPT);
	if (!bev) return -1;
	void* arg = NULL;
	if (ops->on_accept) arg = ops->on_accept(bev);
	evops_register(bev, ops, arg);
	return 0;
}

/* close: ack as default on_failure */
void evops_close(struct bufferevent *bev, short events, void *arg)
{
	bufferevent_free(bev);
}

/* whole service */
static void evops_listener_cb(struct evconnlistener*, evutil_socket_t, struct sockaddr*, int socklen, void*);
static void evops_signal_cb(evutil_socket_t, short, void *);

int evops_start_service(struct event_base *_base, struct sockaddr* host, int socklen, struct evops* template)
{
	int ret = -1;
	struct event_base* base = _base ? _base : event_base_new();
	if (!base) goto clear_0;

	struct event *signal_event = evsignal_new(base, SIGINT, evops_signal_cb, (void *)base);
	if (!signal_event || event_add(signal_event, NULL) < 0) goto clear_1;

	struct evconnlistener *listener = evconnlistener_new_bind(base, evops_listener_cb, template,
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, host, socklen);
	if (!listener) goto clear_2;

	ret = event_base_dispatch(base);

	evconnlistener_free(listener);
clear_2:
	event_free(signal_event);
clear_1:
	if (!_base) event_base_free(base);
clear_0:
	return ret;
}

static void evops_listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
			      struct sockaddr *sa, int socklen, void *arg)
{
	if (evops_on_accept(evconnlistener_get_base(listener), fd, arg) < 0) {
		evutil_closesocket(fd);
	}
}

static void evops_signal_cb(evutil_socket_t sig, short events, void *user_data)
{
	struct event_base *base = user_data;
	struct timeval delay = { 2, 0 };
	event_base_loopexit(base, &delay);
}
