/***************************************************************************
 *   $Id: main.c,v 1.3 2008/10/22 16:04:17 pingwin Exp $
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

/* umask */
#include <sys/stat.h>

/* for daemonizing */
#include <pwd.h>
#include <grp.h>

/* libevent */
#include <event.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>

#include "includes/stubtypes.h"
#include "includes/XKSignals.h"

#if defined(FREEBSD)
#include <sys/sysctl.h>
#include <sys/socketvar.h>
#include <netinet/in_pcb.h>
#include <netinet/tcp.h>
#include <netinet/tcp_var.h>
#endif



#ifndef WATCH_PORT
#define WATCH_PORT 25
#endif

#ifndef MAX_CLIENTS
#define MAX_CLIENTS 16
#endif

#ifndef DEFAULT_WORKING_DIR
#define DEFAULT_WORKING_DIR "/tmp/"
#endif

#ifndef DEFAULT_RUNNING_USERNAME
#define DEFAULT_RUNNING_USERNAME "root"
#endif

#ifndef DEFAULT_RUNNING_GROUP
#define DEFAULT_RUNNING_GROUP "adm"
#endif

int init();
void *socket_stream(void *arg);

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

	//printf("[XIG] count: %d len: %d\n", xig->xig_count, xig->xig_len);

	for (xig = (struct xinpgen *)((char *)xig + xig->xig_len);
		xig->xig_len > sizeof(struct xinpgen);
		xig = (struct xinpgen *)((char *)xig + xig->xig_len)) {

		if (ntohs( ((struct xtcpcb *)xig)->xt_inp.inp_lport ) == WATCH_PORT)
			tcp_count ++;
	}
	free(buf);
	return tcp_count;
}
#elif defined(GNU_LINUX)
int port_conn_count() {
	return 1337;
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
	struct sockaddr_in client_addr;
	socklen_t l = sizeof(struct sockaddr);
	pthread_t threadID;
	int bytes_received = 0;
	int key_len = strlen(PASSKEY);
	char *received_key = malloc(key_len+1);
	bzero(received_key, key_len+1);

	int client_sock = accept(fd, (struct sockaddr *)&client_addr, &l);
	bytes_received = recv(client_sock, received_key, key_len, 0);

	if (bytes_received == -1) {
		perror("recv:");
		close(client_sock);
		return;

	} else if (bytes_received == 0) {
		printf("Connection Closed\n");
		close(client_sock);
		return;

	} else if (bytes_received != key_len) {
		printf("Not matching key length\n");
		close(client_sock);
		return;
	}

	// compare the supplied key to what we have
	if (strcmp(PASSKEY, received_key) != 0) {
		printf("Key doesn't match, dropping connection\n");
		close(client_sock);
		return;
	}

	free(received_key);

	// so the key's match, now we do this
	int rc = pthread_create(&threadID, NULL, socket_stream, (void*)client_sock);

	switch(rc) {
	case EAGAIN:	printf("EE Lacking Resources.\n");break;
	case EINVAL:	printf("EE The value specifid by attr is invalid.\n"); break;
	case EPERM:	printf("EE The caller does not have appropriate permission to set the requird scheduling parameters or scheduling policy.\n"); break;
	}
}

void *socket_stream(void *arg) {
	int client_sock = (int)arg;
	int bytes_sent = 0;
	int s = sizeof(struct svr_status_t);

	do {
		bytes_sent = send(client_sock, (const void *)get_current_status(), s, MSG_NOSIGNAL);
		if (bytes_sent == -1) {
			break;
		} else if (bytes_sent == 0) {
			break;
		}
	} while (sleep(2) == 0);

	close(client_sock);
}


/**
 *	@short print usage message
 */
void
printUsage() {
	printf(" -=[ Stub Monitor ]=- \n");
	printf(" -h help (what you see now)\n");
	printf(" -f run in foreground\n");
	exit(EXIT_SUCCESS);
}


/**
 *	@name daemonize
 *	@short fork the current process and return the parent and the child is the new "main" process
 *	@return int status
 */
int
daemonize() {
	pid_t pid;

	// --------------------------------------------------
	// Daemonizing occurs here
	// --------------------------------------------------
	if ( (pid = fork()) < 0 ) {
		return pid; /* error */
	} else if (pid != 0) {
		return 1; /* parent */
	}

	// Get New Pid of Child
	pid = getpid();

	// Become Session Leader
	setsid();

	// Clear File Mode Creation Mask
	umask(0);

	// Change Working Directory
	chdir(DEFAULT_WORKING_DIR);

	// ---------------------------------------------
	// Change the uid and gid
	// ---------------------------------------------
	if (getuid() != 0) {
		printf("Root must exec\n");
		exit(EXIT_FAILURE);
	}

	// get the default or configured effective user id and group id
	struct passwd *euser = getpwnam(DEFAULT_RUNNING_USERNAME);
	if (euser == NULL) {
		perror("Failed to find user info:");
		exit(EXIT_FAILURE);
	}

	struct group *egroup = getgrnam(DEFAULT_RUNNING_GROUP);
	if (egroup == NULL) {
		perror("Failed to find group info:");
		exit(EXIT_FAILURE);
	}


	if (setgid( egroup->gr_gid ) < 0) {
		perror("setgid:");
		exit(EXIT_FAILURE);
	}

	if (setuid( euser->pw_uid ) < 0) {
		perror("setuid:");
		exit(EXIT_FAILURE);
	}

	syslog(LOG_INFO, "Launched at pid (%d)", getpid());

	return pid;
}

/**
 *	@short begin program
 *	@param int the number of arguments passed to binary
 *	@param char** arguements
 *	@return int
 */
int main(int argc, char *argv[]) {
	setlogmask(LOG_DEBUG);

	// --------------------------------------------------
	// Grab CLI Arguements
	// --------------------------------------------------
	int opt;
	int run_foreground = 0;

	while ((opt = getopt(argc, argv, "hf")) != -1) {
		switch (opt) {
		// run in foreground
		case 'f':
			run_foreground = 1;
			break;

		// help
		case 'h':
			printUsage();
			break;
		}
	}

	// --------------------------------------------------
	// alright all alarms are set, now daemonize
	// --------------------------------------------------
	if (run_foreground)
		init();
	else
		switch( daemonize() ) {
			case -1: /* error */
				perror("Failed to fork:");
				syslog(LOG_ERR, "ERRNO: %d :: FAILED TO FORK ", errno);
				break;

			case 1: /* parent */
				break;

			default: /* child */
				// --------------------------------------------------
				// Init XKSignals Handlers
				// --------------------------------------------------
				XK_signals_init(NULL);

				// --------------------------------------------------
				// Start the Server
				// --------------------------------------------------
				init();
				break;
		}
	return EXIT_SUCCESS;
}



int init() {
	struct event ev;

	XK_signals_init(NULL);
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
}

/* newline */
