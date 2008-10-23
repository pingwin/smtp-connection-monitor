/***************************************************************************
 *   $Id: client.c,v 1.4 2008/10/23 18:38:32 pingwin Exp $
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
#include <unistd.h>

#include <event.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "includes/stubtypes.h"

#ifndef MAX_HOSTS
#define MAX_HOSTS 16
#endif

char *host_list[ MAX_HOSTS ];
int num_hosts;

void init_host_list() {
	int i =0;
	num_hosts = 0;
	for (i=0; i<MAX_HOSTS; i ++) {
		host_list[ i ] = malloc(64);
		bzero(host_list[ i ], 64);
	}
}

int add_to_host_list(const char *host) {
	if (num_hosts >= MAX_HOSTS)
		return EXIT_FAILURE;
	strcpy(host_list[ num_hosts ], host);
	num_hosts ++;
}

int load_host_file(const char *host_file) {
	FILE *fd;
	char *buf = malloc(64);
	bzero(buf, 64);


	if ((fd = fopen(host_file, "r")) == NULL)
		return EXIT_FAILURE;


	while (fgets(buf, 64, fd) != NULL) {
		// comment?
		switch(buf[0]) {
		case ';': case '#': case '\r': case '\n':
			bzero(buf, 64);
			continue;
		}

		if (buf[ strlen(buf)-1 ] == '\n')
			buf[ strlen(buf)-1 ] = '\0';
			if (buf[ strlen(buf)-1 ] == '\r')
				buf[ strlen(buf)-1 ] = '\0';

		if (add_to_host_list(buf) < 0) {
			printf("Too many entries in host list: max is (%d)\n", MAX_HOSTS);
			break;
		}
	}

	fclose(fd);
	free(buf);
	return EXIT_SUCCESS;
}


int client_connection(char *arg) {
	int sock;
	int yes = 1;
	int addr_len = sizeof(struct sockaddr);

	struct sockaddr_in dest_addr;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket:");
		return -1;
	}

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons( HOST_PORT );
	dest_addr.sin_addr.s_addr = inet_addr( (const char*)arg );
	bzero(&(dest_addr.sin_zero), 8);

	if (connect(sock, (struct sockaddr *)&dest_addr, addr_len) < 0) {
		perror("connect:");
		return -1;
	}

	if (send(sock, PASSKEY, strlen(PASSKEY), 0) < 0) {
		perror("send:");
		return -1;
	}

	return sock;
}

void socket_read(int sock, short event, void *arg) {
	int bytes_received =0;
	struct svr_status_t status;
// 	struct sockaddr_in *dest_addr = ((struct sockaddr_in *)arg);

	bytes_received = recv(sock, (void *)&status, sizeof(struct svr_status_t), 0);

	if (bytes_received == -1) {
		perror("recv:");
		close(sock);
		return;
	} else if (bytes_received == 0) {
		printf("Connection Closed\n");
		close(sock);
		return;
	}

	printf("Host: %s\tRL: %.2f %.2f %.2f\tC: %d\n",
		(char*)arg,
		status.load[0],
		status.load[1],
		status.load[2],
		status.num_connections
		);
}

void printUsage() {
	printf(" -=[ Stub Watcher ]=-\n");
	printf(" <host> Host IP to watch.\n");
	printf(" -c <confile> Configuration file to load a list of hosts.\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	int opt;
	int i=0;
	int sock;
	struct event ev[ MAX_HOSTS ];
	init_host_list();

	while ((opt = getopt(argc, argv, "hc:")) != -1) {
		switch (opt) {
		// help
		case 'h':
			printUsage();
			break;
		// config file
		case 'c':
			if (load_host_file(optarg) == EXIT_FAILURE) {
				printf("Host File Not Found\n");
				return EXIT_FAILURE;
			}
			break;
		}
	}

	if (num_hosts < 1) {
		if (argc == 2) {
			add_to_host_list(argv[1]);
		} else {
			printUsage();
		}
	}

	event_init();

	printf("Watching %d hosts\n", num_hosts);
	for (i=0; i<num_hosts; i++) {
		printf("Watching %d %s\n", i, host_list[i]);
		sock = client_connection(host_list[i]);
		event_set(&ev[ i ], sock, EV_WRITE | EV_PERSIST, socket_read, host_list[i]);
		event_add(&ev[ i ], NULL);
	}


	printf("Finished adding\n");

	event_dispatch();

	while (sleep(20)==0);

	return EXIT_SUCCESS;
}

/* newline */
