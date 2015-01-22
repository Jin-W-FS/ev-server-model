#include <string.h>
#include <time.h>
#include "services.h"

static void datetime_on_accept(struct bufferevent *bev, struct evops* ops)
{
	char buff[200];
	time_t t = time(NULL);
	if (!ctime_r(&t, buff)) goto failed;
	if (bufferevent_write(bev, buff, strlen(buff)) < 0) goto failed;
	evops_register(bev, ops, NULL);
	return;
failed:
	bufferevent_free(bev);
	return;
}

void datetime_on_send(struct bufferevent *bev, void *arg)
{
	if (bufferevent_flush(bev, EV_WRITE, BEV_FINISHED) <= 0) {
		bufferevent_free(bev);
	}
}

static struct sockaddr_in sin = {
	.sin_family = AF_INET,
	.sin_port   = INET_PORT(10013),
};

struct evops datetime_proto = {
	.name = "datetime 10013/tcp",
	.host = (struct sockaddr*)&sin,
	.sockaddr_len = sizeof(sin),
	.on_accept = datetime_on_accept,
	.on_send = datetime_on_send,
};

DECLEAR_SERVICE(datetime_proto);
