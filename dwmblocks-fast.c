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
#include <stdint.h>
#include <time.h>
#include <sys/select.h>

#if defined _POSIX_REALTIME_SIGNALS && (_POSIX_REALTIME_SIGNALS > 0)
#	define HAVE_RT_SIGNALS 1
#endif

/* Maximum user signal number.
 * Must accommodate all SIG_* defines in config.h. */
#ifdef HAVE_RT_SIGNALS
#	define G_SIGNAL_MAX ((int)SIGRTMAX - (int)SIGRTMIN)
#else
#	define G_SIGNAL_MAX 31
#endif

#ifdef USE_X11
#	include <X11/Xlib.h>
#	include <X11/Xatom.h>
#endif

#include "blocks.h"
#include "macros.h"
#include "utils.h"
#include "path.h"

#include "dwmblocks-fast.h"
unsigned int g_time;

#ifdef HAVE_RT_SIGNALS
#	define SIGMINUS (SIGRTMIN)
#else
#	define SIGMINUS (SIGUSR1 - 1)
#endif

#define G_STATUSBLOCKLEN 32
/* Length of pad_left and pad_right < sizeof(g_statusblocks[0]). */
#define G_STATUSLEN (S_LEN(G_STATUS_PAD_LEFT) + (sizeof(g_statusblocks)) + sizeof(g_statusblocks) + S_LEN(G_STATUS_PAD_RIGHT) + 1)

typedef enum {
	G_WRITE_STATUSBAR = 0,
	G_WRITE_STDOUT
} g_write_ty;

static unsigned short b_sleeps[LEN(g_blocks)];
/* Smallest countdown across all blocks, maintained by whichever pass
 * touched the countdowns last (getcmds / getcmds_sig / b_init), so
 * the mainloop never rescans b_sleeps[] to schedule its sleep. */
static unsigned int g_wake_min;
static struct {
	char *(*func)(char *, unsigned int, const char *, unsigned short *);
	const char *arg;
} b_blocks[LEN(g_blocks)];
static unsigned short b_intervals[LEN(g_blocks)];

static unsigned char b_tostatus_idxs[LEN(g_blocks)];
/* G_STATUSBLOCKLEN fits in an unsigned char. */
static unsigned char b_statusblocks_len[LEN(g_blocks)];
static struct {
	const char *pad_left;
	const char *pad_right;
} b_statuses[LEN(g_blocks)];
static unsigned char b_signals[LEN(g_blocks)];

static char g_statusblocks[LEN(g_blocks)][G_STATUSBLOCKLEN];
static char g_status_str[G_STATUSLEN];
static unsigned int g_status_str_len;

#define B_FUNC(idx) (b_blocks[(idx)].func)
#define B_ARG(idx)  (b_blocks[(idx)].arg)

#define B_PAD_LEFT(idx)  (b_statuses[(idx)].pad_left)
#define B_PAD_RIGHT(idx) (b_statuses[(idx)].pad_right)

#define B_SLEEP(idx)            (b_sleeps[(idx)])
#define B_INTERVAL(idx)         (b_intervals[(idx)])
#define B_TOSTATUS(idx)         (b_tostatus_idxs[(idx)])
#define B_STATUSBLOCKS_LEN(idx) (b_statusblocks_len[(idx)])
#define B_SIGNAL(idx)           (b_signals[(idx)])

#if HAVE_RT_SIGNALS
static void
g_handler_sig_dummy(int num);
#endif
static int
g_getcmds(void);
static int
g_getcmds_sig(unsigned int signal);
static int
g_init_signals(void);
static void
g_handler_sig(int signum);
static char *
g_status_get(char *str);
static int
g_status_write(char *status);
static int
g_status_mainloop(void);
static void
g_handler_term(int signum);
static void
g_handler_restart(int signum);
#ifdef USE_X11
static int
g_init_x11(void);

