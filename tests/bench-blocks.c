/* SPDX-License-Identifier: ISC */
/* Microbenchmark: statusbar block dispatch layouts.
 *
 * Compares ns/tick of the g_getcmds() dispatch loop under three
 * layouts, holding the work per fired block constant:
 *
 *   A  aos       plain array-of-structs, original (unsorted) order,
 *                sleep counter stored inside the struct
 *   B  aos-sort  same, array pre-sorted by (interval, signal)
 *   C  soa-sort  production layout: sorted order + parallel arrays
 *                (func/arg/interval/sleep/tostatus), as in
 *                dwmblocks-fast.c b_init()/g_getcmds()
 *   D  soa-mod   like C, but scheduling via g_time % interval == 0
 *                instead of per-block sleep counters
 *
 * Block functions are stubs shaped like the real writers (write
 * 0-21 bytes, reset *interval). Only the dispatch differs.
 *
 * Build: cc -O2 -flto -march=native -o bench tests/bench-blocks.c
 */

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ROWLEN 32

volatile unsigned long long sink;

typedef char *(*fn_ty)(char *dst, unsigned int dst_size, const char *arg, unsigned short *interval);

/* Stubs mimicking real writers: emit 0-21 bytes, reset *interval. */
static char *
w_cpu(char *dst, unsigned int dst_size, const char *arg, unsigned short *interval)
{
	(void)arg;
	(void)dst_size;
	dst[0] = '4';
	dst[1] = '6';
	dst[2] = '\0';
	*interval = 29;
	return dst + 2;
}

static char *
w_time(char *dst, unsigned int dst_size, const char *arg, unsigned short *interval)
{
	(void)arg;
	(void)dst_size;
	memcpy(dst, "9:41 PM", 7);
	dst[7] = '\0';
	*interval = 89;
	return dst + 7;
}

static char *
w_disk(char *dst, unsigned int dst_size, const char *arg, unsigned short *interval)
{
	(void)arg;
	(void)dst_size;
	dst[0] = '8';
	dst[1] = '2';
	dst[2] = 'M';
	dst[3] = '\0';
	*interval = 29;
	return dst + 3;
}

static char *
w_date(char *dst, unsigned int dst_size, const char *arg, unsigned short *interval)
{
	(void)arg;
	(void)dst_size;
	memcpy(dst, "Sat, 23 Aug 2026", 16);
	dst[16] = '\0';
	*interval = 3599;
	return dst + 16;
}

static char *
w_icon_off(char *dst, unsigned int dst_size, const char *arg, unsigned short *interval)
{
	(void)arg;
	(void)dst_size;
	dst[0] = '\0';
	*interval = 65534;
	return dst;
}

static char *
w_power(char *dst, unsigned int dst_size, const char *arg, unsigned short *interval)
{
	(void)arg;
	(void)dst_size;
	memcpy(dst, "15W", 3);
	dst[3] = '\0';
	*interval = 1;
	return dst + 3;
}

static const fn_ty g_stubs[] = { w_cpu, w_time, w_disk, w_date, w_icon_off, w_power };

/* Interval profile mirroring the default blocks.def.h mix. */
static unsigned short
iv_of(unsigned int i)
{
	static const unsigned short pat[] = { 2, 2, 30, 2, 65535, 30, 2, 65535, 30, 2, 3600, 30, 65535, 59 };
	return pat[i % (sizeof(pat) / sizeof(pat[0]))];
}

static unsigned char
sig_of(unsigned int i)
{
	static const unsigned char pat[] = { 0, 0, 0, 1, 2, 0, 0, 3, 0, 4, 0, 0, 0, 0 };
	return pat[i % (sizeof(pat) / sizeof(pat[0]))];
}

/* --- Layouts ------------------------------------------------------- */

typedef struct {
	fn_ty func;
	const char *arg;
	const char *pad_left;
	const char *pad_right;
	unsigned short interval;
	unsigned short sleep;
	unsigned char signal;
	unsigned char idx;
} aos_ty;


