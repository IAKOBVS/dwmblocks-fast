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

#define LEN(X)           (sizeof(X) / sizeof(X[0]))
#define G_STATUSBLOCKLEN 64
/* Length of pad_left and pad_right < sizeof(g_statusblocks[0]). */
#define G_STATUSLEN (S_LEN(G_STATUS_PAD_LEFT) + (sizeof(g_statusblocks)) + sizeof(g_statusblocks) + S_LEN(G_STATUS_PAD_RIGHT) + 1)

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

static unsigned int b_sleeps[LEN(g_blocks)];
static struct {
	char *(*func)(char *, unsigned int, const char *, unsigned int *);
	const char *arg;
} b_blocks[LEN(g_blocks)];
static unsigned int b_intervals[LEN(g_blocks)];
static unsigned char b_tostatus_idxs[LEN(g_blocks)];
/* G_STATUSBLOCKLEN fits in an unsigned char. */
static unsigned char b_statusblocks_len[LEN(g_blocks)];
static unsigned char b_toblock_idxs[LEN(g_blocks)];
static struct {
	const char *pad_left;
	const char *pad_right;
} b_statuses[LEN(g_blocks)];
static unsigned char b_signals[LEN(g_blocks)];

static char g_statusblocks[LEN(g_blocks)][G_STATUSBLOCKLEN];
static char g_status_str[G_STATUSLEN];

#define B_FUNC(idx) (b_blocks[(idx)].func)
#define B_ARG(idx)  (b_blocks[(idx)].arg)

#define B_PAD_LEFT(idx)  (b_statuses[(idx)].pad_left)
#define B_PAD_RIGHT(idx) (b_statuses[(idx)].pad_right)

#define B_SLEEP(idx)            (b_sleeps[(idx)])
#define B_INTERVAL(idx)         (b_intervals[(idx)])
#define B_TOSTATUS(idx)         (b_tostatus_idxs[(idx)])
#define B_STATUSBLOCKS_LEN(idx) (b_statusblocks_len[(idx)])
#define B_TOBLOCK(idx)          (b_toblock_idxs[(idx)])
#define B_SIGNAL(idx)           (b_signals[(idx)])

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
static int g_status_changed;

static sigset_t sigset_rt;
static sigset_t sigset_old;

/* Run command or execute C function. */
static ATTR_INLINE char *
g_getcmd(char *dst, char *(*func)(char *, unsigned int, const char *, unsigned int *), const char *arg, unsigned int *interval)
{
	return func(dst, sizeof(g_statusblocks[0]), arg, interval);
}

int
compare_interval_and_signal(const void *a, const void *b)
{
	const g_block_ty *p = (const g_block_ty *)a;
	const g_block_ty *q = (const g_block_ty *)b;
	if (p->interval > q->interval)
		return 1;
	if (p->interval < q->interval)
		return -1;
	if (p->signal > q->signal)
		return 1;
	if (p->signal < q->signal)
		return -1;
	return 0;
}

static void
b_init()
{
	for (unsigned i = 0; i < LEN(g_blocks); ++i) {
		/* Check too long padding. */
		const size_t pad_len = strlen(g_blocks[i].pad_left) + strlen(g_blocks[i].pad_right);
		if (unlikely(pad_len > sizeof(g_statusblocks[0])))
			DIE();
		B_INTERVAL(i) = g_blocks[i].interval;
		B_FUNC(i) = g_blocks[i].func;
		B_ARG(i) = g_blocks[i].arg;
		B_TOSTATUS(i) = g_blocks[i].internal_tostatus_idx;
		B_TOBLOCK(B_TOSTATUS(i)) = i;
		B_PAD_LEFT(i) = g_blocks[i].pad_left;
		B_PAD_RIGHT(i) = g_blocks[i].pad_right;
		B_SIGNAL(i) = g_blocks[i].signal;
	}
}

/* Run commands or functions according to their interval. */
static void

g_getcmds_init()
{
	unsigned int i;
	/* Initialize the original order of the staturbar. */
	for (i = 0; i < LEN(g_blocks); ++i) {
		g_blocks[i].internal_tostatus_idx = i;
		/* Larger intervals mean less likely to need to update. */
		if (g_blocks[i].interval == 0)
			g_blocks[i].interval = (unsigned int)-1;
	}
	/* Sort blocks from their intervals. */
	qsort(g_blocks, LEN(g_blocks), sizeof(g_blocks[0]), compare_interval_and_signal);
	/* Initialize all statusblockss. */
	b_init();
	for (i = 0; i < LEN(g_blocks); ++i)
		/* Initialize the status blocks. */
		B_STATUSBLOCKS_LEN(B_TOSTATUS(i)) = g_getcmd(g_statusblocks[B_TOSTATUS(i)], B_FUNC(i), B_ARG(i), &B_INTERVAL(i)) - g_statusblocks[B_TOSTATUS(i)];
	if (unlikely(g_status_write(g_status_str) != G_RET_SUCC))
		DIE();
}

