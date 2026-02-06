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
#include <string.h>

#include "../macros.h"

#if defined HAVE_POPEN && defined HAVE_PCLOSE && defined HAVE_FILENO

/* Execute shell script. */
char *
b_write_shell(char *dst, unsigned int dst_size, const char *cmd, unsigned int *interval)
{
	FILE *fp = popen(cmd, "r");
	if (unlikely(fp == NULL))
		DIE(return dst);
	const int fd = io_fileno(fp);
	if (unlikely(fd == -1)) {
		pclose(fp);
		DIE(return dst);
	}
	const ssize_t read_sz = read(fd, dst, dst_size);
	if (unlikely(pclose(fp) == -1))
		DIE(return dst);
	if (unlikely(read_sz == -1))
		DIE(return dst);
	/* Chop newline. */
	char *end = (char *)memchr(dst, '\n', (size_t)read_sz);
	/* Nul-terminate newline or end of string. */
	dst = end ? end : dst + read_sz;
	*dst = '\0';
	(void)interval;
	return dst;
}

#endif
