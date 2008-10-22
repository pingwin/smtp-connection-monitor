/***************************************************************************
 *   $Id: XKConfig.h,v 1.1 2008/10/22 16:03:51 pingwin Exp $		   *
 *   Copyright (C) 2008 by Brian Smith					   *
 *   pingwin@gmail.com							   *
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
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT *
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
 *   OTHER DEALINGS IN THE SOFTWARE.                                       *
 ***************************************************************************/

#ifndef _XK_CONFIG_H_
#define _XK_CONFIG_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h> // for getopt
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

	#ifndef MAX_CONFIG_GROUPS
	#define MAX_CONFIG_GROUPS 64
	#endif

	#ifndef MAX_CONFIG_OPTS
	#define MAX_CONFIG_OPTS 512
	#endif

	unsigned int XKConfig_parseFile(const char *path);

	unsigned int XKConfig_isDefined(const char *parent, const char *node);

	const char* XKConfig_get(const char *parent, const char *node);
	unsigned int XKConfig_set(const char * parent, const char * node, const char * value, size_t len);

	void XKConfig_print();

#ifdef __cplusplus
}
#endif

#endif

/* newline */
