/* SPDX-License-Identifier: ISC */
/* Copyright 2025-2026 James Tirta Halim <tirtajames45 at gmail dot com>
 * This file is part of dwmblocks-fast.
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

#include "../config.h"

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "../macros.h"
#include "../utils.h"
#include "../blocks/temp.h"
#include "procfs.h"

#define SIZE_T_MAX_DIGITS 20

static int fd_cpu_usage = -1;
static int fd_cpu_usage_power = -1;
static int fd_cpu_temp = -1;

/* Single attempt; no startup sleep-retry loops (they froze the bar for
 * up to 10 s per missing file on machines without RAPL/temp sensors).
 * Missing files degrade to placeholder rendering instead. */
static int
b_cpu_init(const char *filename)
{
	return open(filename, O_RDONLY);
}

typedef struct {
	unsigned long long user, nice, system, idle, iowait, irq, softirq;
	unsigned long long time;
	unsigned long long cpu_time;
} time_ty;
static time_ty last;

static int
b_read_cpu_usage(void)
{
	if (unlikely(fd_cpu_usage == -1)) {
		fd_cpu_usage = b_cpu_init("/proc/stat");
		if (unlikely(fd_cpu_usage < 0))
			DIE(return -1);
	}
	char buf[B_PAGE_SIZE + 1];
	const unsigned int read_sz = b_proc_read_filefd(buf, sizeof(buf), fd_cpu_usage);
	if (unlikely(read_sz == (unsigned int)-1))
		DIE(return -1);
	time_ty curr;
	const char *p = buf;
	/* clang-format off */
	p += S_LEN("CPU  ");
	curr.user = (int)u_strtou10(p, &p); p += S_LEN(" ");
	curr.nice = (int)u_strtou10(p, &p); p += S_LEN(" ");
	curr.system = (int)u_strtou10(p, &p); p += S_LEN(" ");
	curr.idle = (int)u_strtou10(p, &p); p += S_LEN(" ");
	curr.iowait = (int)u_strtou10(p, &p); p += S_LEN(" ");
	curr.irq = (int)u_strtou10(p, &p); p += S_LEN(" ");
	curr.softirq = (int)u_strtou10(p, &p);
	/* clang-format on */
	curr.time = curr.user + curr.nice + curr.system + curr.idle + curr.iowait + curr.irq + curr.softirq;
	curr.cpu_time = curr.user + curr.nice + curr.system + curr.irq + curr.softirq;
	if (unlikely(curr.time == 0))
		return 0;
	const int time_diff = curr.time - last.time;
	/* Two samples within the same jiffy: avoid dividing by zero. */
	if (unlikely(time_diff == 0)) {
		last = curr;
		return 0;
	}
	const int usage = (int)((long double)100 * ((long double)(curr.cpu_time - last.cpu_time) / (long double)time_diff));
	last = curr;
	return usage;
}

static unsigned long long last_energy;
static unsigned long long energy_max_range;
static struct timespec last_clock;

/* Return energy consumed in uJ between two RAPL counter snapshots,
 * correcting for the wrap of the counter at max_range_uj.
 * Tested by tests/test-edge-cases.c. */
unsigned long long
b_cpu_energy_diff(unsigned long long curr, unsigned long long prev, unsigned long long max_range_uj)
{
	return (curr >= prev) ? curr - prev : curr + max_range_uj - prev;
}

static int
b_read_cpu_usage_power(void)
{
	if (unlikely(fd_cpu_usage_power == -1)) {
		fd_cpu_usage_power = b_cpu_init("/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj");
		if (unlikely(fd_cpu_usage_power < 0)) {
			fd_cpu_usage_power = -2;
			return 0;
		}
		const int fd_range = b_cpu_init("/sys/class/powercap/intel-rapl/intel-rapl:0/max_energy_range_uj");
		if (unlikely(fd_range < 0)) {
			close(fd_cpu_usage_power);
			fd_cpu_usage_power = -2;
			return 0;
		}
		char rbuf[SIZE_T_MAX_DIGITS + 1];
		const unsigned int rsz = b_proc_read_filefd(rbuf, sizeof(rbuf), fd_range);
		if (unlikely(rsz == (unsigned int)-1)) {
			close(fd_range);
			close(fd_cpu_usage_power);
			fd_cpu_usage_power = -2;
			return 0;
		}
		const char *unused_range;
		energy_max_range = u_strtoull10(rbuf, &unused_range);
		if (unlikely(close(fd_range) == -1))
			DIE(return -1);
	}
	if (unlikely(fd_cpu_usage_power == -2))
		return 0;
	char buf[SIZE_T_MAX_DIGITS + 1];
	const unsigned int read_sz = b_proc_read_filefd(buf, sizeof(buf), fd_cpu_usage_power);
	if (unlikely(read_sz == (unsigned int)-1))
		DIE(return -1);
	const char *unused;
	struct timespec curr_clock;
	if (unlikely(clock_gettime(CLOCK_MONOTONIC, &curr_clock) != 0))
		DIE(return -1);
	const unsigned long long curr_energy = u_strtoull10(buf, &unused);
	const double clock_diff = (double)(curr_clock.tv_sec - last_clock.tv_sec) + (double)(curr_clock.tv_nsec - last_clock.tv_nsec) / 1000000000;
	const unsigned long long energy_diff = b_cpu_energy_diff(curr_energy, last_energy, energy_max_range);
	last_energy = curr_energy;
	last_clock = curr_clock;
	/* Avoid dividing by zero when called twice within the same tick. */
	if (unlikely(clock_diff <= 0))
		return 0;
	return (int)((double)energy_diff / (clock_diff * 1000000.0));
}

char *
b_write_cpu_usage(char *dst, unsigned int dst_size, const char *unused, unsigned short *interval)
{
	char *p = dst;
	const int usage = b_read_cpu_usage();
	if (unlikely(usage == -1))
		DIE(return NULL);
	if (unlikely(usage <= -1))
		DIE(return NULL);
	p = u_utoa_le3_p((unsigned int)usage, p);
	(void)dst_size;
	(void)unused;
	(void)interval;
	return p;
}

char *
b_write_cpu_usage_power(char *dst, unsigned int dst_size, const char *unused, unsigned short *interval)
{
	char *p = dst;
	const int usage = b_read_cpu_usage_power();
	/* Watts are unbounded (>999 W on HEDT under load): use the
	 * general writer, not the 3-digit fast path. */
	p = u_utoa_p((unsigned int)usage, p);
	(void)dst_size;
	(void)unused;
	(void)interval;
	return p;
}

char *
b_write_cpu_temp(char *dst, unsigned int dst_size, const char *temp_file, unsigned short *interval)
{
	if (unlikely(fd_cpu_temp == -1)) {
		fd_cpu_temp = b_cpu_init(temp_file);
		if (unlikely(fd_cpu_temp < 0))
			/* Missing sensor: placeholder now, retry open next tick. */
			return u_stpcpy_len(dst, S_LITERAL("?"));
	}
	(void)temp_file;
	return b_write_tempfd(dst, dst_size, fd_cpu_temp, interval);
}
