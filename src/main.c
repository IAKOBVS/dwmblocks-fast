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

/* Must be at the top. */
#include "../include/config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#ifdef USE_X11
#	include <X11/Xlib.h>
#	include <X11/Xatom.h>
#endif

#include "../include/macros.h"
#include "../include/utils.h"

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
g_handler_sig_dummy(int num);
#endif
void
g_getcmds(unsigned int time, g_block_ty *blocks, unsigned int blocks_len, unsigned char *statusbar_len);
void
g_getcmds_sig(unsigned int signal, g_block_ty *blocks, unsigned int blocks_len);
g_ret_ty
g_setup_signals();
void
g_handler_sig(int signum);
char *
g_status_get(char *str);
g_ret_ty
g_status_write(char *status);
g_ret_ty
g_status_mainloop();
void
g_handler_term(int signum);
#ifdef USE_X11
static g_ret_ty
g_setup_x11();
static Display *g_dpy;
static int g_screen;
static Window g_win_root;
static g_write_ty g_write_dst = G_WRITE_STATUSBAR;
#else
static g_write_ty g_write_dst = G_WRITE_STDOUT;
#endif
pthread_mutex_t g_mutex;

static char g_statusbar[LEN(g_blocks)][G_CMDLENGTH];
static char g_statusstr[G_STATUSLEN];
/* G_CMDLENGTH fits in an unsigned char. */
static unsigned char g_statusbarlen[LEN(g_blocks)];
static int g_statuscontinue = 1;
static int g_statuschanged = 0;