static void
run_aos(aos_ty *b, unsigned int n, unsigned int ticks)
{
	for (unsigned int t = 0; t < ticks; ++t)
		for (unsigned int i = 0; i < n; ++i) {
			if (b[i].sleep-- > 0)
				continue;
			b[i].sleep = b[i].interval - 1;
			char tmp[ROWLEN];
			char *e = b[i].func(tmp, ROWLEN, b[i].arg, &b[i].sleep);
			sink += (unsigned long long)(e - tmp) + b[i].idx;
		}
}


static void
run_soa(fn_ty *f, const char **arg, unsigned short *sleep, unsigned short *itv, unsigned char *idx, unsigned int n, unsigned int ticks)
{
	for (unsigned int t = 0; t < ticks; ++t)
		for (unsigned int i = 0; i < n; ++i) {
			if (sleep[i]-- > 0)
				continue;
			sleep[i] = itv[i] - 1;
			char tmp[ROWLEN];
			char *e = f[i](tmp, ROWLEN, arg[i], &sleep[i]);
			sink += (unsigned long long)(e - tmp) + idx[i];
		}
}

/* Variant D: like C, but schedule via global-time modulo instead of
 * per-block sleep counters. No state writes on the skip path; pays a
 * division per block per tick instead. */
static void
run_soa_mod(fn_ty *f, const char **arg, unsigned short *itv, unsigned char *idx, unsigned int n, unsigned int ticks)
{
	unsigned int gtime = 0;
	for (unsigned int t = 0; t < ticks; ++t, ++gtime)
		for (unsigned int i = 0; i < n; ++i) {
			if (gtime % (unsigned int)itv[i] != 0)
				continue;
			char tmp[ROWLEN];
			unsigned short dummy;
			char *e = f[i](tmp, ROWLEN, arg[i], &dummy);
			sink += (unsigned long long)(e - tmp) + idx[i];
		}
}

/* --- Harness -------------------------------------------------------- */

static double
now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static int
cmp_u(const void *a, const void *b)
{
	const double x = *(const double *)a;
	const double y = *(const double *)b;
	return (x > y) - (x < y);
}

static int
cmp_blk(const void *a, const void *b)
{
	const aos_ty *p = (const aos_ty *)a;
	const aos_ty *q = (const aos_ty *)b;
	if (p->interval != q->interval)
		return (p->interval > q->interval) - (p->interval < q->interval);
	return (p->signal > q->signal) - (p->signal < q->signal);
}

static double
median(double *v, unsigned int n)
{
	qsort(v, n, sizeof(v[0]), cmp_u);
	return v[n / 2];
}