static Display *g_dpy;
static int g_screen;
static Window g_win_root;
static g_write_ty g_write_dst = G_WRITE_STATUSBAR;
#else
static const g_write_ty g_write_dst = G_WRITE_STDOUT;
#endif
static volatile sig_atomic_t g_signal_mask;
static volatile sig_atomic_t g_restart;
static int g_status_changed;
static int g_status_changed_len;
static unsigned int g_status_start_idx;
static unsigned char g_status_idx[LEN(g_blocks)];

static sigset_t sigset_rt;
static sigset_t sigset_empty;

/* Run command or execute C function. */
static ATTR_INLINE char *
g_getcmd(char *dst, char *(*func)(char *dst, unsigned int dst_len, const char *arg, unsigned short *interval), const char *arg, unsigned short *interval)
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

static int
b_init(void)
{
	for (unsigned int i = 0; i < LEN(g_blocks); ++i) {
		/* Check too long padding. */
		const size_t pad_len = strlen(g_blocks[i].pad_left) + strlen(g_blocks[i].pad_right);
		if (unlikely(pad_len > sizeof(g_statusblocks[0])))
			DIE(return -1);
		/* Verify function pointer is non-NULL. */
		if (unlikely(g_blocks[i].func == NULL))
			DIE(return -1);
		/* Verify signal number is in range for the bitmask. */
		if (unlikely(g_blocks[i].signal > G_SIGNAL_MAX))
			DIE(return -1);
		B_INTERVAL(i) = g_blocks[i].interval;
		B_FUNC(i) = g_blocks[i].func;
		B_ARG(i) = g_blocks[i].arg;
		B_TOSTATUS(i) = g_blocks[i].internal_tostatus_idx;
		B_PAD_LEFT(B_TOSTATUS(i)) = g_blocks[i].pad_left;
		B_PAD_RIGHT(B_TOSTATUS(i)) = g_blocks[i].pad_right;
		B_SIGNAL(i) = g_blocks[i].signal;
		/* Run every block on the next pass; also keeps the
		 * g_wake_min invariant fresh after a restart. */
		B_SLEEP(i) = 0;
	}
	g_wake_min = 0;
	return 0;
}

/* Run commands or functions according to their interval. */
static void
g_getcmds_init(void)
{
	memcpy(g_status_str, S_LITERAL(G_STATUS_PAD_LEFT));
	/* Initialize the original order of the staturbar. */
	for (unsigned int i = 0; i < LEN(g_blocks); ++i) {
		g_blocks[i].internal_tostatus_idx = i;
		/* Larger intervals mean less likely to need to update,
		 * needed for sort. */
		if (g_blocks[i].interval == 0)
			g_blocks[i].interval = (unsigned short)-1;
	}
	/* Sort blocks from their intervals. */
	qsort(g_blocks, LEN(g_blocks), sizeof(g_blocks[0]), compare_interval_and_signal);
	/* Initialize all statusblockss. */
	b_init();
}

/* Run commands or functions according to their interval.  Countdowns
 * are decremented by g_ticks_advance; this pass runs the blocks whose
 * countdown reached zero and tracks the smallest remaining countdown
 * for the scheduler. */
static int
g_getcmds(void)
{
	g_wake_min = (unsigned int)-1;
	unsigned short left;
	for (unsigned int i = 0; i < LEN(g_blocks); ++i, g_wake_min = MIN(g_wake_min, left)) {
		left = B_SLEEP(i);
		if (left)
			continue;
		B_SLEEP(i) = B_SIGNAL(i) ? B_INTERVAL(i) : B_INTERVAL(i) - 1;
		/* Skip blocks with NULL function pointer. */
		if (unlikely(B_FUNC(i) == NULL))
			continue;
		/* May need update. */
		char tmp[sizeof(g_statusblocks[0])];
		/* Get the result of g_getcmd. */
		const char *tmp_e = g_getcmd(tmp, B_FUNC(i), B_ARG(i), &B_SLEEP(i));
		if (unlikely(tmp_e == NULL))
			DIE(return -1);
		const unsigned int tmp_len = tmp_e - tmp;
		const unsigned char sti = B_TOSTATUS(i);
		g_status_changed_len = tmp_len != B_STATUSBLOCKS_LEN(sti);
		/* Check if there has been change. */
		if (!g_status_changed_len && !memcmp(tmp, g_statusblocks[sti], tmp_len))
			continue;
		/* Get the latest change. */
		u_stpcpy_len(g_statusblocks[sti], tmp, tmp_len);
		B_STATUSBLOCKS_LEN(sti) = tmp_len;
		/* Mark change. */
		++g_status_changed;
		/* Get latest rightmost. */
		g_status_start_idx = MIN(g_status_start_idx, sti);
	}
	return 0;
}

