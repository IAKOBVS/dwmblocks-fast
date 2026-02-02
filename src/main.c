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

#ifdef USE_X11
#	include <X11/Xlib.h>
#	include <X11/Xatom.h>
#endif

#include "../include/blocks.h"
#include "../include/macros.h"
#include "../include/utils.h"
#include "../include/path.h"

#define DO_CLEANUP 0

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

int
g_getcmds(unsigned int time, g_block_ty *blocks, unsigned int blocks_len, unsigned char *statusbar_len);
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

static char g_statusbar[LEN(g_blocks)][G_CMDLENGTH];
static char g_statusstr[G_STATUSLEN];
/* G_CMDLENGTH fits in an unsigned char. */
static unsigned char g_statusbarlen[LEN(g_blocks)];

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
int
g_getcmds(unsigned int time, g_block_ty *blocks, unsigned int blocks_len, unsigned char *statusbar_len)
{
	int changed = 0;
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
				changed = 1;
			}
		}
	return changed;
}

g_ret_ty
g_setup_signals()
{
	if (unlikely(signal(SIGTERM, g_handler_term) == SIG_ERR))
		DIE(return G_RET_ERR);
	if (unlikely(signal(SIGINT, g_handler_term) == SIG_ERR))
		DIE(return G_RET_ERR);
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
	g_dpy = XOpenDisplay(NULL);
	if (unlikely(g_dpy == NULL)) {
		fprintf(stderr, "dwmblocks-fast: Failed to open display.\n");
		DIE(return G_RET_ERR);
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
	switch (g_write_dst) {
	case G_WRITE_STATUSBAR:;
		g_XStoreNameLen(g_dpy, g_win_root, status, p - status);
		XFlush(g_dpy);
		break;
	case G_WRITE_STDOUT: {
		*p++ = '\n';
		const unsigned int statuslen = p - status;
		ssize_t ret = write(STDOUT_FILENO, status, statuslen);
		if (unlikely(ret != statuslen))
			DIE(return G_RET_ERR);
		break;
	}
	}
	return G_RET_SUCC;
}

int
starts_with(const char *s1, const char *s2)
{
	for (; *s1 == *s2 && *s2; ++s1, ++s2) {}
	return *s2 == '\0';
}

/* Update hwmon/hwmon[0-9]* and thermal/thermal_zone[0-9]* to point to
 * the real file, given that the number may change between reboots. */
static g_ret_ty
g_paths_sysfs_resolve()
{
	for (unsigned int i = 0; i < LEN(g_blocks); ++i) {
		if (g_blocks[i].command && starts_with(g_blocks[i].command, "/sys/")) {
			char *p = path_sysfs_resolve(g_blocks[i].command);
			if (unlikely(p == NULL))
				DIE();
			if (p != g_blocks[i].command) {
				DBG(fprintf(stderr, "%s:%d:%s: %s doesn't exist, resolved to %s (which is malloc'd).\n", __FILE__, __LINE__, ASSERT_FUNC, g_blocks[i].command, p));
				/* Set new path. */
				g_blocks[i].command = p;
			} else {
				DBG(fprintf(stderr, "%s:%d:%s %s exists.\n", __FILE__, __LINE__, ASSERT_FUNC, p));
			}
		}
	}
	return G_RET_SUCC;
}

static g_ret_ty
g_status_init()
{
	if (unlikely(g_setup_signals() != G_RET_SUCC))
		DIE(return G_RET_ERR);
#ifdef USE_X11
	if (unlikely(g_setup_x11() != G_RET_SUCC))
		DIE(return G_RET_ERR);
#endif
	if (unlikely(g_paths_sysfs_resolve() != G_RET_SUCC))
		DIE(return G_RET_ERR);
	return G_RET_SUCC;
}

static ATTR_MAYBE_UNUSED void
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
	for (unsigned int i = (unsigned int)-1;; ++i) {
		if (g_getcmds(i, g_blocks, LEN(g_blocks), g_statusbarlen))
			if (unlikely(g_status_write(g_statusstr) != G_RET_SUCC))
				DIE(return G_RET_ERR);
#ifdef TEST
		break;
#endif
		if (unlikely(sleep(1)))
			DIE(return G_RET_ERR);
	}
	return G_RET_SUCC;
}

void
g_handler_term(int signum)
{
	_Exit(EXIT_FAILURE);
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
	if (unlikely(g_status_init() != G_RET_SUCC))
		DIE(return EXIT_FAILURE);
	if (unlikely(atexit(g_status_cleanup) != 0))
		DIE(return EXIT_FAILURE);
	if (unlikely(g_status_mainloop() != G_RET_SUCC))
		DIE(return EXIT_FAILURE);
	return EXIT_SUCCESS;
}
