/* SPDX-License-Identifier: ISC */
/* Copyright 2020 torrinfail
 * Copyright 2025-2026 James Tirta Halim <tirtajames45 at gmail dot com>
 * This file is part of dwmblocks-fast, derived from dwmblocks with
 * modifications.
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
#include <stdlib.h>
#include <assert.h>
#ifndef NO_X
#	include <X11/Xlib.h>
#	include <X11/Xatom.h>
#endif

#include "macros.h"
#include "utils.h"

#ifdef __OpenBSD__
#	define SIGPLUS  SIGUSR1 + 1
#	define SIGMINUS SIGUSR1 - 1
#else
#	define SIGPLUS  SIGRTMIN
#	define SIGMINUS SIGRTMIN
#endif

#define LEN(X)      (sizeof(X) / sizeof(X[0]))
#define G_CMDLENGTH 64
#define G_STATUSLEN (S_LEN(" ") + LEN(g_blocks) * G_CMDLENGTH + 1)

typedef struct Block Block;
typedef enum {
	G_RET_SUCC = 0,
	G_RET_ERR
} g_ret_ty;
typedef enum {
	G_WRITE_STATUSBAR = 0,
	G_WRITE_STDOUT
} g_write_ty;

#ifndef __OpenBSD__
void
dummysighandler(int num);
#endif
void
sighandler(int num);
void
getcmds(unsigned int time, Block *blocks, unsigned int blocks_len, unsigned char *statusbar_len);
void
getsigcmds(unsigned int signal, Block *blocks, unsigned int blocks_len);
g_ret_ty
setupsignals();
void
sighandler(int signum);
char *
getstatus(char *str);
g_ret_ty
writestatus(char *status);
g_ret_ty
statusloop();
void
termhandler(int signum);
#ifndef NO_X
static g_ret_ty
setupX();
static Display *g_dpy;
static int g_screen;
static Window g_root;
static g_write_ty g_write_dst = G_WRITE_STATUSBAR;
#else
static g_write_ty g_write_dst = G_WRITE_STDOUT;
#endif

#include "blocks.h"

static char g_statusbar[LEN(g_blocks)][G_CMDLENGTH];
static char g_statusstr[G_STATUSLEN];
/* G_CMDLENGTH fits in an unsigned char. */
static unsigned char g_statusbarlen[LEN(g_blocks)];
static int g_statuscontinue = 1;
static int g_statuschanged = 0;

