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

#include <fcntl.h>
#include <unistd.h>

#include "../../include/macros.h"
#include "../../include/utils.h"
#include "../../include/config.h"

char *
b_write_temp_internal(char *dst, unsigned int dst_len, const char *temp_file)
{
	const int fd = open(temp_file, O_RDONLY);
	if (unlikely(fd == -1))
		DIE(return dst);
	/* Milidegrees = degrees * 1000 */
	int read_sz = read(fd, dst, S_LEN("100") + S_LEN("000") + S_LEN("\n"));
	if (unlikely(close(fd) == -1))
		DIE(return dst);
	if (unlikely(read_sz  == -1))
		DIE(return dst);
	/* Don't read the newline. */
	if (*(dst + read_sz - 1) == '\n')
		--read_sz;
	/* Don't read the milidegrees. */
	read_sz -= S_LEN("000");
	*(dst + read_sz) = '\0';
	return dst + read_sz;
	(void)dst_len;
}

char *
b_write_temp(char *dst, unsigned int dst_len, const char *temp_file, unsigned int *interval)
{
	char *p = dst;
	p = b_write_temp_internal(p, dst_len, temp_file);
	if (unlikely(p == dst))
		DIE(return dst);
	return p;
	(void)dst_len;
	(void)interval;
}
