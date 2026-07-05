/* SPDX-License-Identifier: ISC */
/* Copyright 2025-2026 James Tirta Halim <tirtajames45 at gmail dot com>
 *
 * Edge-case tests that exercise block functions directly.
 *
 * NOTE: The DIE macro calls assert(0) which aborts, so we only
 * test code-paths that succeed.  Error recovery (NULL return)
 * is handled by DIE's abort, which is tested implicitly by the
 * crash tests in the shell runner.
 *
 * Build:
 *   cc -o tests/test-edge-cases-bin tests/test-edge-cases.c \
 *      $(OBJS) $(REQ) $(LDFLAGS)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/* Satisfy extern reference from block object files. */
unsigned int g_time;

/* Block function prototypes */
extern char *b_write_date(char *dst, unsigned int dst_size,
                          const char *unused, unsigned int *interval);
extern char *b_write_time(char *dst, unsigned int dst_size,
                          const char *unused, unsigned int *interval);
extern char *b_write_cpu_usage(char *dst, unsigned int dst_size,
                               const char *unused, unsigned int *interval);
extern char *b_write_ram_usage_percent(char *dst, unsigned int dst_size,
                                       const char *unused, unsigned int *interval);
extern char *b_write_ram_usage_available(char *dst, unsigned int dst_size,
                                         const char *unused, unsigned int *interval);
extern char *b_write_disk_usage_percent(char *dst, unsigned int dst_size,
                                        const char *path, unsigned int *interval);
extern char *b_write_disk_usage_free(char *dst, unsigned int dst_size,
                                     const char *path, unsigned int *interval);

static int nfail;

#define CHECK(cond, msg) do {                                   \
        if (!(cond)) {                                          \
                fprintf(stderr, "  FAIL  %s:%d: %s\n",          \
                        __FILE__, __LINE__, msg);               \
                ++nfail;                                        \
        }                                                       \
} while (0)

/* ------------------------------------------------------------------ */
/*  Test 1 — NULL arg on a block that ignores arg                     */
/* ------------------------------------------------------------------ */

static int
test_null_arg(void)
{
	char buf[64] = {0};
	unsigned int interval = 0;

	printf("  [edge 1] NULL arg on b_write_time                    ... ");
	char *end = b_write_time(buf, sizeof(buf), NULL, &interval);
	CHECK(end != NULL, "expected non-NULL return");
	CHECK(end > buf, "expected data written");
	if (end != NULL && end > buf)
		printf("PASS (wrote %td bytes)\n", end - buf);
	else
		printf("FAIL\n");
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Test 2 — empty-string arg on a block that ignores arg             */
/* ------------------------------------------------------------------ */

static int
test_empty_arg(void)
{
	char buf[64] = {0};
	unsigned int interval = 0;

	printf("  [edge 2] empty-string arg on b_write_date            ... ");
	char *end = b_write_date(buf, sizeof(buf), "", &interval);
	CHECK(end != NULL, "expected non-NULL return");
	CHECK(end > buf, "expected data written");
	if (end != NULL && end > buf)
		printf("PASS\n");
	else
		printf("FAIL\n");
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Test 3 — interval modified by block function                      */
/* ------------------------------------------------------------------ */

static int
test_interval_modified(void)
{
	char buf[64] = {0};
	unsigned int interval = 999;

	printf("  [edge 3] b_write_time sets interval to < 90 sec       ... ");
	b_write_time(buf, sizeof(buf), NULL, &interval);
	CHECK(interval < 90, "expected interval < 90");
	CHECK(interval > 0, "expected interval > 0");
	if (interval > 0 && interval < 90)
		printf("PASS (interval=%u)\n", interval);
	else
		printf("FAIL\n");
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Test 4 — b_write_disk with valid path but zero-sized dst          */
/* ------------------------------------------------------------------ */

static int
test_disk_zero_dst(void)
{
	char buf[1] = {0};
	unsigned int interval = 0;

	printf("  [edge 4] b_write_disk_usage_percent small dst (size=1) ... ");
	char *end = b_write_disk_usage_percent(buf, 1, "/", &interval);
	CHECK(end != NULL, "expected non-NULL return");
	/* With dst_size=1, the function should truncate. */
	if (end != NULL)
		printf("PASS (returned, wrote %td bytes)\n", end - buf);
	else
		printf("FAIL\n");
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Test 5 — consecutive calls to same block produce different output */
/* ------------------------------------------------------------------ */

static int
test_consecutive_calls(void)
{
	char buf1[64] = {0};
	char buf2[64] = {0};
	unsigned int interval = 0;

	printf("  [edge 5] two calls within same minute yield same data   ... ");
	b_write_time(buf1, sizeof(buf1), NULL, &interval);
	b_write_time(buf2, sizeof(buf2), NULL, &interval);
	/* Both calls are within the same second, output must match. */
	CHECK(memcmp(buf1, buf2, sizeof(buf1)) == 0, "expected identical output");
	if (memcmp(buf1, buf2, sizeof(buf1)) == 0)
		printf("PASS\n");
	else
		printf("FAIL\n");
	return 0;
}

/* ------------------------------------------------------------------ */
/*  main                                                              */
/* ------------------------------------------------------------------ */

int
main(void)
{
	printf("dwmblocks-fast edge-case tests\n");
	printf("==============================\n\n");

	test_null_arg();
	test_empty_arg();
	test_interval_modified();
	test_disk_zero_dst();
	test_consecutive_calls();

	printf("\n%s: %s\n",
	       nfail ? "FAIL" : "PASS",
	       nfail ? "some edge tests failed" : "all edge tests passed");
	return nfail ? 1 : 0;
}
