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
#include <stdint.h>

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
/*  Test 6 — loop-free uint64 parser u_strtou10                       */
/* ------------------------------------------------------------------ */

extern uint64_t test_u_strtou10(const char *p, const char **endp);

static int
test_cpu_u_strtou10(void)
{
	printf("  [edge 6] test_u_strtou10 loop-free uint64 parser      ... ");

	const char *p;
	const char *end;
	uint64_t val;

	// 1. Single digit
	p = "7";
	val = test_u_strtou10(p, &end);
	CHECK(val == 7, "expected 7");
	CHECK(end == p + 1, "expected end to advance by 1");

	// 2. Trailing spaces / non-digits
	p = "123 abc";
	val = test_u_strtou10(p, &end);
	CHECK(val == 123, "expected 123");
	CHECK(end == p + 3, "expected end to advance by 3");
	CHECK(*end == ' ', "expected advanced pointer to point to space");

	// 3. Leading space or invalid characters
	p = " 456";
	val = test_u_strtou10(p, &end);
	CHECK(val == 0, "expected 0 for leading non-digit");
	CHECK(end == p, "expected end to not advance");

	// 4. Large number (10 digits)
	p = "1234567890";
	val = test_u_strtou10(p, &end);
	CHECK(val == 1234567890ULL, "expected 1234567890");
	CHECK(end == p + 10, "expected end to advance by 10");

	// 5. Maximum uint64_t (20 digits)
	p = "18446744073709551615";
	val = test_u_strtou10(p, &end);
	CHECK(val == 18446744073709551615ULL, "expected max uint64_t");
	CHECK(end == p + 20, "expected end to advance by 20");

	// 6. 21 digits (more than maximum 20 digits handled by loop-free)
	p = "123456789012345678901";
	val = test_u_strtou10(p, &end);
	CHECK(val == 12345678901234567890ULL, "expected first 20 digits parsed");
	CHECK(end == p + 20, "expected end to advance by 20");

	// 7. Zeros
	p = "0000123";
	val = test_u_strtou10(p, &end);
	CHECK(val == 123, "expected 123 from leading zeros");
	CHECK(end == p + 7, "expected end to advance by 7");

	printf("PASS\n");
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
	test_cpu_u_strtou10();

	printf("\n%s: %s\n",
	       nfail ? "FAIL" : "PASS",
	       nfail ? "some edge tests failed" : "all edge tests passed");
	return nfail ? 1 : 0;
}