/* Run command or execute C function. */
char *
g_getcmd(g_block_ty *block, char *output)
{
	char *dst = output;
	if (block->icon) {
		/* Add icon. */
		dst = u_stpcpy(dst, block->icon);
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
	dst = u_stpcpy_len(dst, DELIM, DELIMLEN);
	return dst;
}

/* Run commands or functions according to their interval. */
void
g_getcmds(unsigned int time, g_block_ty *blocks, unsigned int blocks_len, unsigned char *statusbar_len)
{
	g_block_ty *curr = blocks;
	for (unsigned int i = 0; i < blocks_len; ++i, ++curr)
		if ((curr->interval != 0 && (unsigned int)time % curr->interval == 0) || time == (unsigned int)-1) {
			char tmp[sizeof(g_statusbar[0])];
			/* Get the result of g_getcmd. */
			unsigned int tmp_len = g_getcmd(curr, tmp) - tmp;
			/* Check if needs update. */
			if (tmp_len != statusbar_len[i] || memcmp(tmp, g_statusbar[i], tmp_len)) {
				/* Get the latest change. */
				u_stpcpy_len(g_statusbar[i], tmp, tmp_len);
				statusbar_len[i] = tmp_len;
				/* Mark change. */
				g_statuschanged = 1;
			}
		}
}

/* Same as g_getcmds but executed when receiving a signal. */
void
g_getcmds_sig(unsigned int signal, g_block_ty *blocks, unsigned int blocks_len)
{
	g_block_ty *curr = blocks;
	for (unsigned int i = 0; i < blocks_len; ++i, ++curr)
		if (curr->signal == signal)
			g_statusbarlen[i] = g_getcmd(curr, g_statusbar[i]) - g_statusbar[i];
}

g_ret_ty
g_setup_signals()
{
#ifndef __OpenBSD__
	/* Initialize all real time signals with dummy handler. */
	for (int i = SIGRTMIN; i <= SIGRTMAX; ++i)
		if (signal(i, g_handler_sig_dummy) == SIG_ERR)
			ERR(return G_RET_ERR);
#endif
	for (unsigned int i = 0; i < LEN(g_blocks); ++i)
		if (g_blocks[i].signal > 0)
			if (signal(SIGMINUS + (int)g_blocks[i].signal, g_handler_sig) == SIG_ERR)
				ERR(return G_RET_ERR);
	if (signal(SIGTERM, g_handler_term) == SIG_ERR)
		ERR(return G_RET_ERR);
	if (signal(SIGINT, g_handler_term) == SIG_ERR)
		ERR(return G_RET_ERR);
	return G_RET_SUCC;
}

/* Construct the status string. */
char *
g_status_get(char *dst)
{
	char *dst_s = dst;
	/* Cosmetic: start with a space. */
	*dst++ = ' ';
	char *p = dst;
	for (unsigned int i = 0; i < LEN(g_blocks); ++i)
		p = u_stpcpy_len(p, g_statusbar[i], g_statusbarlen[i]);
	/* Chop last delim, if bar is not empty. */
	if (p != dst)
		p -= DELIMLEN;
	else
		p = dst_s;
	*p = '\0';
	return p;
}

#ifdef USE_X11
static ATTR_INLINE int
g_XStoreNameLen(Display *dpy, Window w, const char *name, int len)
{
	/* Directly use XChangeProperty to save a strlen. */
	return XChangeProperty(dpy, w, XA_WM_NAME, XA_STRING, 8, PropModeReplace, (_Xconst unsigned char *)name, len);
}

g_ret_ty
g_setup_x11()
{
	pthread_mutex_init(&g_mutex, NULL);
	g_dpy = XOpenDisplay(NULL);
	if (!g_dpy) {
		fprintf(stderr, "dwmblocks-fast: Failed to open display.\n");
		ERR(return G_RET_ERR);
	}
	g_screen = DefaultScreen(g_dpy);
	g_win_root = RootWindow(g_dpy, g_screen);
	return G_RET_SUCC;
}
#endif

g_ret_ty
g_status_write(char *status)
{
	char *p = g_status_get(status);
#ifdef USE_X11
	if (g_write_dst == G_WRITE_STATUSBAR) {
		g_XStoreNameLen(g_dpy, g_win_root, status, p - status);
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

static g_ret_ty
g_status_init()
{
	if (g_setup_signals() == G_RET_ERR)
		ERR(return G_RET_ERR);
#ifdef USE_X11
	if (g_setup_x11() == G_RET_ERR)
		return G_RET_ERR;
#endif
	return G_RET_SUCC;
}

static void
g_status_cleanup()
{
#ifdef USE_X11
	XCloseDisplay(g_dpy);
#endif
}

/* Main loop. */
g_ret_ty
g_status_mainloop()
{
	unsigned int i = 0;
	g_getcmds((unsigned int)-1, g_blocks, LEN(g_blocks), g_statusbarlen);
	for (;;) {
		g_getcmds(i++, g_blocks, LEN(g_blocks), g_statusbarlen);
		if (g_statuschanged)
			if (g_status_write(g_statusstr) == G_RET_ERR)
				ERR(return G_RET_ERR);
		if (!g_statuscontinue)
			break;
#ifdef TEST
		return G_RET_SUCC;
#endif
		sleep(1);
	}
	return G_RET_SUCC;
}

#ifndef __OpenBSD__
/* Handle errors gracefully. */
void
g_handler_sig_dummy(int signum)
{
	char buf[S_LEN("dwmblocks-fast: sending unknown signal: ") + sizeof(size_t) * 8 + S_LEN("\n") + 1];
	char *p = buf;
	p = u_stpcpy_len(p, S_LITERAL("dwmblocks-fast: sending unknown signal: "));
	if (signum < 0)
		*p++ = '-';
	p = u_utoa_p((unsigned int)signum, buf);
	*p++ = '\n';
	*p = '\0';
	/* fprintf is not reentrant. */
	assert(write(STDERR_FILENO, buf, (size_t)(p - buf)) != (p - buf));
}
#endif

void
g_handler_sig(int signum)
{
	pthread_mutex_lock(&g_mutex);
	g_getcmds_sig((unsigned int)signum - (unsigned int)SIGPLUS, g_blocks, LEN(g_blocks));
	g_status_write(g_statusstr);
	pthread_mutex_unlock(&g_mutex);
}

void
g_handler_term(int signum)
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
	if (g_status_init() == G_RET_ERR)
		ERR(return EXIT_FAILURE);
	if (g_status_mainloop() == G_RET_ERR)
		ERR(return EXIT_FAILURE);
	g_status_cleanup();
	return EXIT_SUCCESS;
}
