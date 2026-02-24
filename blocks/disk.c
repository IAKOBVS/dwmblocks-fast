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

#include <sys/statvfs.h>
#include <assert.h>
#include <stdio.h>

#include "../macros.h"
#include "../utils.h"
#include "../dwmblocks-fast.h"

static struct statvfs b_statvfs;
static const char *b_mountpoint;
static unsigned int b_statvfs_time = (unsigned int)-1;

static int
b_read_statvfs(const char *mountpoint, struct statvfs *sfs)
{
	if (g_time != b_statvfs_time || b_mountpoint != mountpoint) {
		b_mountpoint = mountpoint;
		b_statvfs_time = g_time;
		if (unlikely(statvfs(mountpoint, sfs) != 0))
			DIE(return -1);
	}
	return 0;
}

unsigned int
b_read_disk_usage_percent(const char *mountpoint)
{
	if (unlikely(b_read_statvfs(mountpoint, &b_statvfs) != 0))
		DIE(return (unsigned int)-1);
	const unsigned int percent = 100 - (unsigned int)(((long double)b_statvfs.f_bfree / (long double)b_statvfs.f_blocks) * (long double)100);
	return percent;
}

char *
b_write_disk_usage_free(char *dst, unsigned int dst_size, const char *mountpoint, unsigned int *interval)
{
	if (unlikely(b_read_statvfs(mountpoint, &b_statvfs) != 0))
		DIE(return NULL);
	unsigned long long avail = b_statvfs.f_bsize * b_statvfs.f_bavail;
	int unit = u_humanize(&avail);
	char *p = dst;
	p = u_ulltoa_p(avail, dst);
	if (likely(unit != '\0'))
		*p++ = unit;
	*p = '\0';
	return p;
	(void)dst_size;
	(void)interval;
}

char *
b_write_disk_usage_percent(char *dst, unsigned int dst_size, const char *mountpoint, unsigned int *interval)
{
	const unsigned int usage = b_read_disk_usage_percent(mountpoint);
	if (unlikely(usage == (unsigned int)-1))
		DIE(return NULL);
	char *p = dst;
	p = u_ulltoa_p((unsigned int)usage, p);
	return p;
	(void)dst_size;
	(void)interval;
}
