#ifndef EVENT_HANDLER_CLASS_H
#define EVENT_HANDLER_CLASS_H

#include <endian.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>

#if __BYTE_ORDER == __BIG_ENDIAN
#define INET_PORT(p)	(p)
#else
#define INET_PORT(p)	(((p & 0xff) << 8) | ((p >> 8) & 0xff))
#endif

struct evops
{
	const char* name;
	struct sockaddr* host;
	int sockaddr_len;
	/* for saving the listener. no need to assign */
	struct evconnlistener* listener;
	/* accept() */
	void (*on_accept)(struct bufferevent* bev, struct evops* ops);
	/* connect() -> arg */
	void (*on_connected)(struct bufferevent* bev, struct evops* ops, void* arg);
	void (*on_connect_failed)(struct bufferevent* bev, short events, void* arg);
	/* bufferevent_data_cb */
	void (*on_recv)(struct bufferevent* bev, void* arg);
	void (*on_send)(struct bufferevent* bev, void* arg);
	/* bufferevent_event_cb */
	void (*on_failure)(struct bufferevent* bev, short events, void* arg);
};

void evops_on_accept(struct event_base* base, evutil_socket_t fd, struct evops* ops);
int evops_connect(struct event_base* base, struct sockaddr* target, int socklen, struct evops* ops, void* opsarg);
int evops_start_service(struct event_base* base, struct evops* template);
int evops_start_services(struct event_base* base, struct evops* templates[]);
void evops_close(struct bufferevent* bev, short events, void* arg);

static inline void evops_register(struct bufferevent* bev, struct evops* ops, void* data) {
	bufferevent_event_cb on_failure = ops->on_failure ? ops->on_failure : evops_close;
	bufferevent_setcb(bev, ops->on_recv, ops->on_send, on_failure, data);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
}
static inline void evops_unregister(struct bufferevent* bev) {
	bufferevent_disable(bev, EV_READ | EV_WRITE);
}

#endif
