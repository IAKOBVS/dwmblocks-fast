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

#ifndef UTILS_H
#	define UTILS_H 1

#	include <string.h>

static char *
utoa_p(unsigned int number, char *buf)
{
	char *start = buf;
	do
		*buf++ = number % 10 + '0';
	while ((number /= 10) != 0);
	char *end = buf;
	*buf-- = '\0';
	int c;
	for (; start < buf;) {
		c = *start;
		*start++ = *buf;
		*buf-- = c;
	}
	return (char *)end;
}

static int
xisdigit(int c)
{
	return ((unsigned)c - '0' < 10);
}

static unsigned int
xstrtou10(char *p, char **endp)
{
	unsigned int n = 0;
	unsigned char *pp = (unsigned char *)p;
	while (xisdigit(*pp))
		n = n * 10 + *(pp++) - '0';
	*endp = (char *)pp;
	return n;
}

static char *
xstpcpy_len(char *dst, const char *src, size_t n)
{
#	ifdef HAVE_MEMPCPY
	dst = (char *)mempcpy(dst, src, n);
	*dst = '\0';
	return dst;
#	else
	dst = (char *)memcpy(dst, src, n) + n;
	*dst = '\0';
	return dst;
#	endif
}

static char *
xstpcpy(char *dst, const char *src)
{
#	ifdef HAVE_STPCPY
	return stpcpy(dst, src);
#	else
	size_t n = strlen(src);
	dst = (char *)memcpy(dst, src, n) + n;
	*dst = '\0';
	return dst;
#	endif
}

static char *
xstrstr_len(const char *hs, size_t hs_len, const char *ne, size_t ne_len)
{
#	ifdef HAVE_STPCPY
	return (char *)memmem(hs, hs_len, ne, ne_len);
#	else
	return strstr((char *)hs, (char *)ne);
	(void)hs_len;
	(void)ne_len;
#	endif
}

#endif /* UTILS_H */
