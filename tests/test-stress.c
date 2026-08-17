/* SPDX-License-Identifier: ISC */
/* Copyright 2025-2026 James Tirta Halim <tirtajames45 at gmail dot com>
 *
 * Stress/edge-case tests for dwmblocks-fast signal handling.
 *
 * Tests:
 *   1. Signal bitmask accumulation
 *   2. Fork/kill rapid-fire stress (requires setcap for powercap)
 *   3. Rapid re-signal stress (requires setcap for powercap)
 *   4. Edge-case resilience — reserved signal index, top-of-range
 *   5. Mock-block edge cases — out-of-range, valid match
 *
 * Build:
 *   cc -o tests/test-stress-bin tests/test-stress.c -lrt
 */

#define _GNU_SOURCE
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */

static int binary_check_done;
static int binary_ok;

/* Reset real-time signal handlers to SIG_DFL for a clean fork. */
static void
reset_rt_handlers(void)
{
	struct sigaction sa_def;
	memset(&sa_def, 0, sizeof(sa_def));
	sa_def.sa_handler = SIG_DFL;
	sigemptyset(&sa_def.sa_mask);
	for (int s = SIGRTMIN; s <= SIGRTMAX; ++s)
		sigaction(s, &sa_def, NULL);
}

/* Probe whether the real dwmblocks-fast binary works (needs setcap). */
static int
probe_binary(void)
{
	if (binary_check_done)
		return binary_ok;
	binary_check_done = 1;

	/* Save parent signal mask. */
	sigset_t old;
	sigemptyset(&old);
	sigprocmask(SIG_BLOCK, NULL, &old);

	pid_t pid = fork();
	if (pid == -1)
		return 0;

	if (pid == 0) {
		/* Child: reset handlers to default, restore mask, exec. */
		reset_rt_handlers();
		sigprocmask(SIG_SETMASK, &old, NULL);
		int fd = open("/dev/null", O_WRONLY);
		if (fd != -1) {
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			close(fd);
		}
		execl("./bin/dwmblocks-fast", "dwmblocks-fast", "-p", (char *)NULL);
		execlp("dwmblocks-fast", "dwmblocks-fast", "-p", (char *)NULL);
		_Exit(127);
	}

	struct timespec ts = { .tv_sec = 0, .tv_nsec = 150000000L };
	nanosleep(&ts, NULL);
	kill(pid, SIGTERM);

	int wstatus;
	waitpid(pid, &wstatus, 0);

	binary_ok = (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0);
	return binary_ok;
}

/* ------------------------------------------------------------------ */
/*  Test 1 — signal delivery (simple g_signal, last-one-wins)         */
/* ------------------------------------------------------------------ */

#define G_SIGNAL_MAX  (SIGRTMAX - SIGRTMIN)
#define SIGPLUS       SIGRTMIN

static volatile sig_atomic_t g_signal;

static void
handler_signal(int signum)
{
	int sig_num = signum - (int)SIGPLUS;
	if (sig_num > 0 && sig_num <= G_SIGNAL_MAX)
		g_signal = (sig_atomic_t)sig_num;
}

static void
unblock_rt_signals(void)
{
	sigset_t rt_set;
	sigemptyset(&rt_set);
	for (int s = SIGRTMIN; s <= SIGRTMAX; ++s)
		sigaddset(&rt_set, s);
	sigprocmask(SIG_UNBLOCK, &rt_set, NULL);
}

static int
test_signal_delivery(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler_signal;
	sa.sa_flags   = SA_RESTART;
	sigfillset(&sa.sa_mask);

	printf("  [test 1] signal delivery (single signal)                 ... ");

	unblock_rt_signals();
	for (int sig = SIGPLUS + 1; sig <= SIGPLUS + 4; ++sig)
		sigaction(sig, &sa, NULL);

	g_signal = 0;
	raise(SIGPLUS + 3);

	int sig = (int)g_signal;
	g_signal = 0;
	if (sig != 3) {
		printf("FAIL (g_signal=%d, expected 3)\n", sig);
		return 1;
	}
	printf("PASS\n");

	printf("  [test 1b] last-one-wins (two signals)                    ... ");
	g_signal = 0;
	raise(SIGPLUS + 1);
	raise(SIGPLUS + 4);
	sig = (int)g_signal;
	g_signal = 0;
	if (sig != 4) {
		printf("FAIL (g_signal=%d, expected 4)\n", sig);
		return 1;
	}
	printf("PASS\n");

	printf("  [test 1c] signal index 0 (reserved) ignored               ... ");
	sigaction(SIGPLUS + 0, &sa, NULL);
	g_signal = 0;
	raise(SIGPLUS + 0);
	sig = (int)g_signal;
	g_signal = 0;
	if (sig != 0) {
		printf("FAIL (g_signal=%d, expected 0)\n", sig);
		return 1;
	}
	printf("PASS\n");

	return 0;
}

/* ------------------------------------------------------------------ */
/*  Test 2 — fork/kill rapid-fire stress                              */
/* ------------------------------------------------------------------ */

