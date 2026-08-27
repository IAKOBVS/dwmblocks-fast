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

#include "../blocks/procfs.h"
#include "../utils.h"

/* Satisfy extern reference from block object files. */
unsigned int g_time;

/* Block function prototypes */
extern char *b_write_date(char *dst, unsigned int dst_size,
                          const char *unused, unsigned short *interval);
extern char *b_write_time(char *dst, unsigned int dst_size,
                          const char *unused, unsigned short *interval);
extern char *b_write_cpu_usage(char *dst, unsigned int dst_size,
                               const char *unused, unsigned short *interval);
extern char *b_write_ram_usage_percent(char *dst, unsigned int dst_size,
                                       const char *unused, unsigned short *interval);
extern char *b_write_ram_usage_available(char *dst, unsigned int dst_size,
                                         const char *unused, unsigned short *interval);
extern char *b_write_disk_usage_percent(char *dst, unsigned int dst_size,
                                        const char *path, unsigned short *interval);
extern char *b_write_disk_usage_free(char *dst, unsigned int dst_size,
                                     const char *path, unsigned short *interval);
extern unsigned long long b_cpu_energy_diff(unsigned long long curr,
                                            unsigned long long last,
                                            unsigned long long max_range_uj);

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
	unsigned short interval = 0;

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
	unsigned short interval = 0;

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
	unsigned short interval = 999;

	printf("  [edge 3] b_write_time sets interval to < 90 sec       ... ");
	b_write_time(buf, sizeof(buf), NULL, &interval);
	CHECK(interval <= 90, "expected interval <= 90");
	CHECK(interval > 0, "expected interval > 0");
	if (interval > 0 && interval <= 90)
		printf("PASS (interval=%u)\n", (unsigned int)interval);
	else
		printf("FAIL\n");
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Test 4 — b_write_disk_usage_percent on a valid path               */
/* ------------------------------------------------------------------ */
/* NOTE: block writers are unbounded and require an adequately sized
 * dst (production rows are G_STATUSBLOCKLEN bytes). */

