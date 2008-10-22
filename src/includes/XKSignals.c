/***************************************************************************
 *   $Id: XKSignals.c,v 1.1 2008/10/22 16:03:51 pingwin Exp $
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

#include "XKSignals.h"
#include <syslog.h>

struct {
	unsigned int signalLog[66];

	unsigned int signalReceived:1;
	int lastSignalReceived;

	unsigned int shutdownInitiated:1;

	struct sigaction handler;
} XK_signals;


/**
 * @name InterruptSignalHandler
 * @short handle signals
 * @param int signal received
 */
void
InterruptSignalHandler(int signal) {
	XK_signals.signalLog[ signal ] ++;
	XK_signals.lastSignalReceived = signal;

	switch(signal) {
	case SIGINT:
		if (XK_signals.signalLog[ SIGINT ] >= 2) {
			#ifdef AppShutdown
			AppShutdown(EXIT_FAILURE);
			#else
			exit(EXIT_FAILURE);
			#endif
		}


		printf("Received CTRL+C\n");
		break;
/*
	case SIGSEGV:
		printf("Received SIGSEGV: trying to continue\n");
		break;
*/
	default:
		XK_signals.signalReceived = 1;
		syslog(LOG_ERR, "Received Signal (%d)", signal);
		#ifdef AppShutdown
		AppShutdown(EXIT_FAILURE);
		#else
		exit(EXIT_FAILURE);
		#endif
	}
}



/**
 * @name XK_signals_init
 * @short initialize signal handlers
 */
void
XK_signals_init(void (*funcPtr)(int signal)) {
	unsigned int i = 0;

	// --------------------------------------------------
	// if not function pointer is passed
	// then assigndefault local func
	// --------------------------------------------------
	if (funcPtr == NULL) {
		funcPtr = InterruptSignalHandler;
	}

	// --------------------------------------------------
	// setup signal handlers
	// --------------------------------------------------
	XK_signals.handler.sa_handler = funcPtr;

	if (sigfillset(&XK_signals.handler.sa_mask) < 0) {
		syslog(LOG_ERR, "sigfillset failed.");
		#ifdef AppShutdown
		AppShutdown(EXIT_FAILURE);
		#else
		exit(EXIT_FAILURE);
		#endif
	}

	XK_signals.handler.sa_flags = 0;

	// --------------------------------------------------
	// zero out signalLog
	// --------------------------------------------------
	for(i=0; i<=65; i++)	XK_signals.signalLog[i] = 0;

	XK_signals.lastSignalReceived = 0;
	XK_signals.shutdownInitiated = 0;

	// --------------------------------------------------
	// Interrupt (ANSI).
	// --------------------------------------------------
	if (sigaction(SIGINT, &XK_signals.handler, 0) < 0) {
		syslog(LOG_ERR, "signaction setup SIGINT failed.");
		#ifdef AppShutdown
		AppShutdown(EXIT_FAILURE);
		#else
		exit(EXIT_FAILURE);
		#endif
	}

	// --------------------------------------------------
	// Illegal instruction (ANSI).
	// --------------------------------------------------
	if (sigaction(SIGILL, &XK_signals.handler, 0) < 0) {
		syslog(LOG_ERR, "signaction setup SIGINT failed.");
		#ifdef AppShutdown
		AppShutdown(EXIT_FAILURE);
		#else
		exit(EXIT_FAILURE);
		#endif
	}

	// --------------------------------------------------
	// Abort (ANSI).
	// --------------------------------------------------
	if (sigaction(SIGABRT, &XK_signals.handler, 0) < 0) {
		syslog(LOG_ERR, "signaction setup SIGABRT failed.");
		#ifdef AppShutdown
		AppShutdown(EXIT_FAILURE);
		#else
		exit(EXIT_FAILURE);
		#endif
	}

	// --------------------------------------------------
	// Hangup (POSIX).
	// --------------------------------------------------
	if (sigaction(SIGHUP, &XK_signals.handler, 0) < 0) {
		syslog(LOG_ERR, "signaction setup SIGHUP failed.");
		#ifdef AppShutdown
		AppShutdown(EXIT_FAILURE);
		#else
		exit(EXIT_FAILURE);
		#endif
	}

	// --------------------------------------------------
	// Segmentation violation (ANSI).
	// --------------------------------------------------
	if (sigaction(SIGSEGV, &XK_signals.handler, 0) < 0) {
		syslog(LOG_ERR, "signaction setup SIGSEGV failed.");
		#ifdef AppShutdown
		AppShutdown(EXIT_FAILURE);
		#else
		exit(EXIT_FAILURE);
		#endif
	}

// #ifdef SIGPIPE
	// --------------------------------------------------
	// Broken Pipe
	// --------------------------------------------------
	if (sigaction(SIGPIPE, &XK_signals.handler, 0) < 0) {
		syslog(LOG_ERR, "signaction setup SIGPIPE failed.");
		#ifdef AppShutdown
		AppShutdown(EXIT_FAILURE);
		#else
		exit(EXIT_FAILURE);
		#endif
	}
// #endif

#ifdef SIGSTKFLT
	// --------------------------------------------------
	// Stack fault. Signal doesn't exist for OpenBSD
	// --------------------------------------------------
	if (sigaction(SIGSTKFLT, &XK_signals.handler, 0) < 0) {
		syslog(LOG_ERR, "signaction setup SIGSTKFLT failed.");
		#ifdef AppShutdown
		AppShutdown(EXIT_FAILURE);
		#else
		exit(EXIT_FAILURE);
		#endif
	}
#endif
}

/* newline */
