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
#include <bits/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "../macros.h"
#include "../utils.h"
#include "../blocks/temp.h"
#include "procfs.h"

int
b_read_cpu_usage(void)
{
	char buf[B_PAGE_SIZE + 1];
	const unsigned int read_sz = b_proc_read_file(buf, sizeof(buf), "/proc/stat");
	if (unlikely(read_sz == (unsigned int)-1))
		DIE(return -1);
	typedef struct {
		unsigned long long user, nice, system, idle, iowait, irq, softirq;
		unsigned long long time;
		unsigned long long cpu_time;
	} time_ty;
	static time_ty last;
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
	/* clang-format off */
	curr.time = curr.user + curr.nice + curr.system + curr.idle + curr.iowait + curr.irq + curr.softirq;
	curr.cpu_time = curr.user + curr.nice + curr.system + curr.irq + curr.softirq;
	if (unlikely(curr.time == 0))
		return 0;
	const int usage = (int)((long double)100 * ((long double)(curr.cpu_time - last.cpu_time) / (long double)(curr.time - last.time)));
	last = curr;
	return usage;
}

int
b_read_cpu_usage_power(void)
{
	char buf[sizeof(size_t) * 8 + 1];
	const unsigned int read_sz = b_proc_read_file(buf, sizeof(buf), "/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj");
	if (unlikely(read_sz == (unsigned int)-1))
		DIE(return -1);
	static int last_energy;
	static struct timespec last_clock;
	const char *unused;
	struct timespec curr_clock;
	if (unlikely(clock_gettime(CLOCK_MONOTONIC, &curr_clock) != 0))
		DIE(return -1);
	const int curr_energy = (int)u_strtou10(buf, &unused);
	const double clock_diff = (double)(curr_clock.tv_sec - last_clock.tv_sec) + (double)(curr_clock.tv_nsec - last_clock.tv_nsec) / 1000000000;
	const double energy_diff = (double)(curr_energy - last_energy);
	last_energy = curr_energy;
	last_clock = curr_clock;
	return (int)(energy_diff / (clock_diff * 1000000.0));
}

char *
b_write_cpu_usage(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	char *p = dst;
	const int usage = b_read_cpu_usage();
	if (unlikely(usage == -1))
		DIE(return NULL);
	p = u_utoa_le3_p((unsigned int)usage, p);
	return p;
	(void)dst_size;
	(void)unused;
	(void)interval;
}

char *
b_write_cpu_usage_power(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	char *p = dst;
	const int usage = b_read_cpu_usage_power();
	if (unlikely(usage == -1))
		DIE(return NULL);
	p = u_utoa_le3_p((unsigned int)usage, p);
	return p;
	(void)dst_size;
	(void)unused;
	(void)interval;
}

char *
b_write_cpu_temp(char *dst, unsigned int dst_size, const char *temp_file, unsigned int *interval)
{
	return b_write_temp(dst, dst_size, temp_file, interval);
}
