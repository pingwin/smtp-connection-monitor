/***************************************************************************
 *   $Id: main.c,v 1.1.1.1 2008/10/22 12:36:35 pingwin Exp $
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
#include <strings.h>
#include <syslog.h>

#include <sys/types.h>

/* libevent */
#include <event.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h> 

#ifdef FREEBSD
#include <sys/sysctl.h>

#include <sys/socketvar.h>
#include <netinet/in_pcb.h>
#include <netinet/tcp.h>
#include <netinet/tcp_var.h>
#endif

#ifndef WATCH_PORT
#define WATCH_PORT 8000
#endif

#ifndef HOST_PORT
#define HOST_PORT 2120
#endif

#ifndef MAX_CLIENTS
#define MAX_CLIENTS 16
#endif



struct svr_status_t {
	double load[3];
	int num_connections;
};


#if defined(FREEBSD)
int port_conn_count() {
	size_t len  =0;
	int tcp_count = 0;
	char *buf;
	struct xinpgen *xig;
	
	// find out the size of the buffer
	if (sysctlbyname("net.inet.tcp.pcblist", 0, &len, 0, 0) < 0) {
		perror("sysctlbyname");
		return EXIT_FAILURE;
	}

	// malloc for the buffer we are about to receive
	if ((buf = malloc(len)) == 0) {
		perror("malloc");
		return EXIT_FAILURE;
	}

	// and take it!
	if (sysctlbyname("net.inet.tcp.pcblist", buf, &len, 0, 0) < 0) {
		perror("sysctlbyname");
		return EXIT_FAILURE;
	}

	xig = (struct xinpgen *)buf;
//	printf("[XIG] count: %d len: %d\n", xig->xig_count, xig->xig_len);

	for (xig = (struct xinpgen *)((char *)xig + xig->xig_len);
		xig->xig_len > sizeof(struct xinpgen);
		xig = (struct xinpgen *)((char *)xig + xig->xig_len)) {

		if (ntohs( ((struct xtcpcb *)xig)->xt_inp.inp_lport ) == WATCH_PORT)
			tcp_count ++;
	}
	free(buf);
	return tcp_count;
}
#elif defined(LINUX)
int port_conn_count() {
	return 0;
}
#else
#error Do not understand other systems
#endif


struct svr_status_t *get_current_status() {
	struct svr_status_t *status = malloc(sizeof(struct svr_status_t));
	if (getloadavg((status->load), 3) < 0) {
		perror("getloadavg");
		return NULL;
	}
	
	status->num_connections = port_conn_count();
	
	return status;
}


void socket_read(int fd, short event, void *arg) {
	socklen_t l = sizeof(struct sockaddr);
	int bytes_received = 0;
	char *buf = malloc(512);
	bzero(buf, 512);
	
	struct sockaddr_in client_addr;
	int client_sock = accept(fd, (struct sockaddr *)&client_addr, &l);
	bytes_received = recv(client_sock, buf, 511, 0);
	
	if (bytes_received == -1) {
		perror("recv:");
		return;
	} else if (bytes_received == 0) {
		printf("Connection Closed\n");
		return;
	}
	
	printf("Recieved: '%s'\n", buf);
//	struct svr_status_t *status = get_current_status();
	send(client_sock, (const void *)get_current_status(), sizeof(struct svr_status_t), 0);
	//send(client_sock, (const void *)status, sizeof(struct svr_status_t), 0);

	free(buf);
}


/**
 *	@short begin program
 *	@param int the number of arguments passed to binary
 *	@param char** arguements
 *	@return int
 */
int main(int argc, char *argv[]) {
	setlogmask(LOG_DEBUG);
	struct event ev;

	event_init();
	
	int sock;
	struct sockaddr_in svr_addr;
	int yes = 1;
	int addrLen = sizeof(struct sockaddr);
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket:");
		return EXIT_FAILURE;
	}
	
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		perror("setsockopt:");
		return EXIT_FAILURE;
	}
	
	svr_addr.sin_family = AF_INET;
	svr_addr.sin_port = htons( HOST_PORT );
	
	svr_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(svr_addr.sin_zero), 8);
	
	if (bind(sock, (struct sockaddr *)&svr_addr, addrLen) < 0) {
		perror("bind:");
		return EXIT_FAILURE;
	}
	
	if (listen(sock, MAX_CLIENTS) < 0) {
		perror("listen:");
		return EXIT_FAILURE;
	}
	
	event_set(&ev, sock, EV_READ | EV_PERSIST, socket_read, NULL);
	event_add(&ev, NULL);
	
	event_dispatch();
	
	
	return EXIT_SUCCESS;
}

/* newline */
