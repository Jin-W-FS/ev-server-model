#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "evops.h"

extern struct evops proto;

#define PORT 12345

int main(int argc, char* argv[])
{
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port   = htons(PORT),
	};
	printf("[PID] %d\n", getpid());
	int ret = evops_start_service(NULL, (struct sockaddr*)&sin, sizeof(sin), &proto);
	return ret;
}