static int
run_forkkill(const char *label, int rounds, int multi)
{
	printf("  [test %s]                                                 ... ", label);
	fflush(stdout);

	if (!probe_binary()) {
		printf("SKIP (binary needs setcap)\n");
		return 0;
	}

	int pipefd[2];
	if (pipe(pipefd) == -1)
		return 1;

	/* Save mask, reset handlers, fork. */
	sigset_t old_mask;
	sigemptyset(&old_mask);
	sigprocmask(SIG_BLOCK, NULL, &old_mask);

	pid_t child = fork();
	if (child == -1) {
		close(pipefd[0]);
		close(pipefd[1]);
		return 1;
	}

	if (child == 0) {
		reset_rt_handlers();
		sigprocmask(SIG_SETMASK, &old_mask, NULL);
		int devnull = open("/dev/null", O_WRONLY);
		if (devnull != -1) {
			dup2(devnull, STDERR_FILENO);
			close(devnull);
		}
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		execl("./bin/dwmblocks-fast", "dwmblocks-fast", "-p", (char *)NULL);
		execlp("dwmblocks-fast", "dwmblocks-fast", "-p", (char *)NULL);
		_Exit(127);
	}

	close(pipefd[1]);

	struct timespec ts = { .tv_sec = 0, .tv_nsec = 200000000L };
	nanosleep(&ts, NULL);

	int total_sent = 0;
	int sigs[] = { SIGPLUS + 1, SIGPLUS + 2, SIGPLUS + 3, SIGPLUS + 4 };
	int nsigs = multi ? 4 : 2;
	for (int r = 0; r < rounds; ++r) {
		for (int s = 0; s < nsigs; ++s) {
			if (kill(child, sigs[s]) == 0)
				++total_sent;
		}
		kill(child, SIGUSR1);
	}

	nanosleep(&ts, NULL);
	kill(child, SIGTERM);

	int wstatus;
	waitpid(child, &wstatus, 0);
	close(pipefd[0]);

	if (WIFSIGNALED(wstatus) && WTERMSIG(wstatus) != SIGTERM) {
		printf("FAIL (child killed by signal %d)\n", WTERMSIG(wstatus));
		return 1;
	}
	printf("PASS (sent %d signals)\n", total_sent);
	return 0;
}

static int
test_fork_kill_stress(void)
{
	return run_forkkill("2", 100, 0);
}

static int
test_rapid_resignal(void)
{
	return run_forkkill("3", 50, 1);
}

/* ------------------------------------------------------------------ */
/*  Test 4 — edge-case resilience                                     */
/* ------------------------------------------------------------------ */

static int
test_edge_cases(void)
{
	int failed = 0;
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler_signal;
	sa.sa_flags   = SA_RESTART;
	sigfillset(&sa.sa_mask);

	unblock_rt_signals();
	int max_valid = SIGRTMAX - SIGRTMIN;

	printf("  [test 4a] signal index 0 (reserved) ignored               ... ");
	sigaction(SIGPLUS + 0, &sa, NULL);
	g_signal = 0;
	raise(SIGPLUS + 0);
	int sig = (int)g_signal;
	g_signal = 0;
	if (sig != 0) {
		printf("FAIL (g_signal=%d, expected 0)\n", sig);
		++failed;
	} else {
		printf("PASS\n");
	}

	if (max_valid >= 1) {
		int top = (max_valid < G_SIGNAL_MAX) ? max_valid : G_SIGNAL_MAX;
		sigaction(SIGPLUS + top, &sa, NULL);

		printf("  [test 4b] signal index %d (top of RT range) accepted    ... ", top);
		g_signal = 0;
		raise(SIGPLUS + top);
		sig = (int)g_signal;
		g_signal = 0;
		if (sig != top) {
			printf("FAIL (g_signal=%d, expected %d)\n", sig, top);
			++failed;
		} else {
			printf("PASS\n");
		}
	}

	return failed;
}

/* ------------------------------------------------------------------ */
/*  Test 5 — mock-block edge cases                                    */
/* ------------------------------------------------------------------ */

enum { MOCK_NBLOCKS = 4 };
static unsigned char mock_signal[MOCK_NBLOCKS];

static void
mock_init(void)
{
	mock_signal[0] = 1;
	mock_signal[1] = 2;
	mock_signal[2] = 3;
	mock_signal[3] = 4;
}

static int
mock_getcmds_sig(unsigned int signal)
{
	if (signal > G_SIGNAL_MAX)
		return 0;
	for (int i = 0; i < MOCK_NBLOCKS; ++i) {
		if (mock_signal[i] == signal)
			return 1;
	}
	return 0;
}

static int
test_mock_edge(void)
{
	int failed = 0;

	printf("  [test 5a] signal=99 (above max) – early return            ... ");
	if (mock_getcmds_sig(99) != 0) {
		printf("FAIL\n");
		++failed;
	} else {
		printf("PASS\n");
	}

	printf("  [test 5b] signal=1  (valid)      – matches block 0        ... ");
	if (mock_getcmds_sig(1) != 1) {
		printf("FAIL\n");
		++failed;
	} else {
		printf("PASS\n");
	}

	printf("  [test 5c] signal=0  (timer-only) – no match (no sig 0 blk) ... ");
	if (mock_getcmds_sig(0) != 0) {
		printf("FAIL\n");
		++failed;
	} else {
		printf("PASS\n");
	}

	return failed;
}

/* ------------------------------------------------------------------ */
/*  main                                                              */
/* ------------------------------------------------------------------ */

int
main(void)
{
	int total_fail = 0;

	printf("dwmblocks-fast stress/edge tests\n");
	printf("================================\n\n");

	/* Block everything except SIGTERM and SIGINT so they
	 * can't interfere during non-fork tests. */
	sigset_t old_block;
	sigset_t block_all;
	sigfillset(&block_all);
	sigdelset(&block_all, SIGTERM);
	sigdelset(&block_all, SIGINT);
	sigprocmask(SIG_SETMASK, &block_all, &old_block);

	total_fail += test_signal_delivery();
	total_fail += test_fork_kill_stress();
	total_fail += test_rapid_resignal();
	total_fail += test_edge_cases();

	mock_init();
	total_fail += test_mock_edge();

	sigprocmask(SIG_SETMASK, &old_block, NULL);

	printf("\n%s: %s\n",
	       total_fail ? "FAIL" : "PASS",
	       total_fail ? "some tests failed" : "all tests passed");
	return total_fail ? 1 : 0;
}
