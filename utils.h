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
#	include "macros.h"


static ATTR_MAYBE_UNUSED char *
u_ulltoa_p(unsigned long long num, char *buf)
{
	char *start = buf;
	do
		*buf++ = num % 10 + '0';
	while ((num /= 10) != 0);
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

static ATTR_MAYBE_UNUSED char *
u_utoa_p(unsigned int num, char *buf)
{
	return u_ulltoa_p(num, buf);
}

static ATTR_MAYBE_UNUSED char *
u_utoa_lt2_p(unsigned int num, char *buf)
{
	/* digits == 2 */
	if (num > 9) {
		*(buf + 0) = (num / 10) + '0';
		*(buf + 1) = (num % 10) + '0';
		*(buf + 2) = '\0';
		return buf + 2;
	}
	/* digits == 1 */
	*(buf + 0) = num + '0';
	*(buf + 1) = '\0';
	return buf + 1;
}

static ATTR_MAYBE_UNUSED char *
u_utoa_lt3_p(unsigned int num, char *buf)
{
	/* digits == 2 */
	if (likely((unsigned int)(num - 10) < 90)) {
		*(buf + 0) = (num / 10) + '0';
		*(buf + 1) = (num % 10) + '0';
		*(buf + 2) = '\0';
		return buf + 2;
	}
	/* digits == 1 */
	if (num < 10) {
		*(buf + 0) = num + '0';
		*(buf + 1) = '\0';
		return buf + 1;
	}
	/* digits == 3 */
	*(buf + 0) = (num / 100) + '0';
	*(buf + 1) = ((num / 10) % 10) + '0';
	*(buf + 2) = (num % 10) + '0';
	*(buf + 3) = '\0';
	return buf + 3;
}

static ATTR_INLINE int
u_isdigit(int c)
{
	return ((unsigned)c - '0' < 10);
}

static ATTR_MAYBE_UNUSED unsigned int
u_strtou10(const char *p, const char **endp)
{
	unsigned int n = 0;
	unsigned char *pp = (unsigned char *)p;
	while (u_isdigit(*pp))
		n = n * 10 + *(pp++) - '0';
	*endp = (char *)pp;
	return n;
}

static ATTR_INLINE unsigned int
u_atou10(const char *p)
{
	const char *unused;
	return u_strtou10(p, &unused);
}

static ATTR_INLINE char *
u_stpcpy_len(char *dst, const char *src, size_t n)
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

static ATTR_INLINE char *
u_stpcpy(char *dst, const char *src)
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

static ATTR_INLINE char *
u_strstr_len(const char *hs, size_t hs_len, const char *ne, size_t ne_len)
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
