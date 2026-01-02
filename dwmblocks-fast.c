/* SPDX-License-Identifier: ISC */
/* ISC License (ISC)
 *
 * Copyright 2020 torrinfail
 * Copyright 2025-2026 James Tirta Halim <tirtajames45 at gmail dot com>
 * This file is derived from dwmblocks with modifications.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided that
 * the above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#ifndef NO_X
#	include <X11/Xlib.h>
#	include <X11/Xatom.h>
#endif

#ifdef __OpenBSD__
#	define SIGPLUS  SIGUSR1 + 1
#	define SIGMINUS SIGUSR1 - 1
#else
#	define SIGPLUS  SIGRTMIN
#	define SIGMINUS SIGRTMIN
#endif

#define LEN(X)       (sizeof(X) / sizeof(X[0]))
#define GX_CMDLENGTH 64
#define MIN(a, b)    ((a < b) ? a : b)
#define GX_STATUSLEN (LEN(gx_blocks) * GX_CMDLENGTH + 1)

typedef struct Block Block;
typedef enum {
	GX_RET_SUCC = 0,
	GX_RET_ERR
} gx_ret_ty;

#ifndef __OpenBSD__
void
dummysighandler(int num);
#endif
void
sighandler(int num);
void
getcmds(int time, Block *blocks, unsigned int blocks_len, unsigned char *statusbar_len);
void
getsigcmds(unsigned int signal, Block *blocks, unsigned int blocks_len);
gx_ret_ty
setupsignals();
void
sighandler(int signum);
char *
getstatus(char *str);
gx_ret_ty
statusloop();
void
termhandler(int signum);
gx_ret_ty
pstdout(char *status);
#ifndef NO_X
gx_ret_ty
setroot(char *status);
static gx_ret_ty (*writestatus)(char *status) = setroot;
static gx_ret_ty
setupX();
static Display *gx_dpy;
static int gx_screen;
static Window gx_root;
#else
static void (*writestatus)(const char *status, unsigned int length) = pstdout;
#endif

#include "blocks.h"

static char gx_statusbar[LEN(gx_blocks)][GX_CMDLENGTH];
static char gx_statusstr[GX_STATUSLEN];
/* GX_CMDLENGTH fits in an unsigned char. */
static unsigned char gx_statusbarlen[LEN(gx_blocks)];
static int gx_statuscontinue = 1;
static int gx_statuschanged = 0;

/* Run command or execute C function. */
char *
getcmd(Block *block, char *output)
{
	char *dst = output;
	/* Add icon. */
	dst = xstpcpy(dst, block->icon);
	*dst++ = ' ';
	char *old = dst;
	/* Add result of command or C function. */
	dst = block->func(dst, GX_CMDLENGTH - (dst - output), block->command, &block->interval);
	/* No result. Set length to zero. */
	if (dst == old) {
		*output = '\0';
		return output;
	}
	/* Add delimiter. */
	dst = xstpcpy_len(dst, DELIM, DELIMLEN);
	return dst;
}

/* Run commands or functions according to their interval. */
void
getcmds(int time, Block *blocks, unsigned int blocks_len, unsigned char *statusbar_len)
{
	Block *curr = blocks;
	for (unsigned int i = 0; i < blocks_len; ++i, ++curr)
		if ((curr->interval != 0 && (unsigned int)time % curr->interval == 0) || time == -1) {
			char tmp[GX_CMDLENGTH];
			/* Get the result of getcmd. */
			unsigned int tmp_len = getcmd(curr, tmp) - tmp;
			/* Check if needs update. */
			if (tmp_len != statusbar_len[i]
			    || memcmp(tmp, gx_statusbar[i], tmp_len)) {
				/* Get the latest change. */
				xstpcpy_len(gx_statusbar[i], tmp, tmp_len);
				statusbar_len[i] = tmp_len;
				/* Mark change. */
				gx_statuschanged = 1;
			}
		}
}

/* Same as getcmds but executed when receiving a signal. */
void
getsigcmds(unsigned int signal, Block *blocks, unsigned int blocks_len)
{
	Block *curr = blocks;
	for (unsigned int i = 0; i < blocks_len; ++i, ++curr)
		if (curr->signal == signal)
			gx_statusbarlen[i] = getcmd(curr, gx_statusbar[i]) - gx_statusbar[i];
}