/* Run command or execute C function. */
char *
getcmd(Block *block, char *output)
{
	char *dst = output;
	if (block->icon) {
		/* Add icon. */
		dst = xstpcpy(dst, block->icon);
		*dst++ = ' ';
	}
	char *old = dst;
	/* Add result of command or C function. */
	dst = block->func(dst, G_CMDLENGTH - (dst - output), block->command, &block->interval);
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
getcmds(unsigned int time, Block *blocks, unsigned int blocks_len, unsigned char *statusbar_len)
{
	Block *curr = blocks;
	for (unsigned int i = 0; i < blocks_len; ++i, ++curr)
		if ((curr->interval != 0 && (unsigned int)time % curr->interval == 0) || time == (unsigned int)-1) {
			char tmp[G_CMDLENGTH];
			/* Get the result of getcmd. */
			unsigned int tmp_len = getcmd(curr, tmp) - tmp;
			/* Check if needs update. */
			if (tmp_len != statusbar_len[i] || memcmp(tmp, g_statusbar[i], tmp_len)) {
				/* Get the latest change. */
				xstpcpy_len(g_statusbar[i], tmp, tmp_len);
				statusbar_len[i] = tmp_len;
				/* Mark change. */
				g_statuschanged = 1;
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
			g_statusbarlen[i] = getcmd(curr, g_statusbar[i]) - g_statusbar[i];
}

g_ret_ty
setupsignals()
{
#ifndef __OpenBSD__
	/* Initialize all real time signals with dummy handler. */
	for (int i = SIGRTMIN; i <= SIGRTMAX; ++i)
		if (signal(i, dummysighandler) == SIG_ERR)
			ERR(return G_RET_ERR);
#endif

	for (unsigned int i = 0; i < LEN(g_blocks); ++i)
		if (g_blocks[i].signal > 0)
			if (signal(SIGMINUS + (int)g_blocks[i].signal, sighandler) == SIG_ERR)
				ERR(return G_RET_ERR);
	return G_RET_SUCC;
}

/* Construct the status string. */
char *
getstatus(char *dst)
{
	char *dst_s = dst;
	/* Cosmetic: start with a space. */
	*dst++ = ' ';
	char *p = dst;
	for (unsigned int i = 0; i < LEN(g_blocks); ++i)
		p = xstpcpy_len(p, g_statusbar[i], g_statusbarlen[i]);
	/* Chop last delim, if bar is not empty. */
	if (p != dst) {
		*(p -= DELIMLEN) = '\0';
		return p;
	} else {
		return dst_s;
	}
}

#ifndef NO_X
static int
g_XStoreNameLen(Display *dpy, Window w, const char *name, int len)
{
	/* Directly use XChangeProperty to save a strlen. */
	return XChangeProperty(dpy, w, XA_WM_NAME, XA_STRING, 8, PropModeReplace, (_Xconst unsigned char *)name, len);
}

g_ret_ty
setroot(char *status)
{
	char *p = getstatus(status);
	g_XStoreNameLen(g_dpy, g_root, status, p - status);
	XFlush(g_dpy);
	g_statuschanged = 0;
	return G_RET_SUCC;
}

g_ret_ty
setupX()
{
	g_dpy = XOpenDisplay(NULL);
	if (!g_dpy) {
		fprintf(stderr, "dwmblocks: Failed to open display\n");
		ERR(return G_RET_ERR);
	}
	g_screen = DefaultScreen(g_dpy);
	g_root = RootWindow(g_dpy, g_screen);
	return G_RET_SUCC;
}
#endif

g_ret_ty
writestatus(char *status)
{
	char *p = getstatus(status);
#ifndef NO_X
	if (g_write_dst == G_WRITE_STATUSBAR) {
		g_XStoreNameLen(g_dpy, g_root, status, p - status);
		XFlush(g_dpy);
		g_statuschanged = 0;
		return G_RET_SUCC;
	}
#endif
	*p++ = '\n';
	unsigned int statuslen = p - status;
	ssize_t ret = write(STDOUT_FILENO, status, statuslen);
	g_statuschanged = 0;
	if (ret != statuslen)
		ERR(return G_RET_ERR);
	return G_RET_SUCC;
}

/* Main loop. */
g_ret_ty
statusloop()
{
	if (setupsignals() == G_RET_ERR)
		ERR(return G_RET_ERR);
	unsigned int i = 0;
	getcmds((unsigned int)-1, g_blocks, LEN(g_blocks), g_statusbarlen);
	for (;;) {
		getcmds(i++, g_blocks, LEN(g_blocks), g_statusbarlen);
		if (g_statuschanged)
			if (writestatus(g_statusstr) == G_RET_ERR)
				ERR(return G_RET_ERR);
		if (!g_statuscontinue)
			break;
		sleep(1);
	}
	return G_RET_SUCC;
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
	getsigcmds((unsigned int)signum - (unsigned int)SIGPLUS, g_blocks, LEN(g_blocks));
	writestatus(g_statusstr);
}

void
termhandler(int signum)
{
	g_statuscontinue = 0;
	(void)signum;
}

int
main(int argc, char **argv)
{
	/* Handle command line arguments. */
	for (int i = 0; i < argc; ++i)
		/* Check if printing to stdout. */
		if (!strcmp("-p", argv[i]))
			g_write_dst = G_WRITE_STDOUT;
#ifndef NO_X
	if (setupX() == G_RET_ERR)
		return EXIT_FAILURE;
#endif
	if (signal(SIGTERM, termhandler) == SIG_ERR)
		ERR(return EXIT_FAILURE);
	if (signal(SIGINT, termhandler) == SIG_ERR)
		ERR(return EXIT_FAILURE);
	if (statusloop() == G_RET_ERR)
		ERR(return EXIT_FAILURE);
#ifndef NO_X
	XCloseDisplay(g_dpy);
#endif
	return EXIT_SUCCESS;
}
