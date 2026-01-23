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

#	include "../config.h"

#	include <assert.h>
#	include <unistd.h>
#	include <string.h>

#	include "../macros.h"

/* Execute shell script. */
char *
c_write_shell(char *dst, unsigned int dst_len, const char *cmd, unsigned int *interval)
{
#	if HAVE_POPEN && HAVE_PCLOSE
	FILE *fp = popen(cmd, "r");
	if (fp == NULL)
		ERR(return NULL);
	int fd;
	ssize_t read_sz;
	fd = io_fileno(fp);
	if (fd == -1)
		ERR(pclose(fp); return NULL);
	read_sz = read(fd, dst, dst_len);
	if (pclose(fp) < 0)
		ERR(return NULL);
	if (read_sz == -1)
		ERR(return NULL);
	/* Chop newline. */
	char *end = (char *)memchr(dst, '\n', read_sz);
	/* Nul-terminate newline or end of string. */
	dst = end ? end : dst + read_sz;
	*dst = '\0';
#	else
	assert("c_write_cmd: calling c_write_cmd when popen or pclose is not available!");
	(void)dst_len;
	(void)cmd;
#	endif
	(void)interval;
	return dst;
}
