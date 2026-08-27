/* SPDX-License-Identifier: ISC */
/* Copyright 2025-2026 James Tirta Halim <tirtajames45 at gmail dot com>
 *
 * Engine tests: interval/signal sort, tostatus permutation integrity,
 * OBS ordering constraint, and status-string construction.
 *
 * Includes the production translation unit directly to reach statics;
 * its main() is renamed out of the way.
 *
 * Build:
 *   cc -o tests/test-status-bin tests/test-status.c $(OBJS) $(REQ) $(LDFLAGS)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define main dwmblocks_fast_production_main
#include "../dwmblocks-fast.c"
#undef main

static int nfail;

#define CHECK(cond, msg) do {                                   \
        if (!(cond)) {                                          \
                fprintf(stderr, "  FAIL  %s:%d: %s\n",          \
                        __FILE__, __LINE__, msg);               \
                ++nfail;                                        \
        }                                                       \
} while (0)

static void
test_comparator(void)
{
	printf("  [status 1] compare_interval_and_signal ordering       ... ");
	g_block_ty v[] = {
		{ .interval = 30, .signal = 5 },
		{ .interval = 2,  .signal = 0 },
		{ .interval = 30, .signal = 1 },
		{ .interval = 65535, .signal = 0 },
	};
	qsort(v, 4, sizeof(v[0]), compare_interval_and_signal);
	CHECK(v[0].interval == 2, "smallest interval first");
	CHECK(v[1].interval == 30 && v[1].signal == 1, "interval tie broken by signal");
	CHECK(v[2].interval == 30 && v[2].signal == 5, "interval tie broken by signal");
	CHECK(v[3].interval == 65535, "largest interval last");
	if (!nfail)
		printf("PASS\n");
	else
		printf("FAIL\n");
}

static void
test_init_layout(void)
{
	printf("  [status 2] g_getcmds_init: permutation + sort + OBS   ... ");
	const unsigned int n = LEN(g_blocks);

	/* Snapshot identity of every configured block (func+arg pair). */
	struct { char *(*f)(char *, unsigned int, const char *, unsigned short *); const char *a; } pre[LEN(g_blocks)];
	for (unsigned int i = 0; i < n; ++i) {
		pre[i].f = g_blocks[i].func;
		pre[i].a = g_blocks[i].arg;
	}

	g_getcmds_init();

	int ok = 1;
	/* Intervals non-decreasing after sort (signal-only mapped to max). */
	for (unsigned int i = 1; i < n; ++i)
		if (B_INTERVAL(i) < B_INTERVAL(i - 1)) {
			CHECK(0, "intervals not sorted ascending");
			ok = 0;
			break;
		}

	/* B_TOSTATUS is a permutation and maps back to the original slot. */
	unsigned char seen[LEN(g_blocks)] = {0};
	for (unsigned int i = 0; i < n; ++i) {
		const unsigned char t = B_TOSTATUS(i);
		if (t >= n || seen[t]) {
			CHECK(0, "tostatus not a permutation");
			ok = 0;
			break;
		}
		seen[t] = 1;
		if (pre[t].f != g_blocks[i].func || pre[t].a != g_blocks[i].arg) {
			CHECK(0, "tostatus does not map back to the original block");
			ok = 0;
			break;
		}
	}

#ifdef HAVE_PROCFS
	/* OBS constraint: recording block must come after on-block in
	 * iteration order so it can observe the open pid cache. */
	unsigned int i_on = n, i_rec = n;
	for (unsigned int i = 0; i < n; ++i) {
		if (B_FUNC(i) == b_write_obs_on)
			i_on = i;
		if (B_FUNC(i) == b_write_obs_recording)
			i_rec = i;
	}
	if (i_on < n && i_rec < n)
		CHECK(i_on < i_rec, "b_write_obs_on must precede b_write_obs_recording");
#endif

	if (ok && !nfail)
		printf("PASS\n");
	else
		printf("FAIL\n");
}

static void
test_status_builder(void)
{
	printf("  [status 3] g_status_get slow path composes pads+rows  ... ");
	const unsigned int n = LEN(g_blocks);
	int ok = 1;

	/* Reset composer state. */
	for (unsigned int i = 0; i < n; ++i)
		B_STATUSBLOCKS_LEN(i) = 0;

	/* Two populated rows, deliberately not adjacent. */
	B_PAD_LEFT(1) = "<L1>";
	B_PAD_RIGHT(1) = "<R1>";
	memcpy(g_statusblocks[1], "AAA", 3);
	B_STATUSBLOCKS_LEN(1) = 3;

	B_PAD_LEFT(3) = "<L3>";
	B_PAD_RIGHT(3) = "<R3>";
	memcpy(g_statusblocks[3], "BB", 2);
	B_STATUSBLOCKS_LEN(3) = 2;

	g_status_start_idx = 1;
	g_status_changed_len = 0;
	g_status_changed = 2; /* != 1 -> force slow path */

	char *end = g_status_get(g_status_str);
	(void)end;
	char exp[sizeof(g_status_str)];
	exp[0] = '\0';
	strncat(exp, G_STATUS_PAD_LEFT, sizeof(exp) - 1);
	strncat(exp, "<L1>", sizeof(exp) - 1 - strlen(exp));
	strncat(exp, "AAA", sizeof(exp) - 1 - strlen(exp));
	strncat(exp, "<R1>", sizeof(exp) - 1 - strlen(exp));
	strncat(exp, "<L3>", sizeof(exp) - 1 - strlen(exp));
	strncat(exp, "BB", sizeof(exp) - 1 - strlen(exp));
	strncat(exp, "<R3>", sizeof(exp) - 1 - strlen(exp));
	strncat(exp, G_STATUS_PAD_RIGHT, sizeof(exp) - 1 - strlen(exp));

	CHECK(strcmp(g_status_str, exp) == 0, g_status_str);
	if (strcmp(g_status_str, exp) != 0) {
		char msg[128];
		snprintf(msg, sizeof(msg), "\"%s\" != \"%s\"", g_status_str, exp);
		CHECK(0, msg);
		ok = 0;
	}
	CHECK(strlen(g_status_str) < G_STATUSLEN, "status exceeds buffer");

	/* Empty rows must contribute nothing (rows 0,2 skipped).
	 * Populated rows contribute 4+3+4 and 4+2+4 bytes. */
	CHECK(strlen(g_status_str) == strlen(G_STATUS_PAD_LEFT) + 21 + strlen(G_STATUS_PAD_RIGHT),
	      "empty rows leaked output");

	if (ok && !nfail)
		printf("PASS\n");
	else
		printf("FAIL\n");
}

int
main(void)
{
	printf("dwmblocks-fast engine tests\n");
	printf("===========================\n\n");

	test_comparator();
	test_init_layout();
	test_status_builder();

	printf("\n%s: %s\n",
	       nfail ? "FAIL" : "PASS",
	       nfail ? "some engine tests failed" : "all engine tests passed");
	return nfail ? 1 : 0;
}