/* Run commands or functions according to their interval. */
static void
g_getcmds(unsigned int time)
{
	for (unsigned int i = 0; i < LEN(g_blocks); ++i) {
		/* Check if needs update. */
		if (B_SLEEP(i)-- > 0)
			continue;
		B_SLEEP(i) = B_INTERVAL(i);
		/* May need update. */
		char tmp[sizeof(g_statusblocks[0])];
		/* Get the result of g_getcmd. */
		const unsigned int tmp_len = g_getcmd(tmp, B_FUNC(i), B_ARG(i), &B_INTERVAL(i)) - tmp;
		/* Check if there has been change. */
		if (tmp_len == B_STATUSBLOCKS_LEN(B_TOSTATUS(i))
		    && !memcmp(tmp, g_statusblocks[B_TOSTATUS(i)], tmp_len))
			continue;
		/* Get the latest change. */
		memcpy(g_statusblocks[B_TOSTATUS(i)], tmp, tmp_len);
		B_STATUSBLOCKS_LEN(B_TOSTATUS(i)) = tmp_len;
		/* Mark change. */
		g_status_changed = 1;
	}
}

/* Same as g_getcmds but executed when receiving a signal. */
static void
g_getcmds_sig(unsigned int signal)
{
	for (unsigned int i = 0; i < LEN(g_blocks); ++i)
		if (B_SIGNAL(i) == signal)
			B_STATUSBLOCKS_LEN(B_TOSTATUS(i)) = g_getcmd(g_statusblocks[B_TOSTATUS(i)], B_FUNC(i), B_ARG(i), &B_INTERVAL(i)) - g_statusblocks[B_TOSTATUS(i)];
	g_status_changed = 1;
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
		if (B_SIGNAL(i) > 0) {
#ifdef HAVE_RT_SIGNALS
			if (unlikely(SIGMINUS + B_SIGNAL(i) > SIGRTMAX)) {
				fprintf(stderr, "dwmblocks-fast: Trying to handle signal (%u) over SIGRTMAX (%d).\n", B_SIGNAL(i), SIGRTMAX);
				DIE();
			}
#endif
			if (unlikely(g_sigaction(SIGMINUS + (int)B_SIGNAL(i), g_handler_sig) == -1))
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
	for (unsigned int i = 0; i < LEN(g_blocks); ++i)
		if (B_STATUSBLOCKS_LEN(i)) {
			end = u_stpcpy(end, B_PAD_LEFT(B_TOBLOCK(i)));
			end = u_stpcpy_len(end, g_statusblocks[i], B_STATUSBLOCKS_LEN(i));
			end = u_stpcpy(end, B_PAD_RIGHT(B_TOBLOCK(i)));
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
}
#endif

static g_ret_ty
g_status_write_stdout(char *status, int status_len)
{
	status[status_len++] = '\n';
	ssize_t ret = write(STDOUT_FILENO, status, (unsigned int)status_len);
	if (unlikely(ret != status_len))
		DIE(return G_RET_ERR);
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
		if (unlikely(g_status_write_stdout(status, end - status) != G_RET_SUCC))
			return G_RET_ERR;
		break;
	}
	g_status_changed = 0;
	return G_RET_SUCC;
}

/* Update hwmon/hwmon[0-9]* and thermal/thermal_zone[0-9]* to point to
 * the real file, given that the number may change between reboots. */
static g_ret_ty
g_paths_sysfs_resolve()
{
	for (unsigned int i = 0; i < LEN(g_blocks); ++i) {
		if (g_blocks[i].arg) {
			const char *p = path_sysfs_resolve(g_blocks[i].arg);
			if (unlikely(p == NULL))
				DIE();
			if (p != g_blocks[i].arg) {
				DBG(fprintf(stderr, "%s:%d:%s: %s doesn't exist, resolved to %s (which is malloc'd).\n", __FILE__, __LINE__, ASSERT_FUNC, g_blocks[i].arg, p));
				/* Set new path. */
				g_blocks[i].arg = p;
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
		if (g_status_changed)
			if (unlikely(g_status_write(g_status_str) != G_RET_SUCC))
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
	g_status_changed = 1;
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
	if (unlikely(g_status_init() != G_RET_SUCC))
		DIE(return EXIT_FAILURE);
#ifndef TEST
	if (unlikely(g_status_mainloop() != G_RET_SUCC))
		DIE(return EXIT_FAILURE);
#endif
	g_status_cleanup();
	return EXIT_SUCCESS;
}
