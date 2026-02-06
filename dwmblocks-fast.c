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
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>

#ifdef USE_X11
#	include <X11/Xlib.h>
#	include <X11/Xatom.h>
#endif

#include "blocks.h"
#include "macros.h"
#include "utils.h"
#include "path.h"

#if defined _POSIX_REALTIME_SIGNALS && (_POSIX_REALTIME_SIGNALS > 0)
#	define HAVE_RT_SIGNALS 1
#endif

#ifdef HAVE_RT_SIGNALS
#	define SIGPLUS  SIGRTMIN
#	define SIGMINUS SIGRTMIN
#else
#	define SIGPLUS  SIGUSR1 + 1
#	define SIGMINUS SIGUSR1 - 1
#endif

#define LEN(X)      (sizeof(X) / sizeof(X[0]))
#define G_CMDLENGTH 64
#define G_STATUSLEN (S_LEN(G_STATUS_PAD_LEFT) + (LEN(g_blocks) * G_CMDLENGTH) + S_LEN(G_STATUS_PAD_RIGHT) + 1)

#define G_STATUS_PAD_LEFT  " "
#define G_STATUS_PAD_RIGHT " "
/* Do not change. */
#define INTERVAL_UPDATE 1

typedef enum {
	G_RET_SUCC = 0,
	G_RET_ERR
} g_ret_ty;
typedef enum {
	G_WRITE_STATUSBAR = 0,
	G_WRITE_STDOUT
} g_write_ty;

#if HAVE_RT_SIGNALS
static void
g_handler_sig_dummy(int num);
#endif
static void
g_getcmds(unsigned int time);
static void
g_getcmds_sig(unsigned int signal);
static g_ret_ty
g_init_signals();
static void
g_handler_sig(int signum);
static char *
g_status_get(char *str);
static g_ret_ty
g_status_write(char *status);
static g_ret_ty
g_status_mainloop();
static void
g_handler_term(int signum);
#ifdef USE_X11
static g_ret_ty
g_init_x11();
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
static int g_statuschanged = 0;
static sigset_t sigset_rt;
static sigset_t sigset_old;

/* Run command or execute C function. */
static char *
g_getcmd(g_block_ty *block, char *dst, unsigned int dst_size)
{
	/* Add result of command or C function. */
	return block->func(dst, dst_size, block->command, &block->interval);
}

/* Run commands or functions according to their interval. */
static void
g_getcmds_init()
{
	for (unsigned int i = 0; i < LEN(g_blocks); ++i)
		g_statusbarlen[i] = g_getcmd(&g_blocks[i], g_statusbar[i], sizeof(g_statusbar[0])) - g_statusbar[i];
	if (unlikely(g_status_write(g_statusstr) != G_RET_SUCC))
		DIE();
}

/* Run commands or functions according to their interval. */
static void
g_getcmds(unsigned int time)
{
	for (unsigned int i = 0; i < LEN(g_blocks); ++i) {
		/* Check if needs update. */
		if (g_blocks[i].interval != 0 && time % g_blocks[i].interval == 0) {
			char tmp[sizeof(g_statusbar[0])];
			/* Get the result of g_getcmd. */
			const unsigned int tmp_len = g_getcmd(&g_blocks[i], tmp, sizeof(g_statusbar[0])) - tmp;
			/* Check if there has been change. */
			if (tmp_len != g_statusbarlen[i] || memcmp(tmp, g_statusbar[i], tmp_len)) {
				/* Get the latest change. */
				u_stpcpy_len(g_statusbar[i], tmp, tmp_len);
				g_statusbarlen[i] = tmp_len;
				/* Mark change. */
				g_statuschanged = 1;
			}
		}
	}
}

/* Same as g_getcmds but executed when receiving a signal. */
static void
g_getcmds_sig(unsigned int signal)
{
	for (unsigned int i = 0; i < LEN(g_blocks); ++i)
		if (g_blocks[i].signal == signal)
			g_statusbarlen[i] = g_getcmd(&g_blocks[i], g_statusbar[i], sizeof(g_statusbar[0])) - g_statusbar[i];
}

static int
g_sigaction(int signum, void(handler)(int))
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	if (unlikely(sigfillset(&sa.sa_mask)) == -1)
		return -1;
	return sigaction(signum, &sa, NULL);
}

static g_ret_ty
g_init_signals()
{
	if (unlikely(sigemptyset(&sigset_rt)) == -1)
		DIE(return G_RET_ERR);
	/* Initialize RT signals. */
#if HAVE_RT_SIGNALS
	for (int i = SIGRTMIN; i <= SIGRTMAX; ++i) {
		if (unlikely(g_sigaction(i, g_handler_sig_dummy) == -1))
			DIE(return G_RET_ERR);
		if (unlikely(sigaddset(&sigset_rt, i) == -1))
			DIE(return G_RET_ERR);
	}
#endif
	/* Handle RT signals. */
	for (unsigned int i = 0; i < LEN(g_blocks); ++i)
		if (g_blocks[i].signal > 0) {
#ifdef HAVE_RT_SIGNALS
			if (unlikely(SIGMINUS + g_blocks[i].signal > (unsigned int)SIGRTMAX)) {
				fprintf(stderr, "dwmblocks-fast: Trying to handle signal (%u) over SIGRTMAX (%d).\n", g_blocks[i].signal, SIGRTMAX);
				DIE();
			}
#endif
			if (unlikely(g_sigaction(SIGMINUS + (int)g_blocks[i].signal, g_handler_sig) == -1))
				DIE(return G_RET_ERR);
		}
	/* Handle termination signals. */
	if (unlikely(g_sigaction(SIGTERM, g_handler_term) == -1))
		DIE(return G_RET_ERR);
	if (unlikely(g_sigaction(SIGINT, g_handler_term) == -1))
		DIE(return G_RET_ERR);
	return G_RET_SUCC;
}

