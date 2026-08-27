/* SPDX-License-Identifier: ISC */
/* Copyright 2025-2026 James Tirta Halim <tirtajames45 at gmail dot com>
 *
 * TZ-pinned content tests for the time/date blocks.
 *
 * Each case runs in a forked child because b_read_time caches the UTC
 * offset on first use; a fresh address space per case lets every case
 * initialize that cache under its own manipulated TZ.
 *
 * Each case picks a POSIX TZ offset forcing the desired local hour,
 * then checks rendered output against an independently coded oracle
 * computed from the same wall-clock sample. Samples near second 55+
 * or across clock ticks are retried to stay deterministic.
 *
 * Build:
 *   cc -o tests/test-time-bin tests/test-time.c $(OBJS) $(REQ) $(LDFLAGS)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../blocks/time.h"

static const char MONTHS[12][4] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char DAYS[7][4] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static int nfail;

#define CHECK(cond, msg) do {                                   \
        if (!(cond)) {                                          \
                fprintf(stderr, "  FAIL  %s:%d: %s\n",          \
                        __FILE__, __LINE__, msg);               \
                ++nfail;                                        \
        }                                                       \
} while (0)

/* POSIX TZ offsets are sign-inverted: "UTC-N" = N hours east of UTC. */
static void
set_tz_east(unsigned int hours)
{
	char tz[32];
	snprintf(tz, sizeof(tz), "UTC-%u", hours);
	setenv("TZ", tz, 1);
	tzset();
}

/* Render expected "H:MM AM/PM" for local hour h (0-23), minute m. */
static void
expect_time(char *out, size_t out_sz, unsigned int h, unsigned int m)
{
	const char meridiem = (h >= 12) ? 'P' : 'A';
	if (h >= 12)
		h = (h > 12) ? h - 12 : 12;
	else if (h == 0)
		h = 12;
	snprintf(out, out_sz, "%u:%02u %cM", h, m, meridiem);
}

static void
case_time_hour(unsigned int want_hour)
{
	for (int attempt = 0; attempt < 32; ++attempt) {
		/* Offset that makes local hour == want_hour right now. */
		const time_t t = time(NULL);
		const unsigned int utc_h = (unsigned int)((t / 3600) % 24);
		set_tz_east(((want_hour - utc_h) + 24) % 24);

		const time_t t2 = time(NULL);
		if (t2 != t)
			continue;
		time_t local = t2 + (time_t)(((want_hour - utc_h) + 24) % 24) * 3600;
		const struct tm exp_tm = *gmtime(&local);
		if ((unsigned int)exp_tm.tm_hour != want_hour || exp_tm.tm_sec > 56)
			continue;

		char buf[64] = {0};
		unsigned short interval = 0;
		char *end = b_write_time(buf, sizeof(buf), NULL, &interval);
		CHECK(end != NULL, "writer returned NULL");

		char exp[32];
		expect_time(exp, sizeof(exp), (unsigned int)exp_tm.tm_hour, (unsigned int)exp_tm.tm_min);
		char msg[96];
		snprintf(msg, sizeof(msg), "\"%s\" != expected \"%s\"", buf, exp);
		CHECK(strcmp(buf, exp) == 0, msg);
		CHECK(interval > 0 && interval <= 90, "interval not in (0, 90]");
		return;
	}
	fprintf(stderr, "  FAIL  could not pin local hour %u\n", want_hour);
	++nfail;
}

static void
case_date_content(void)
{
	set_tz_east(0);
	for (int attempt = 0; attempt < 8; ++attempt) {
		const time_t t = time(NULL);

		char buf[64] = {0};
		unsigned short interval = 0;
		char *end = b_write_date(buf, sizeof(buf), NULL, &interval);
		CHECK(end != NULL, "writer returned NULL");

		const time_t t2 = time(NULL);
		/* Stale sample (clock ticked around the write): retry. */
		if (t2 != t || localtime(&t2)->tm_sec > 56) {
			sleep(1);
			continue;
		}

		const struct tm tm1 = *localtime(&t);
		char exp[40];
		snprintf(exp, sizeof(exp), "%s, %02u %s %u",
			 DAYS[tm1.tm_wday], (unsigned)tm1.tm_mday,
			 MONTHS[tm1.tm_mon], (unsigned)(tm1.tm_year + 1900));
		char msg[128];
		snprintf(msg, sizeof(msg), "\"%s\" != expected \"%s\"", buf, exp);
		CHECK(strcmp(buf, exp) == 0, msg);
		CHECK(interval > 0, "date interval must be positive");
		/* PT9 regression: pre-fix the value wrapped below an hour. */
		CHECK(interval >= 3600 || interval <= 90,
		      "date interval wrapped unexpectedly");
		return;
	}
	fprintf(stderr, "  FAIL  could not get a stable date sample\n");
	++nfail;
}

/* Fork-isolate one case so the utc_off cache starts clean. Returns
 * 0 on success. */
static int
run_isolated(void (*fn)(void))
{
	const pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
		return 1;
	}
	if (pid == 0) {
		fn();
		_Exit(nfail ? 1 : 0);
	}
	int wstatus;
	waitpid(pid, &wstatus, 0);
	return (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0) ? 0 : 1;
}

static void
case_midnight(void) { case_time_hour(0); }

static void
case_noon(void) { case_time_hour(12); }

static void
case_afternoon(void) { case_time_hour(13); }

int
main(void)
{
	printf("dwmblocks-fast time block tests (TZ-pinned)\n");
	printf("============================================\n\n");

	int fails = 0;
	int r;

	r = run_isolated(case_midnight);
	printf("  [time 1] midnight renders 12:MM AM                   ... %s\n", r ? "FAIL" : "PASS");
	fails += r;

	r = run_isolated(case_noon);
	printf("  [time 2] noon renders 12:MM PM                       ... %s\n", r ? "FAIL" : "PASS");
	fails += r;

	r = run_isolated(case_afternoon);
	printf("  [time 3] 13:xx renders 1:MM PM                       ... %s\n", r ? "FAIL" : "PASS");
	fails += r;

	r = run_isolated(case_date_content);
	printf("  [date 1] content matches oracle (TZ=UTC)             ... %s\n", r ? "FAIL" : "PASS");
	fails += r;

	printf("\n%s: %s\n",
	       fails ? "FAIL" : "PASS",
	       fails ? "some time tests failed" : "all time tests passed");
	return fails ? 1 : 0;
}
