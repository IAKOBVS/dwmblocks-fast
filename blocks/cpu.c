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

#include <stdint.h>

#include "../macros.h"
#include "../utils.h"
#include "../blocks/temp.h"
#include "procfs.h"

#define SIZE_T_MAX_DIGITS 20

static ATTR_INLINE uint64_t
u_strtou10_special(const char *p, const char **endp)
{
	uint64_t n = 0;
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
	if (u_isdigit(*p)) { n = n * 10 + (*p++ - '0'); } else { goto out; }
out:
	*endp = p;
	return n;
}
#define u_strtou10 u_strtou10_special

static int fd_cpu_usage = -1;
static int fd_cpu_usage_power = -1;
static int fd_cpu_temp = -1;

static int
b_cpu_init(const char *filename)
{
	int fd = -1;
	for (int retry = 10; (fd = open(filename, O_RDONLY)) < 0 && retry; --retry)
		sleep(1);
	return fd;
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
	curr.user = u_strtou10(p, &p); p += S_LEN(" ");
	curr.nice = u_strtou10(p, &p); p += S_LEN(" ");
	curr.system = u_strtou10(p, &p); p += S_LEN(" ");
	curr.idle = u_strtou10(p, &p); p += S_LEN(" ");
	curr.iowait = u_strtou10(p, &p); p += S_LEN(" ");
	curr.irq = u_strtou10(p, &p); p += S_LEN(" ");
	curr.softirq = u_strtou10(p, &p);
	/* clang-format off */
	curr.time = curr.user + curr.nice + curr.system + curr.idle + curr.iowait + curr.irq + curr.softirq;
	curr.cpu_time = curr.user + curr.nice + curr.system + curr.irq + curr.softirq;
	if (unlikely(curr.time == 0))
		return 0;
	const int usage = (int)((long double)100 * ((long double)(curr.cpu_time - last.cpu_time) / (long double)(curr.time - last.time)));
	last = curr;
	return usage;
}

static int last_energy;
static struct timespec last_clock;

static int
b_read_cpu_usage_power(void)
{
	if (unlikely(fd_cpu_usage_power == -1)) {
		fd_cpu_usage_power = b_cpu_init("/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj");
		if (unlikely(fd_cpu_usage_power < 0))
			DIE(return -1);
	}
	char buf[SIZE_T_MAX_DIGITS + 1];
	const unsigned int read_sz = b_proc_read_filefd(buf, sizeof(buf), fd_cpu_usage_power);
	if (unlikely(read_sz == (unsigned int)-1))
		DIE(return -1);
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
	if (unlikely(usage >= 101 || usage <= -1))
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
	if (unlikely(fd_cpu_temp == -1)) {
		fd_cpu_temp = b_cpu_init(temp_file);
		if (unlikely(fd_cpu_temp < 0))
			DIE(return NULL);
	}
	return b_write_tempfd(dst, dst_size, fd_cpu_temp, interval);
	(void)temp_file;
}

uint64_t
test_u_strtou10(const char *p, const char **endp)
{
	return u_strtou10(p, endp);
}