/* Same as g_getcmds but executed when receiving a signal.  Visits
 * every block so g_wake_min stays a full snapshot of b_sleeps[] even
 * though only signal-matched blocks render (their functions may
 * rewrite their own countdown, e.g. audio retry intervals). */
static int
g_getcmds_sig(unsigned int signal)
{
	g_wake_min = (unsigned int)-1;
	unsigned short left;
	for (unsigned int i = 0; i < LEN(g_blocks); ++i, g_wake_min = MIN(g_wake_min, left)) {
		left = B_SLEEP(i);
		if (B_SIGNAL(i) != signal)
			continue;
		if (B_FUNC(i) == NULL)
			continue;
		/* Render into tmp first so unchanged output does not
		 * trigger a full status rewrite (mirrors g_getcmds). */
		char tmp[sizeof(g_statusblocks[0])];
		const char *end = g_getcmd(tmp, B_FUNC(i), B_ARG(i), &B_SLEEP(i));
		if (unlikely(end == NULL))
			DIE(return -1);
		const unsigned int tmp_len = end - tmp;
		const unsigned char sti = B_TOSTATUS(i);
		g_status_changed_len = tmp_len != B_STATUSBLOCKS_LEN(sti);
		if (!g_status_changed_len && !memcmp(tmp, g_statusblocks[sti], tmp_len))
			continue;
		u_stpcpy_len(g_statusblocks[sti], tmp, tmp_len);
		B_STATUSBLOCKS_LEN(sti) = tmp_len;
		/* Mark change. */
		++g_status_changed;
		/* Get latest rightmost. */
		g_status_start_idx = MIN(g_status_start_idx, sti);
	}
	return 0;
}

static int
g_sigaction(int signum, void(handler)(int))
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sa.sa_flags = SA_RESTART;
	if (unlikely(sigfillset(&sa.sa_mask)) == -1)
		DIE(return -1);
	if (unlikely(sigaction(signum, &sa, NULL)) == -1)
		DIE(return -1);
	return 0;
}

static ATTR_INLINE void
g_sig_block(void)
{
	sigprocmask(SIG_BLOCK, &sigset_rt, NULL);
}

static int
g_init_signals(void)
{
	if (unlikely(sigemptyset(&sigset_rt)) == -1)
		DIE(return -1);
	if (unlikely(sigemptyset(&sigset_empty)) == -1)
		DIE(return -1);
	/* Initialize RT signals. */
#if HAVE_RT_SIGNALS
	for (int i = SIGRTMIN; i <= SIGRTMAX; ++i) {
		if (unlikely(g_sigaction(i, g_handler_sig_dummy) == -1))
			DIE(return -1);
		if (unlikely(sigaddset(&sigset_rt, i) == -1))
			DIE(return -1);
	}
#endif
	/* Handle signals for blocks. */
	for (unsigned int i = 0; i < LEN(g_blocks); ++i) {
		if (B_SIGNAL(i) > 0) {
			int target_sig = SIGMINUS + (int)B_SIGNAL(i);
#ifdef HAVE_RT_SIGNALS
			if (unlikely(target_sig > SIGRTMAX)) {
				fprintf(stderr, "dwmblocks-fast: Trying to handle signal (%u) over SIGRTMAX (%d).\n", B_SIGNAL(i), SIGRTMAX);
				DIE(return -1);
			}
#endif
			/* Add fallback or RT signal to the block mask. */
			if (unlikely(sigaddset(&sigset_rt, target_sig) == -1))
				DIE(return -1);

			if (unlikely(g_sigaction(target_sig, g_handler_sig) == -1))
				DIE(return -1);
		}
	}
	/* Handle termination signals. */
	if (unlikely(g_sigaction(SIGTERM, g_handler_term) == -1))
		DIE(return -1);
	if (unlikely(g_sigaction(SIGINT, g_handler_term) == -1))
		DIE(return -1);
	if (unlikely(g_sigaction(SIGHUP, g_handler_restart) == -1))
		DIE(return -1);
	g_sig_block();
	return 0;
}

