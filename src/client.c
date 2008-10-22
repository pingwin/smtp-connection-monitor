/***************************************************************************
 *   $Id: client.c,v 1.1 2008/10/22 16:03:51 pingwin Exp $
 *   Copyright (C) 2008 by Brian Smith   *
 *   pingwin@gmail.com   *
 *                                                                         *
 *   Permission is hereby granted, free of charge, to any person obtaining *
 *   a copy of this software and associated documentation files (the       *
 *   "Software"), to deal in the Software without restriction, including   *
 *   without limitation the rights to use, copy, modify, merge, publish,   *
 *   distribute, sublicense, and/or sell copies of the Software, and to    *
 *   permit persons to whom the Software is furnished to do so, subject to *
 *   the following conditions:                                             *
 *                                                                         *
 *   The above copyright notice and this permission notice shall be        *
 *   included in all copies or substantial portions of the Software.       *
 *                                                                         *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. *
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
 *   OTHER DEALINGS IN THE SOFTWARE.                                       *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <syslog.h>
#include <unistd.h>


/* libevent */
#include <event.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#include "includes/stubtypes.h"
#include "includes/XKConfig.h"

void socket_read(int fd, short event, void *arg) {
	socklen_t l = sizeof(struct sockaddr);
	int bytes_received = 0;
	struct svr_status_t status;
	struct sockaddr_in *remote_addr = ((struct sockaddr_in *)&arg);


	while (1) {
		bytes_received = recv(fd, (void *)&status, sizeof(struct svr_status_t), 0);

		if (bytes_received == -1) {
			perror("recv:");
			close(fd);
			return;
		} else if (bytes_received == 0) {
			printf("Connection Closed\n");
			close(fd);
			return;
		}

		printf("Host: %s Remove Load: %.2f %.2f %.2f Num Connections: %d\n",
			inet_ntoa(remote_addr->sin_addr),
			status.load[0],
			status.load[1],
			status.load[2],
			status.num_connections
			);
	}

}

int client_connection(const char *host, struct sockaddr_in *dest_addr) {
	int sock;
	int yes = 1;
	int addr_len = sizeof(struct sockaddr);

	if (dest_addr == NULL) {
		dest_addr = malloc(addr_len);
	}

	bzero(dest_addr, addr_len);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket:");
		return -1;
	}

	dest_addr->sin_family = AF_INET;
	dest_addr->sin_port = htons( HOST_PORT );
	dest_addr->sin_addr.s_addr = inet_addr( host );
	bzero(&(dest_addr->sin_zero), 8);

	if (connect(sock, (struct sockaddr *)dest_addr, addr_len) < 0) {
		perror("connect:");
		return -1;
	}

	int bytes_received = 0;
	if ((bytes_received = send(sock, PASSKEY, strlen(PASSKEY), 0)) < 0) {
		perror("send:");
		return -1;
	}

	return sock;
}

void printUsage() {
	printf(" -=[ Stub Watcher ]=-\n");
	printf(" <host> Host IP to watch.\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

	if (argc != 2) printUsage();

	struct event ev;

	event_init();

	struct sockaddr_in dest_addr;
	int sock = client_connection(argv[1], &dest_addr);
	if (sock < 0) {
		perror("client_connection");
		return EXIT_FAILURE;
	}

	event_set(&ev, sock, EV_WRITE | EV_PERSIST, socket_read, (void *)&dest_addr);
	event_add(&ev, NULL);

	event_dispatch();

	return EXIT_SUCCESS;
}

/* newline */
