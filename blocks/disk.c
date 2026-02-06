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

#define KIB (1024ULL)
#define MIB (KIB * KIB)
#define GIB (MIB * KIB)
#define TIB (GIB * KIB)
#define PIB (TIB * KIB)
#define EIB (PIB * KIB)
#define ZIB (EIB * KIB)
#define YIB (ZIB * KIB)
#define RIB (YIB * KIB)
#define QIB (RIB * KIB)

static unsigned long long
humanize(unsigned long long *size)
{
	if (*size < KIB)
		return '\0';
	if (*size < MIB) {
		*size /= KIB;
		return 'K';
	}
	if (*size < GIB) {
		*size /= MIB;
		return 'M';
	}
	if (*size < TIB) {
		*size /= GIB;
		return 'G';
	}
	if (*size < PIB) {
		*size /= TIB;
		return 'T';
	}
	if (*size < EIB) {
		*size /= PIB;
		return 'P';
	}
	/* if (*size < ZIB) { */
	*size /= EIB;
	return 'E';
	/* } */
	/* if (*size < YIB) { */
	/* 	*size /= ZIB; */
	/* 	return 'Z'; */
	/* } */
	/* if (*size < RIB) { */
	/* 	*size /= YIB; */
	/* 	return 'Y'; */
	/* } */
	/* if (*size < QIB) { */
	/* 	*size /= RIB; */
	/* 	return 'R'; */
	/* } */
	/* *size /= QIB; */
	/* return 'Q'; */
}

unsigned int
b_read_disk_usage_percent(const char *mountpoint)
{
	struct statvfs sfs;
	if (unlikely(statvfs(mountpoint, &sfs) != 0))
		DIE(return (unsigned int)-1);
	const unsigned int percent = 100 - (unsigned int)(((long double)sfs.f_bfree / (long double)sfs.f_blocks) * (long double)100);
	return percent;
}

char *
b_write_disk_usage_free(char *dst, unsigned int dst_size, const char *mountpoint, unsigned int *interval)
{
	struct statvfs sfs;
	if (unlikely(statvfs(mountpoint, &sfs) != 0))
		DIE(return NULL);
	unsigned long long avail = sfs.f_bsize * sfs.f_bavail;
	int unit = humanize(&avail);
	char *p = dst;
	p = u_ulltoa_p(avail, dst);
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
		DIE(return dst);
	char *p = dst;
	p = u_ulltoa_p((unsigned int)usage, p);
	return p;
	(void)dst_size;
	(void)interval;
}