/* Construct the status string. */
static char *
g_status_get(char *dst)
{
	dst += S_LEN(G_STATUS_PAD_LEFT);
	char *start = dst;
	/* Skip things we don't need to update. */
	dst += g_status_idx[g_status_start_idx];
	/* Slow path: multiple bars need to be updated, or there is length change. */
	if (g_status_changed_len || g_status_changed != 1) {
		for (unsigned int i = g_status_start_idx; i < LEN(g_statusblocks); ++i) {
			g_status_idx[i] = dst - start;
			if (B_STATUSBLOCKS_LEN(i)) {
				dst = u_stpcpy(dst, B_PAD_LEFT(i));
				dst = u_mempcpy(dst, g_statusblocks[i], B_STATUSBLOCKS_LEN(i));
				dst = u_stpcpy(dst, B_PAD_RIGHT(i));
				DBG(fprintf(stderr, "%s:%d:%s: Printing pad_left: %s\n", __FILE__, __LINE__, ASSERT_FUNC, B_PAD_LEFT(i)));
				DBG(fprintf(stderr, "%s:%d:%s: Printing g_statusblocks[%d]: %s\n", __FILE__, __LINE__, ASSERT_FUNC, i, g_statusblocks[i]));
				DBG(fprintf(stderr, "%s:%d:%s: Printing pad_right: %s\n", __FILE__, __LINE__, ASSERT_FUNC, B_PAD_RIGHT(i)));
			}
		}
		dst = u_stpcpy_len(dst, S_LITERAL(G_STATUS_PAD_RIGHT));
	} else {
		/* Fast path: only one bar needs to be updated and no length change. */
		if (B_STATUSBLOCKS_LEN(g_status_start_idx)) {
			memcpy(dst + strlen(B_PAD_LEFT(g_status_start_idx)), g_statusblocks[g_status_start_idx], B_STATUSBLOCKS_LEN(g_status_start_idx));
			dst = g_status_str + g_status_str_len;
			DBG(fprintf(stderr, "%s:%d:%s: Printing g_statusblocks[%d]: %s\n", __FILE__, __LINE__, ASSERT_FUNC, g_status_start_idx, g_statusblocks[g_status_start_idx]));
		}
	}
	g_status_start_idx = (unsigned int)-1;
	return dst;
}

/* CLOCK_MONOTONIC as nanoseconds. */
static ATTR_INLINE unsigned long long
g_mono_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long long)ts.tv_sec * 1000000000ULL + (unsigned long long)ts.tv_nsec;
}

static unsigned long long g_sched_ns; /* next wake deadline */
static unsigned long long g_last_ns;  /* previous pass, for deltas */
static unsigned long long g_rem_ns;   /* sub-second carry across passes */

/* Advance the scheduler clock by the real elapsed time.  Ticks come
 * from CLOCK_MONOTONIC with the sub-second remainder carried across
 * passes, so g_time tracks wall time exactly instead of assuming each
 * iteration costs one second (block rendering and X11 writes made the
 * old fixed increment drift).  Countdowns are decremented by the same
 * amount; blocks reaching zero are due on the next pass. */
