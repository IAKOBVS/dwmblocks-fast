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

#include "../../include/config.h"

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include "../../include/macros.h"
#include "../../include/utils.h"
#include "../../include/blocks/temp.h"

int
b_read_cpu_usage()
{
	char buf[4096];
	int fd = open("/proc/stat", O_RDONLY);
	if (unlikely(fd == -1))
		DIE(return -1);
	ssize_t read_sz = read(fd, buf, sizeof(buf));
	if (unlikely(close(fd) == -1))
		DIE(return -1);
	if (unlikely(read_sz == -1))
		DIE(return -1);
	buf[read_sz] = '\0';
	static int l_user, l_nice, l_system, l_idle, l_iowait, l_irq, l_softirq;
	int user = l_user, nice = l_nice, system = l_system, idle = l_idle, iowait = l_iowait, irq = l_irq, softirq = l_softirq;
	char *p = buf;
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
b_write_cpu_usage(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	char *p = dst;
	const int usage = b_read_cpu_usage();
	if (unlikely(usage == -1))
		DIE(return dst);
	p = u_utoa_lt3_p((unsigned int)usage, p);
	p = u_stpcpy_len(p, S_LITERAL(UNIT_USAGE));
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

char *
b_write_cpu_temp(char *dst, unsigned int dst_len, const char *temp_file, unsigned int *interval)
{
	return b_write_temp(dst, dst_len, temp_file, interval);
}

char *
b_write_cpu_all(char *dst, unsigned int dst_len, const char *temp_file, unsigned int *interval)
{
	char *p = dst;
	const int usage = b_read_cpu_usage();
	if (unlikely(usage == -1))
		DIE(return dst);
	p = b_write_temp_internal(p, dst_len, temp_file);
	if (unlikely(p == dst))
		DIE(return dst);
	p = u_stpcpy_len(p, S_LITERAL(UNIT_TEMP));
	*p++ = ' ';
	p = u_utoa_lt3_p((unsigned int)usage, p);
	p = u_stpcpy_len(p, S_LITERAL(UNIT_USAGE));
	return p;
	(void)dst_len;
	(void)interval;
}