int
main(int argc, char **argv)
{
	static const unsigned int ns[] = { 20, 64, 256, 1024, 4096, 16384, 65536 };
	const unsigned int nn = sizeof(ns) / sizeof(ns[0]);
	unsigned int rounds = 11;
	if (argc > 1)
		rounds = (unsigned int)atoi(argv[1]);

	printf("block-dispatch microbenchmark (stubs, dispatch only)\n");
	printf("rows: %zu bytes, stubs: %zu, rounds: %u\n\n", sizeof(aos_ty), sizeof(g_stubs) / sizeof(g_stubs[0]), rounds);

	double *ra = malloc(sizeof(double) * rounds);
	double *rb = malloc(sizeof(double) * rounds);
	double *rc = malloc(sizeof(double) * rounds);
	double *rd = malloc(sizeof(double) * rounds);
	if (!ra || !rb || !rc || !rd)
		return 1;

	for (unsigned int ni = 0; ni < nn; ++ni) {
		const unsigned int n = ns[ni];
		/* Enough ticks for a stable measurement at any n. */
		const unsigned int ticks = n <= 32 ? 2000000u : n <= 128 ? 400000u : n <= 512 ? 100000u : n <= 2048 ? 25000u : n <= 8192 ? 8000u : n <= 32768 ? 2000u : 600u;

		aos_ty *aos = aligned_alloc(64, ((sizeof(aos_ty) * n + 63) / 64) * 64);
		aos_ty *aos_sorted = aligned_alloc(64, ((sizeof(aos_ty) * n + 63) / 64) * 64);
		fn_ty *f = aligned_alloc(64, ((sizeof(fn_ty) * n + 63) / 64) * 64);
		const char **arg = aligned_alloc(64, ((sizeof(*arg) * n + 63) / 64) * 64);
		unsigned short *slp = aligned_alloc(64, ((sizeof(short) * n + 63) / 64) * 64);
		unsigned short *itv = aligned_alloc(64, ((sizeof(short) * n + 63) / 64) * 64);
		unsigned char *idx = aligned_alloc(64, ((sizeof(char) * n + 63) / 64) * 64);
		if (!aos || !aos_sorted || !f || !arg || !slp || !itv || !idx)
			return 1;

		/* Identical logical content for all variants. */
		for (unsigned int i = 0; i < n; ++i) {
			fn_ty fn = g_stubs[i % (sizeof(g_stubs) / sizeof(g_stubs[0]))];
			aos[i] = (aos_ty){ .func = fn, .arg = NULL, .pad_left = "", .pad_right = "", .interval = iv_of(i), .sleep = 0, .signal = sig_of(i), .idx = (unsigned char)i };
		}
		/* Sort a copy by (interval, signal), keeping original order
		 * in aos[]. SoA views follow the SAME sorted order. */
		memcpy(aos_sorted, aos, sizeof(aos_ty) * n);
		qsort(aos_sorted, n, sizeof(aos_ty), cmp_blk);
		/* SoA views follow the SAME sorted order. */
		for (unsigned int i = 0; i < n; ++i) {
			f[i] = aos_sorted[i].func;
			arg[i] = aos_sorted[i].arg;
			itv[i] = aos_sorted[i].interval;
			idx[i] = aos_sorted[i].idx;
		}

		/* Warmup (untimed). */
		run_aos(aos, n, ticks / 10);
		run_aos(aos_sorted, n, ticks / 10);
		run_soa(f, arg, slp, itv, idx, n, ticks / 10);
		run_soa_mod(f, arg, itv, idx, n, ticks / 10);

		for (unsigned int r = 0; r < rounds; ++r) {
			/* Reset schedules each round. */
			for (unsigned int i = 0; i < n; ++i) {
				aos[i].sleep = 0;
				aos_sorted[i].sleep = 0;
				slp[i] = 0;
			}
			double t0 = now_ns();
			run_aos(aos, n, ticks);
			ra[r] = (now_ns() - t0) / ticks;

			t0 = now_ns();
			run_aos(aos_sorted, n, ticks);
			rb[r] = (now_ns() - t0) / ticks;

			t0 = now_ns();
			run_soa(f, arg, slp, itv, idx, n, ticks);
			rc[r] = (now_ns() - t0) / ticks;

			t0 = now_ns();
			run_soa_mod(f, arg, itv, idx, n, ticks);
			rd[r] = (now_ns() - t0) / ticks;
		}

		const double ma = median(ra, rounds);
		const double mb = median(rb, rounds);
		const double mc = median(rc, rounds);
		const double md = median(rd, rounds);
		double ia = ra[0], ib = rb[0], ic = rc[0], id = rd[0];
		for (unsigned int r = 0; r < rounds; ++r) {
			if (ra[r] < ia) ia = ra[r];
			if (rb[r] < ib) ib = rb[r];
			if (rc[r] < ic) ic = rc[r];
			if (rd[r] < id) id = rd[r];
		}
		printf("n=%5u ticks=%u   (aos %zu KB, soa-skip arr %zu KB)\n", n, ticks, sizeof(aos_ty) * n / 1024, sizeof(short) * n / 1024 + 1);
		printf("  aos       med %8.2f ns/tick  min %8.2f  (%6.2f ns/block)\n", ma, ia, ma / n);
		printf("  aos-sort  med %8.2f ns/tick  min %8.2f  (%6.2f ns/block)  %.2fx vs aos\n", mb, ib, mb / n, ma / mb);
		printf("  soa-sort  med %8.2f ns/tick  min %8.2f  (%6.2f ns/block)  %.2fx vs aos\n", mc, ic, mc / n, ma / mc);
		printf("  soa-mod   med %8.2f ns/tick  min %8.2f  (%6.2f ns/block)  %.2fx vs soa-sort\n", md, id, md / n, mc / md);
		printf("\n");

		free(aos);
		free(aos_sorted);
		free(f);
		free(arg);
		free(slp);
		free(itv);
		free(idx);
	}

	fprintf(stderr, "(sink=%llu)\n", sink);
	free(ra);
	free(rb);
	free(rc);
	free(rd);
	return 0;
}