static void
g_ticks_advance(unsigned long long now)
{
	g_rem_ns += now - g_last_ns;
	g_last_ns = now;
	const unsigned int secs = (unsigned int)(g_rem_ns / 1000000000ULL);
	if (unlikely(secs == 0))
		return;
	g_rem_ns %= 1000000000ULL;
	g_time += secs;
	for (unsigned int i = 0; i < LEN(g_blocks); ++i) {
		const unsigned int left = B_SLEEP(i);
		B_SLEEP(i) = (unsigned short)(left > secs ? left - secs : 0);
	}
}

/* Next due moment: start of the current tick bucket plus the nearest
 * block's countdown.  A pure function of scheduler state, so passes
 * made between tick boundaries (e.g. right after a signal) neither
 * lose nor extend the wait. */
static ATTR_INLINE unsigned long long
g_next_deadline(void)
{
	return g_last_ns - g_rem_ns + ((unsigned long long)g_wake_min + 1) * 1000000000ULL;
}

/* Sleep until the absolute deadline.  Returns nonzero if a signal cut
 * the wait short so the caller loops and services the mask at once —
 * signal-triggered updates stay realtime, exactly like the fixed
 * 1 s sleep they replaced.  The deadline itself survives: it is
 * recomputed from state on the next pass. */
static ATTR_INLINE int
g_sleep_till(unsigned long long deadline)
{
	for (;;) {
		const unsigned long long now = g_mono_ns();
		if (now >= deadline)
			return 0;
		const unsigned long long rem = deadline - now;
		if (pselect(0, NULL, NULL, NULL, &(struct timespec){ .tv_sec = (time_t)(rem / 1000000000ULL), .tv_nsec = (long)(rem % 1000000000ULL) }, &sigset_empty) == -1)
			return 1; /* EINTR: pending signal, service it now */
	}
}

#ifdef USE_X11
static ATTR_INLINE int
g_XStoreNameLen(Display *dpy, Window w, const char *name, int len)
{
	/* Directly use XChangeProperty to save a strlen. */
	return XChangeProperty(dpy, w, XA_WM_NAME, XA_STRING, 8, PropModeReplace, (_Xconst unsigned char *)name, len);
}

