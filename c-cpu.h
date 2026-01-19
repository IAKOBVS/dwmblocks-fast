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

#ifndef C_CPU_H
#	define C_CPU_H 1

#	include "config.h"

#		include <stdlib.h>
#		include <assert.h>
#		include <fcntl.h>
#		include <unistd.h>

#		include "macros.h"
#		include "utils.h"

static int
c_atou_lt3(const char *s, int digits)
{
	if (digits == 2)
		return (*s - '0') * 10 + (*(s + 1) - '0');
	else if (digits == 1)
		return (*s - '0');
	else /* digits == 3 */
		return (*s - '0') * 100 + (*(s + 1) - '0') * 10 + (*(s + 2) - '0') * 1;
}

static int
c_read_cpu_temp(void)
{
	/* Milidegrees = degrees * 1000 */
	char temp[S_LEN("100") + S_LEN("1000") + 1] = { 0 };
	int fd = open(CPU_TEMP_FILE, O_RDONLY);
	if (fd == -1)
		ERR(return -1);
	int read_sz = read(fd, temp, S_LEN(temp));
	if (close(fd) == -1)
		ERR(return -1);
	if (read_sz < 0)
		ERR(return -1);
	read_sz -= (int)S_LEN("1000");
	return c_atou_lt3(temp, read_sz);
}

static char *
c_write_cpu_temp(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	int usage = c_read_cpu_temp();
	if (usage < 0)
		ERR(return NULL);
	char *p = utoa_p((unsigned int)usage, dst);
	p = xstpcpy_len(p, S_LITERAL("°"));
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

#endif /* C_CPU_H */
