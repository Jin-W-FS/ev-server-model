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

extern struct evops* services[];

int main(int argc, char* argv[])
{
	printf("[PID] %d\n", getpid());
	int ret = evops_start_services(NULL, services);
	return ret;
}
