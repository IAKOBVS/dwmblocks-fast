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

#include <sys/sysinfo.h>
#include <time.h>
#include <assert.h>

#include "../macros.h"
#include "../utils.h"

struct tm *
b_read_time(void)
{
	time_t t = time(NULL);
	if (t == (time_t)-1)
		return NULL;
	return localtime(&t);
}

/* Format: 9:00 PM */
char *
b_write_time(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	struct tm *tm = b_read_time();
	if (unlikely(tm == NULL))
		DIE(return NULL);
	unsigned int h = (unsigned int)tm->tm_hour;
	char meridiem;
	char *p = dst;
	/* Convert to 12-hour clock */
	if (h > 12) {
		meridiem = 'P';
		h -= 12;
	} else {
		meridiem = 'A';
	}
	/* Write hour */
	p = u_utoa_lt2_p(h, p);
	/* Write : */
	*p++ = ':';
	/* Write minutes */
	if (tm->tm_min < 10) {
		/* One digit */
		*p++ = '0';
		*p++ = tm->tm_min + '0';
	} else {
		/* Two digits */
		*p++ = (tm->tm_min / 10) + '0';
		*p++ = (tm->tm_min % 10) + '0';
	}
	*p++ = ' ';
	/* AM or PM */
	*p++ = meridiem;
	*p++ = 'M';
	*p = '\0';
	/* Set next update for when minute changes. */
	*interval = (unsigned int)(60 - tm->tm_sec);
	return p;
	(void)dst_size;
	(void)unused;
	(void)interval;
}

char *
b_write_date(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	struct tm *tm = b_read_time();
	if (unlikely(tm == NULL))
		DIE(return NULL);
	char *p = dst;
	/* Write day */
	switch (tm->tm_wday) {
	case 0: p = u_stpcpy_len(p, S_LITERAL("Sun, ")); break;
	case 1: p = u_stpcpy_len(p, S_LITERAL("Mon, ")); break;
	case 2: p = u_stpcpy_len(p, S_LITERAL("Tue, ")); break;
	case 3: p = u_stpcpy_len(p, S_LITERAL("Wed, ")); break;
	case 4: p = u_stpcpy_len(p, S_LITERAL("Thu, ")); break;
	case 5: p = u_stpcpy_len(p, S_LITERAL("Fri, ")); break;
	case 6: p = u_stpcpy_len(p, S_LITERAL("Sat, ")); break;
	}
	p = u_utoa_lt2_p((unsigned int)tm->tm_mday, p);
	*p++ = ' ';
	/* Write month */
	switch (tm->tm_mon) {
	case 0: p = u_stpcpy_len(p, S_LITERAL("Jan ")); break;
	case 1: p = u_stpcpy_len(p, S_LITERAL("Feb ")); break;
	case 2: p = u_stpcpy_len(p, S_LITERAL("Mar ")); break;
	case 3: p = u_stpcpy_len(p, S_LITERAL("Apr ")); break;
	case 4: p = u_stpcpy_len(p, S_LITERAL("May ")); break;
	case 5: p = u_stpcpy_len(p, S_LITERAL("Jun ")); break;
	case 6: p = u_stpcpy_len(p, S_LITERAL("Jul ")); break;
	case 7: p = u_stpcpy_len(p, S_LITERAL("Agu ")); break;
	case 8: p = u_stpcpy_len(p, S_LITERAL("Sep ")); break;
	case 9: p = u_stpcpy_len(p, S_LITERAL("Oct ")); break;
	case 10: p = u_stpcpy_len(p, S_LITERAL("Nov ")); break;
	case 11: p = u_stpcpy_len(p, S_LITERAL("Dec ")); break;
	}
	/* Write year */
	p = u_utoa_p((unsigned int)tm->tm_year + 1900, p);
	/* Set next update for when the day changes. */
	*interval = (unsigned int)(((23 - tm->tm_hour) * 3600) + ((59 - tm->tm_min) * 60) + (60 - tm->tm_sec));
	return p;
	(void)dst_size;
	(void)unused;
}