gx_ret_ty
setupsignals()
{
#ifndef __OpenBSD__
	/* Initialize all real time signals with dummy handler. */
	for (int i = SIGRTMIN; i <= SIGRTMAX; ++i)
		if (signal(i, dummysighandler) == SIG_ERR)
			ERR(return GX_RET_ERR);
#endif

	for (unsigned int i = 0; i < LEN(gx_blocks); ++i)
		if (gx_blocks[i].signal > 0)
			if (signal(SIGMINUS + (int)gx_blocks[i].signal, sighandler) == SIG_ERR)
				ERR(return GX_RET_ERR);
	return GX_RET_SUCC;
}

/* Construct the status string. */
char *
getstatus(char *dst)
{
	char *p = dst;
	for (unsigned int i = 0; i < LEN(gx_blocks); ++i)
		p = xstpcpy_len(p, gx_statusbar[i], gx_statusbarlen[i]);
	/* Chop last delim, if bar is not empty. */
	if (p != dst)
		*(p -= DELIMLEN) = '\0';
	return p;
}

#ifndef NO_X
static int
gx_XStoreNameLen(Display *dpy, Window w, const char *name, int len)
{
	/* Directly use XChangeProperty to save a strlen. */
	return XChangeProperty(dpy, w, XA_WM_NAME, XA_STRING, 8, PropModeReplace, (_Xconst unsigned char *)name, len);
}

gx_ret_ty
setroot(char *status)
{
	char *p = getstatus(status);
	gx_XStoreNameLen(gx_dpy, gx_root, status, p - status);
	XFlush(gx_dpy);
	gx_statuschanged = 0;
	return GX_RET_SUCC;
}

gx_ret_ty
setupX()
{
	gx_dpy = XOpenDisplay(NULL);
	if (!gx_dpy) {
		fprintf(stderr, "dwmblocks: Failed to open display\n");
		ERR(return GX_RET_ERR);
	}
	gx_screen = DefaultScreen(gx_dpy);
	gx_root = RootWindow(gx_dpy, gx_screen);
	return GX_RET_SUCC;
}
#endif

gx_ret_ty
pstdout(char *status)
{
	char *p = getstatus(status);
	*p++ = '\n';
	unsigned int statuslen = p - status;
	ssize_t ret = write(STDOUT_FILENO, status, statuslen);
	gx_statuschanged = 0;
	if (ret != statuslen)
		ERR(return GX_RET_ERR);
	return GX_RET_SUCC;
}

/* Main loop. */
gx_ret_ty
statusloop()
{
	if (setupsignals() == GX_RET_ERR)
		ERR(return GX_RET_ERR);
	int i = 0;
	getcmds(-1, gx_blocks, LEN(gx_blocks), gx_statusbarlen);
	for (;;) {
		getcmds(i++, gx_blocks, LEN(gx_blocks), gx_statusbarlen);
		if (gx_statuschanged)
			if (writestatus(gx_statusstr) == GX_RET_ERR)
				ERR(return GX_RET_ERR);
		if (!gx_statuscontinue)
			break;
		sleep(1);
	}
	return GX_RET_SUCC;
}

#ifndef __OpenBSD__
/* This signal handler should do nothing. */
void
dummysighandler(int signum)
{
	return;
	(void)signum;
}
#endif

void
sighandler(int signum)
{
	getsigcmds((unsigned int)signum - (unsigned int)SIGPLUS, gx_blocks, LEN(gx_blocks));
	writestatus(gx_statusstr);
}

void
termhandler(int signum)
{
	gx_statuscontinue = 0;
	(void)signum;
}

int
main(int argc, char **argv)
{
	/* Handle command line arguments. */
	for (int i = 0; i < argc; ++i)
		/* Check if printing to stdout. */
		if (!strcmp("-p", argv[i]))
			writestatus = pstdout;
#ifndef NO_X
	if (setupX() == GX_RET_ERR)
		return EXIT_FAILURE;
#endif
	if (signal(SIGTERM, termhandler) == SIG_ERR)
		ERR(return EXIT_FAILURE);
	if (signal(SIGINT, termhandler) == SIG_ERR)
		ERR(return EXIT_FAILURE);
	if (statusloop() == GX_RET_ERR)
		ERR(return EXIT_FAILURE);
#ifndef NO_X
	XCloseDisplay(gx_dpy);
#endif
	return EXIT_SUCCESS;
}
