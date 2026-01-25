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

int
b_atou_lt3(const char *s, int digits)
{
	if (digits == 2)
		return (*s - '0') * 10 + (*(s + 1) - '0');
	else if (digits == 1)
		return (*s - '0');
	else /* digits == 3 */
		return (*s - '0') * 100 + (*(s + 1) - '0') * 10 + (*(s + 2) - '0') * 1;
}

int
b_read_cpu_temp(void)
{
	/* Milidegrees = degrees * 1000 */
	char temp[S_LEN("100") + S_LEN("1000") + 1] = { 0 };
	int fd = open(CPU_TEMP_FILE, O_RDONLY);
	if (fd == -1)
		DIE(return -1);
	int read_sz = read(fd, temp, S_LEN(temp));
	if (close(fd) == -1)
		DIE(return -1);
	if (read_sz < 0)
		DIE(return -1);
	read_sz -= (int)S_LEN("1000");
	return b_atou_lt3(temp, read_sz);
}

char *
b_write_cpu_temp(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	const int temp = b_read_cpu_temp();
	if (temp < 0)
		DIE(return dst);
	char *p = dst;
	p = u_utoa_p((unsigned int)temp, dst);
	p = u_stpcpy_len(p, S_LITERAL(UNIT_TEMP));
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

int
b_read_cpu_usage()
{
	char buf[4096];
	int fd = open("/proc/stat", O_RDONLY);
	if (fd == -1)
		DIE(return -1);
	ssize_t read_sz = read(fd, buf, sizeof(buf));
	if (close(fd) == -1)
		DIE(return -1);
	if (read_sz < 0)
		DIE(return -1);
	buf[read_sz] = '\0';
	static int l_user, l_nice, l_system, l_idle, l_iowait, l_irq, l_softirq;
	int user = l_user, nice = l_nice, system = l_system, idle = l_idle, iowait = l_iowait, irq = l_irq, softirq = l_softirq;
	char *p = buf;
	/* clang-format off */
	p += S_LEN("CPU  ");
	l_user = (int)u_strtou10(p, &p); p += S_LEN(" ");
	if (user == 0)
		return 0;
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
	if (sum == 0)
		return 0;
	const int l_tot2 = l_user + l_nice + l_system + l_irq + l_softirq;
	const int tot2 = user + nice + system + irq + softirq;
	return (int)((long double)100 * ((long double)(tot2 - l_tot2) / (long double)sum));
}

char *
b_write_cpu_usage(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	const int usage = b_read_cpu_usage();
	if (usage < 0)
		DIE(return dst);
	char *p = dst;
	p = u_utoa_p((unsigned int)usage, p);
	p = u_stpcpy_len(p, S_LITERAL(UNIT_USAGE));
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

char *
b_write_cpu_all(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	const int temp = b_read_cpu_temp();
	if (temp < 0)
		DIE(return dst);
	const int usage = b_read_cpu_usage();
	if (usage < 0)
		DIE(return dst);
	char *p = dst;
	p = u_utoa_p((unsigned int)temp, p);
	p = u_stpcpy_len(p, S_LITERAL(UNIT_TEMP));
	*p++ = ' ';
	p = u_utoa_p((unsigned int)usage, p);
	p = u_stpcpy_len(p, S_LITERAL(UNIT_USAGE));
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}