static int
test_disk_valid_path(void)
{
	char buf[64] = {0};
	unsigned short interval = 0;

	printf("  [edge 4] b_write_disk_usage_percent valid path (\"/\")    ... ");
	char *end = b_write_disk_usage_percent(buf, sizeof(buf), "/", &interval);
	CHECK(end != NULL, "expected non-NULL return");
	CHECK(end > buf, "expected data written");
	CHECK(strlen(buf) > 0 && strlen(buf) <= 3, "expected short percent string");
	if (end != NULL && end > buf)
		printf("PASS (wrote \"%s\")\n", buf);
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
	unsigned short interval = 0;

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
/*  Test 6 — procfs iterator                                          */
/* ------------------------------------------------------------------ */

static int
test_procfs_iterator(void)
{
	printf("  [edge 6] procfs iterator tests                       ... ");
	const char *test_data =
		"MemTotal:       16315212 kB\n"
		"MemFree:          821344 kB\n"
		"MemAvailable:    5671234 kB\n"
		"NoDelimiterLine\n"
		"  WithSpaces   :   ValueWithSpaces   \n"
		"\n"
		"EmptyKey: \n"
		" : EmptyValue\n"
		"TrailingNoNewline: last";

	struct b_proc_iter iter;
	b_proc_iter_init(&iter, test_data, strlen(test_data));

	const char *k, *v;
	unsigned int kl, vl;

	/* 1. MemTotal */
	CHECK(b_proc_iter_next(&iter, &k, &kl, &v, &vl, ':') == 1, "expected next pair");
	CHECK(kl == 8 && memcmp(k, "MemTotal", 8) == 0, "key should be MemTotal");
	CHECK(vl == 11 && memcmp(v, "16315212 kB", 11) == 0, "val should be 16315212 kB");

	/* 2. MemFree */
	CHECK(b_proc_iter_next(&iter, &k, &kl, &v, &vl, ':') == 1, "expected next pair");
	CHECK(kl == 7 && memcmp(k, "MemFree", 7) == 0, "key should be MemFree");
	CHECK(vl == 9 && memcmp(v, "821344 kB", 9) == 0, "val should be 821344 kB");

	/* 3. MemAvailable */
	CHECK(b_proc_iter_next(&iter, &k, &kl, &v, &vl, ':') == 1, "expected next pair");
	CHECK(kl == 12 && memcmp(k, "MemAvailable", 12) == 0, "key should be MemAvailable");
	CHECK(vl == 10 && memcmp(v, "5671234 kB", 10) == 0, "val should be 5671234 kB");

	/* NoDelimiterLine gets skipped */

	/* 4. WithSpaces */
	CHECK(b_proc_iter_next(&iter, &k, &kl, &v, &vl, ':') == 1, "expected next pair");
	CHECK(kl == 10 && memcmp(k, "WithSpaces", 10) == 0, "key should be WithSpaces");
	CHECK(vl == 15 && memcmp(v, "ValueWithSpaces", 15) == 0, "val should be ValueWithSpaces");

	/* Empty lines get skipped */

	/* 5. EmptyKey */
	CHECK(b_proc_iter_next(&iter, &k, &kl, &v, &vl, ':') == 1, "expected next pair");
	CHECK(kl == 8 && memcmp(k, "EmptyKey", 8) == 0, "key should be EmptyKey");
	CHECK(vl == 0, "val should be empty");

	/* 6. EmptyValue */
	CHECK(b_proc_iter_next(&iter, &k, &kl, &v, &vl, ':') == 1, "expected next pair");
	CHECK(kl == 0, "key should be empty");
	CHECK(vl == 10 && memcmp(v, "EmptyValue", 10) == 0, "val should be EmptyValue");

	/* 7. TrailingNoNewline */
	CHECK(b_proc_iter_next(&iter, &k, &kl, &v, &vl, ':') == 1, "expected next pair");
	CHECK(kl == 17 && memcmp(k, "TrailingNoNewline", 17) == 0, "key should be TrailingNoNewline");
	CHECK(vl == 4 && memcmp(v, "last", 4) == 0, "val should be last");

	/* End of iteration */
	CHECK(b_proc_iter_next(&iter, &k, &kl, &v, &vl, ':') == 0, "expected no more pairs");

	printf("PASS\n");
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Test 7 — u_strtoull10 must return the full 64-bit value           */
/* ------------------------------------------------------------------ */
/* Regression: u_strtoull10 used to return unsigned int, truncating the
 * RAPL energy_uj counter (max 262143328850 uJ > UINT32_MAX) to 32 bits,
 * producing bogus power readings. */

static int
test_u_strtoull10(void)
{
	printf("  [edge 7] u_strtoull10 returns full 64-bit value        ... ");
	const char *unused;
	const unsigned long long v = u_strtoull10("262143328850", &unused);
	CHECK(v == 262143328850ULL, "expected full 64-bit parse, no 32-bit truncation");
	CHECK(v > 4294967295ULL, "value must exceed the 32-bit range");
	if (v == 262143328850ULL)
		printf("PASS (v=%llu)\n", v);
	else
		printf("FAIL (v=%llu)\n", v);
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Test 8 — RAPL energy counter wrap must not produce negative power  */
/* ------------------------------------------------------------------ */
/* Regression: b_read_cpu_usage_power computed curr-last in int; when
 * the counter wrapped (max 262143328850 uJ, not a multiple of 2^32),
 * the result could be exactly -1, which the caller treated as an error
 * and aborted with assert(0) at cpu.c:136. */

static int
test_cpu_energy_wrap(void)
{
	printf("  [edge 8] b_cpu_energy_diff across RAPL wrap            ... ");
	const unsigned long long max_range = 262143328850ULL;

	/* normal: counter increased */
	const unsigned long long d1 = b_cpu_energy_diff(100000150000ULL, 100000000000ULL, max_range);
	CHECK(d1 == 150000ULL, "normal delta wrong");

	/* wrap: last near max, curr just past 0 */
	const unsigned long long last_wrap = 262143300000ULL;
	const unsigned long long d2 = b_cpu_energy_diff(30000000ULL, last_wrap, max_range);
	CHECK(d2 == 30000000ULL + (max_range - last_wrap), "wrap delta wrong");
	/* naive u64 subtraction would be 2^64 - last + curr (huge); must stay small */
	CHECK(d2 < 1000000000ULL, "wrap delta must stay small and positive");

	/* no negative, no crash-inducing -1: power over 2 s must be >= 0 */
	const int watts = (int)((double)d2 / (2.0 * 1000000.0));
	CHECK(watts >= 0, "power must never be negative");

	/* delta must never exceed the counter range */
	const unsigned long long d3 = b_cpu_energy_diff(5, max_range - 5, max_range);
	CHECK(d3 > 0 && d3 <= max_range, "wrap delta must be positive and <= max range");

	/* equal snapshots */
	CHECK(b_cpu_energy_diff(123, 123, max_range) == 0, "equal values -> 0");

	if (d1 == 150000ULL && d2 == 30000000ULL + (max_range - last_wrap) && watts >= 0)
		printf("PASS (wrap delta=%llu uJ, %d W)\n", d2, watts);
	else
		printf("FAIL\n");
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Test 9 — value_get must find keys late in the buffer              */
/* ------------------------------------------------------------------ */
/* Regression: value_get's length bookkeeping over-subtracted the
 * absolute offset each iteration, shrinking the scan window faster
 * than the data and even underflowing into OOB reads for keys late
 * in the buffer. */

static int
test_proc_value_get(void)
{
	printf("  [edge 9] b_proc_value_getull finds late keys           ... ");
	int ok = 1;

	/* Audit repro: filler forces one memchr jump, and the tail
	 * (< 2 * key_len bytes) made the old bookkeeping underflow and
	 * miss the key entirely. */
	const char *buf1 = "xx xx xx xx xx xx MemAvailable: 12345\n";
	const unsigned long long v1 = b_proc_value_getull(buf1, (unsigned int)strlen(buf1), S_LITERAL("MemAvailable"), ':', ' ');
	CHECK(v1 == 12345ULL, "expected 12345 parsed from a key late in the buffer");
	ok = ok && v1 == 12345ULL;

	/* meminfo-shaped input where the wanted key is not the first one. */
	const char *buf2 =
		"MemTotal:       16315212 kB\n"
		"MemFree:          821344 kB\n"
		"MemAvailable:    5671234 kB\n";
	const unsigned long long v2 = b_proc_value_getull(buf2, (unsigned int)strlen(buf2), S_LITERAL("MemAvailable"), ':', ' ');
	CHECK(v2 == 5671234ULL, "expected 5671234 parsed from meminfo");
	ok = ok && v2 == 5671234ULL;

	/* Missing key must return the error sentinel. */
	const char *buf3 = "a a a a a a a a a\n";
	const unsigned long long v3 = b_proc_value_getull(buf3, (unsigned int)strlen(buf3), S_LITERAL("Nope"), ' ', ' ');
	CHECK(v3 == (unsigned long long)-1, "missing key must yield -1 sentinel");
	ok = ok && v3 == (unsigned long long)-1;

	if (ok)
		printf("PASS\n");
	else
		printf("FAIL\n");
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Test 10 — utils boundary matrix                                   */
/* ------------------------------------------------------------------ */

static int
test_utils_matrix(void)
{
	printf("  [edge 10] utils boundary matrix                       ... ");
	int ok = 1;
	char buf[64];

	/* u_utoa_p: zero and UINT_MAX. NOTE: writers return the end
	 * pointer (at '\0'), so assert on buf contents. */
	u_utoa_p(0, buf);
	CHECK(strcmp(buf, "0") == 0, "utoa(0)");
	ok = ok && strcmp(buf, "0") == 0;
	u_utoa_p(4294967295u, buf);
	CHECK(strcmp(buf, "4294967295") == 0, "utoa(UINT_MAX)");
	ok = ok && strcmp(buf, "4294967295") == 0;

	/* u_ulltoa_p: ULLONG_MAX = 20 digits. */
	u_ulltoa_p(18446744073709551615ULL, buf);
	CHECK(strcmp(buf, "18446744073709551615") == 0, "ulltoa(ULLONG_MAX)");
	ok = ok && strlen(buf) == 20;

	/* u_utoa_le2_p / le3_p digit widths. */
	u_utoa_le2_p(7, buf);
	CHECK(strcmp(buf, "7") == 0, "le2(7)");
	ok = ok && strcmp(buf, "7") == 0;
	u_utoa_le3_p(999, buf);
	CHECK(strcmp(buf, "999") == 0, "le3(999)");
	ok = ok && strcmp(buf, "999") == 0;
	/* u_utoa_le3_p: 3-digit fast path. Contract: num <= 999
	 * (enforced by assert in debug builds). Unbounded callers must
	 * use u_utoa_p — b_write_cpu_usage_power does, for >999 W. */
	u_utoa_le3_p(100, buf);
	CHECK(strcmp(buf, "100") == 0, "le3(100)");
	ok = ok && strcmp(buf, "100") == 0;
	u_utoa_le3_p(999, buf);
	CHECK(strcmp(buf, "999") == 0, "le3(999 upper bound)");
	ok = ok && strcmp(buf, "999") == 0;
	u_utoa_le3_p(0, buf);
	CHECK(strcmp(buf, "0") == 0, "le3(0)");
	ok = ok && strcmp(buf, "0") == 0;

	/* u_write_humanize boundaries. */
	struct { unsigned long long in; const char *out; } hum[] = {
		{ 0ULL, "0" }, { 1023ULL, "1023" }, { 1024ULL, "1K" },
		{ 1048575ULL, "1023K" }, { 1048576ULL, "1M" },
		{ 1073741824ULL, "1G" }, { 1099511627776ULL, "1T" },
	};
	for (unsigned int i = 0; i < sizeof(hum) / sizeof(hum[0]); ++i) {
		u_write_humanized(buf, hum[i].in);
		if (strcmp(buf, hum[i].out) != 0) {
			char msg[96];
			snprintf(msg, sizeof(msg), "humanize(%llu) -> \"%s\", want \"%s\"",
				 hum[i].in, buf, hum[i].out);
			CHECK(0, msg);
			ok = 0;
		}
	}

	/* u_stpcpy_len with empty string. */
	CHECK(u_stpcpy_len(buf, "", 0) == buf && *buf == '\0', "stpcpy_len empty");

	if (ok)
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
	test_disk_valid_path();
	test_consecutive_calls();
	test_procfs_iterator();
	test_u_strtoull10();
	test_cpu_energy_wrap();
	test_proc_value_get();
	test_utils_matrix();

	printf("\n%s: %s\n",
	       nfail ? "FAIL" : "PASS",
	       nfail ? "some edge tests failed" : "all edge tests passed");
	return nfail ? 1 : 0;
}