/* Construct the status string. */
static char *
g_status_get(char *dst)
{
	char *end = dst;
	end = u_stpcpy_len(end, S_LITERAL(G_STATUS_PAD_LEFT));
	for (unsigned int i = 0; i < LEN(g_blocks); ++i) {
		if (g_statusbarlen[i]) {
			end = u_stpcpy(end, g_blocks[i].pad_left);
			end = u_stpcpy_len(end, g_statusbar[i], g_statusbarlen[i]);
			end = u_stpcpy(end, g_blocks[i].pad_right);
		}
	}
	end = u_stpcpy_len(end, S_LITERAL(G_STATUS_PAD_RIGHT));
	return end;
}

static ATTR_INLINE int
g_sig_block()
{
	return sigprocmask(SIG_BLOCK, &sigset_rt, &sigset_old);
}

static ATTR_INLINE int
g_sig_unblock()
{
	return sigprocmask(SIG_SETMASK, &sigset_old, NULL);
}

static ATTR_INLINE void
g_sleep(unsigned int secs)
{
	if (unlikely(g_sig_unblock() != 0))
		DIE();
	sleep(secs);
	if (unlikely(g_sig_block() != 0))
		DIE();
}

#ifdef USE_X11
static ATTR_INLINE int
g_XStoreNameLen(Display *dpy, Window w, const char *name, int len)
{
	/* Directly use XChangeProperty to save a strlen. */
	return XChangeProperty(dpy, w, XA_WM_NAME, XA_STRING, 8, PropModeReplace, (_Xconst unsigned char *)name, len);
}

static g_ret_ty
g_init_x11()
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

#ifdef USE_X11
static void
g_status_write_x11(const char *status, int status_len)
{
	g_XStoreNameLen(g_dpy, g_win_root, status, status_len);
	XFlush(g_dpy);
	g_statuschanged = 0;
}
#endif

static g_ret_ty
g_status_write_stdout(char *status, int status_len)
{
	status[status_len++] = '\n';
	ssize_t ret = write(STDOUT_FILENO, status, (unsigned int)status_len);
	if (unlikely(ret != status_len))
		DIE(return G_RET_ERR);
	g_statuschanged = 0;
	return G_RET_SUCC;
}

static g_ret_ty
g_status_write(char *status)
{
	const char *end = g_status_get(status);
	switch (g_write_dst) {
#ifdef USE_X11
	case G_WRITE_STATUSBAR:
		g_status_write_x11(status, end - status);
		break;
#endif
	case G_WRITE_STDOUT:
		if (unlikely(g_status_write_stdout(status, end - status) == G_RET_ERR))
			return G_RET_ERR;
		break;
	}
	return G_RET_SUCC;
}

/* Update hwmon/hwmon[0-9]* and thermal/thermal_zone[0-9]* to point to
 * the real file, given that the number may change between reboots. */
static g_ret_ty
g_paths_sysfs_resolve()
{
	for (unsigned int i = 0; i < LEN(g_blocks); ++i) {
		if (g_blocks[i].command) {
			const char *p = path_sysfs_resolve(g_blocks[i].command);
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
	if (unlikely(g_paths_sysfs_resolve() != G_RET_SUCC))
		DIE(return G_RET_ERR);
#ifdef USE_X11
	if (unlikely(g_init_x11() != G_RET_SUCC))
		DIE(return G_RET_ERR);
#endif
	g_getcmds_init();
	if (unlikely(g_init_signals() != G_RET_SUCC))
		DIE(return G_RET_ERR);
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
static g_ret_ty
g_status_mainloop()
{
	unsigned int i = 0;
	for (;;) {
		g_sleep(INTERVAL_UPDATE);
		g_getcmds(i++);
		if (g_statuschanged)
			if (unlikely(g_status_write(g_statusstr) != G_RET_SUCC))
				DIE(return G_RET_ERR);
	}
	return G_RET_SUCC;
}

#ifdef HAVE_RT_SIGNALS
/* Handle errors gracefully. */
static void
g_handler_sig_dummy(int signum)
{
	char buf[S_LEN("dwmblocks-fast: sending unknown signal: ") + sizeof(size_t) * 8 + S_LEN("\n") + 1];
	char *end = buf;
	end = u_stpcpy_len(end, S_LITERAL("dwmblocks-fast: sending unknown signal: "));
	if (signum < 0)
		*end++ = '-';
	end = u_utoa_p((unsigned int)signum, buf);
	*end++ = '\n';
	*end = '\0';
	/* fprintf is not reentrant. */
	if (unlikely(write(STDERR_FILENO, buf, (size_t)(end - buf)) != (end - buf)))
		DIE();
}
#endif

static void
g_handler_sig(int signum)
{
	g_getcmds_sig((unsigned int)signum - (unsigned int)SIGPLUS);
	g_statuschanged = 1;
}

static void
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
	g_status_init();
#ifndef TEST
	if (unlikely(g_status_mainloop() != G_RET_SUCC))
		DIE(return EXIT_FAILURE);
#endif
	g_status_cleanup();
	return EXIT_SUCCESS;
}
