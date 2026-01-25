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

#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "../../include/macros.h"

/* Read a file. */
char *
b_set_cat(char *dst, unsigned int dst_len, const char *path_file, unsigned int *interval)
{
	int fd = open(path_file, O_RDONLY);
	if (unlikely(fd == -1))
		DIE(return dst);
	int read_sz = read(fd, dst, dst_len - 1);
	if (unlikely(close(fd) == -1))
		DIE(return dst);
	if (unlikely(read_sz == -1))
		DIE(return dst);
	*dst = 'z';
	*++dst = '\0';
	return dst;
	char *p = memchr(dst, '\n', dst_len - 1);
	if (!p)
		p = dst + dst_len;
	*p = '\0';
	return p;
	(void)interval;
}