static int
g_init_x11(void)
{
	g_dpy = XOpenDisplay(NULL);
	if (unlikely(g_dpy == NULL)) {
		fprintf(stderr, "dwmblocks-fast: Failed to open display.\n");
		DIE(return -1);
	}
	g_screen = DefaultScreen(g_dpy);
	g_win_root = RootWindow(g_dpy, g_screen);
	return 0;
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

static int
g_status_write_stdout(char *status, int status_len)
{
	status[status_len++] = '\n';
	ssize_t ret = write(STDOUT_FILENO, status, (unsigned int)status_len);
	if (unlikely(ret != status_len))
		DIE(return -1);
	return 0;
}

static int
g_status_write(char *status)
{
	/* TODO: optimize stdout path, use writev from uio.h.  */
	const char *end = g_status_get(status);
	switch (g_write_dst) {
#ifdef USE_X11
	case G_WRITE_STATUSBAR:
		g_status_write_x11(status, end - status);
		break;
#endif
	case G_WRITE_STDOUT:
		if (unlikely(g_status_write_stdout(status, end - status) == -1))
			DIE(return -1);
		break;
	}
	g_status_str_len = end - status;
	g_status_changed = 0;
	g_status_changed_len = 0;
	return 0;
}

/* Update hwmon/hwmon[0-9]* and thermal/thermal_zone[0-9]* to point to
 * the real file, given that the number may change between reboots. */
static int
g_paths_sysfs_resolve(void)
{
	for (unsigned int i = 0; i < LEN(g_blocks); ++i) {
		if (g_blocks[i].arg && (strstr(g_blocks[i].arg, "/sys/"))) {
			const char *p = path_sysfs_resolve(g_blocks[i].arg);
			if (unlikely(p == NULL))
				DIE(return -1);
			if (p != g_blocks[i].arg) {
				DBG(fprintf(stderr, "%s:%d:%s: %s doesn't exist, resolved to %s (which is malloc'd).\n", __FILE__, __LINE__, ASSERT_FUNC, g_blocks[i].arg, p));
				/* Set new path. */
				g_blocks[i].arg = p;
			} else {
				DBG(fprintf(stderr, "%s:%d:%s %s exists.\n", __FILE__, __LINE__, ASSERT_FUNC, p));
			}
		}
	}
	return 0;
}

static int
g_status_init(void)
{
	if (unlikely(g_paths_sysfs_resolve() == -1))
		DIE(return -1);
#ifdef USE_X11
	if (unlikely(g_init_x11() == -1))
		DIE(return -1);
#endif
	g_getcmds_init();
	if (unlikely(g_init_signals() == -1))
		DIE(return -1);
	return 0;
}

static void
g_status_cleanup(void)
{
#ifdef USE_X11
	XCloseDisplay(g_dpy);
#endif
}

/* Main loop. */
static int
g_status_mainloop(void)
{
	g_last_ns = g_sched_ns = g_mono_ns();
	for (;;) {
		/* Atomic read-and-clear so a handler racing between read
		 * and clear cannot lose its bit. */
		const sig_atomic_t mask = __sync_fetch_and_and(&g_signal_mask, 0);
		if (unlikely(mask != 0)) {
			if (unlikely(g_restart != 0)) {
				g_restart = 0;
				b_init();
				/* Sleeps and g_wake_min are zeroed by
				 * b_init: g_next_deadline() schedules the
				 * full refresh within a second. */
			} else {
				for (unsigned int s = 1; s <= (unsigned int)G_SIGNAL_MAX; ++s)
					if (mask & (sig_atomic_t)(1u << s))
						if (unlikely(g_getcmds_sig(s) == -1))
							DIE(return -1);
			}
		} else {
			/* Tick pass: advance the scheduler clock by real
			 * elapsed time and run due blocks.  Signal passes
			 * never touch the countdowns, so signals arriving
			 * mid-wait cannot delay periodic updates; conversely
			 * the deadline is derived from scheduler state, so
			 * servicing a signal early never extends it. */
			g_ticks_advance(g_mono_ns());
			if (unlikely(g_getcmds() == -1))
				DIE(return -1);
		}
		g_sched_ns = g_next_deadline();
		if (g_status_changed)
			if (unlikely(g_status_write(g_status_str) == -1))
				DIE(return -1);
#ifdef TEST
		return 0;
#endif
		g_sleep_till(g_sched_ns); /* returns early on signal */
	}
	return 0;
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
	end = u_utoa_p((unsigned int)signum, end);
	*end++ = '\n';
	*end = '\0';
	/* fprintf is not reentrant. Best-effort write. */
	write(STDERR_FILENO, buf, (size_t)(end - buf));
}
#endif

static void
g_handler_sig(int signum)
{
	int sig_num = signum - (int)SIGMINUS;
	if (sig_num > 0 && sig_num <= G_SIGNAL_MAX)
		g_signal_mask |= (sig_atomic_t)(1u << sig_num);
}

static void
g_handler_term(int signum)
{
	write(STDERR_FILENO, S_LITERAL("Exiting!\n"));
	_Exit(EXIT_SUCCESS);
	(void)signum;
}

static void
g_handler_restart(int signum)
{
	g_signal_mask = (sig_atomic_t)(~(sig_atomic_t)0);
	g_restart = 1;
	(void)signum;
}

int
main(int argc, char **argv)
{
#ifdef USE_X11
	/* Handle command line arguments. */
	for (int i = 0; i < argc; ++i)
		/* Check if printing to stdout. */
		if (!strcmp("-p", argv[i]))
			g_write_dst = G_WRITE_STDOUT;
#endif
	if (unlikely(g_status_init() == -1))
		DIE(return EXIT_FAILURE);
	if (unlikely(g_status_mainloop() == -1))
		DIE(return EXIT_FAILURE);
	g_status_cleanup();
	return EXIT_SUCCESS;
}
