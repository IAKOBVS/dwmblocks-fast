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

#ifdef HAVE_PROCFS
/* #ifdef HAVE_SYSINFO */
/* #include <sys/sysinfo.h> */

#	include <stdio.h>

#	include "../macros.h"
#	include "../utils.h"
#	include "../dwmblocks-fast.h"
#	include "procfs.h"

static char b_meminfo[B_PAGE_SIZE + 1];
static unsigned int b_meminfo_time = (unsigned int)-1;
static unsigned int b_meminfo_sz;

static int
b_meminfo_read(char *meminfo, unsigned int meminfo_sz)
{
	if (g_time != b_meminfo_time) {
		b_meminfo_time = g_time;
		b_meminfo_sz = b_proc_read_file(meminfo, meminfo_sz, "/proc/meminfo");
		if (unlikely(b_meminfo_sz == (unsigned int)-1))
			DIE(return -1);
	}
	return 0;
}

int
b_read_ram_usage_percent(void)
{
	if (unlikely(b_meminfo_read(b_meminfo, sizeof(b_meminfo)) == -1))
		DIE(return -1);
	const unsigned long long avail = b_proc_value_getull(b_meminfo, b_meminfo_sz, S_LITERAL("MemAvailable"), ':', ' ');
	if (unlikely(avail == (unsigned long long )-1))
		DIE(return -1);
	const unsigned long long total = b_proc_value_getull(b_meminfo, b_meminfo_sz, S_LITERAL("MemTotal"), ':', ' ');
	if (unlikely(total == (unsigned long long)-1))
		DIE(return -1);
	const int percent = 100 - (int)((long double)avail / (long double)total * (long double)100);
	return percent;
}

unsigned long long
b_read_ram_usage_available(void)
{
	if (unlikely(b_meminfo_read(b_meminfo, sizeof(b_meminfo)) == -1))
		DIE(return (unsigned long long)-1);
	const unsigned long long avail = b_proc_value_getull(b_meminfo, sizeof(b_meminfo), S_LITERAL("MemAvailable"), ':', ' ');
	if (unlikely(avail == (unsigned long long)-1))
		DIE(return (unsigned long long )-1);
	/* Values are in KiB. */
	return avail * U_KIB;
}

char *
b_write_ram_usage_percent(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	const int usage = b_read_ram_usage_percent();
	if (unlikely(usage == -1))
		DIE(return NULL);
	char *p = dst;
	p = u_utoa_le3_p((unsigned int)usage, p);
	return p;
	(void)dst_size;
	(void)unused;
	(void)interval;
}

char *
b_write_ram_usage_available(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	unsigned long long usage = b_read_ram_usage_available();
	if (unlikely(usage == (unsigned long long)-1))
		DIE(return NULL);
	const int unit = u_humanize(&usage);
	char *p = dst;
	p = u_ulltoa_p((unsigned int)usage, p);
	if (likely(unit != '\0'))
		*p++ = unit;
	*p = '\0';
	return p;
	(void)dst_size;
	(void)unused;
	(void)interval;
}

#endif
