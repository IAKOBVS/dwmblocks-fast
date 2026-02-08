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

#include "../macros.h"
#include "../utils.h"
#include "../blocks/temp.h"
#include "procfs.h"

int
b_read_cpu_usage()
{
	char buf[B_PAGE_SIZE + 1];
	const unsigned int read_sz = b_proc_read_file(buf, sizeof(buf), "/proc/stat");
	if (unlikely(read_sz == (unsigned int)-1))
		DIE(return -1);
	static int l_user, l_nice, l_system, l_idle, l_iowait, l_irq, l_softirq;
	int user = l_user, nice = l_nice, system = l_system, idle = l_idle, iowait = l_iowait, irq = l_irq, softirq = l_softirq;
	const char *p = buf;
	/* clang-format off */
	p += S_LEN("CPU  ");
	l_user = (int)u_strtou10(p, &p); p += S_LEN(" ");
	l_nice = (int)u_strtou10(p, &p); p += S_LEN(" ");
	l_system = (int)u_strtou10(p, &p); p += S_LEN(" ");
	l_idle = (int)u_strtou10(p, &p); p += S_LEN(" ");
	l_iowait = (int)u_strtou10(p, &p); p += S_LEN(" ");
	l_irq = (int)u_strtou10(p, &p); p += S_LEN(" ");
	l_softirq = (int)u_strtou10(p, &p);
	/* clang-format off */
	const int tot = nice + user + system + idle + iowait + irq + softirq;
	const int l_tot = l_nice + l_user + l_system + l_idle + l_iowait + l_irq + l_softirq;
	const int sum = tot - l_tot;
	const int l_tot2 = l_user + l_nice + l_system + l_irq + l_softirq;
	const int tot2 = user + nice + system + irq + softirq;
	return (int)((long double)100 * ((long double)(tot2 - l_tot2) / (long double)sum));
}

char *
b_write_cpu_usage(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	char *p = dst;
	const int usage = b_read_cpu_usage();
	if (unlikely(usage == -1))
		DIE(return dst);
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
