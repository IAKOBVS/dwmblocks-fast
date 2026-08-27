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

#include <time.h>
#include <assert.h>

#include "../macros.h"
#include "../utils.h"

static time_t utc_off = -1;

time_t
b_time_init(void)
{
	const time_t t = time(NULL);
	if (t == (time_t)-1)
		return -1;
	const struct tm *tm = localtime(&t);
	if (unlikely(tm == NULL))
		return -1;
	return tm->tm_gmtoff;
}

static struct tm *
b_read_time(void)
{
	if (unlikely(utc_off == -1)) {
		utc_off = b_time_init();
		if (utc_off == -1)
			return NULL;
	}
	time_t t = time(NULL);
	if (unlikely(t == (time_t)-1))
		return NULL;
	t += utc_off;
	return gmtime(&t);
}

/* TODO: optimize, cache time formatting */
/* Format: 9:00 PM */
char *
b_write_time(char *dst, unsigned int dst_size, const char *unused, unsigned short *interval)
{
	const struct tm *tm = b_read_time();
	if (unlikely(tm == NULL))
		DIE(return NULL);
	unsigned int h = (unsigned int)tm->tm_hour;
	char meridiem;
	char *p = dst;
	/* Convert to 12-hour clock. 0:00 is 12 AM, 12:00 is 12 PM. */
	if (h >= 12) {
		meridiem = 'P';
		if (h > 12)
			h -= 12;
	} else {
		meridiem = 'A';
		if (h == 0)
			h = 12;
	}
	/* Write hour */
	p = u_utoa_le2_p(h, p);
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
	*interval = (unsigned short)(60 - tm->tm_sec);
	(void)dst_size;
	(void)unused;
	return p;
}

char *
b_write_date(char *dst, unsigned int dst_size, const char *unused, unsigned short *interval)
{
	const struct tm *tm = b_read_time();
	if (unlikely(tm == NULL))
		DIE(return NULL);
	char *p = dst;
	const char *days[] = {
		"Sun, ",
		"Mon, ",
		"Tue, ",
		"Wed, ",
		"Thu, ",
		"Fri, ",
		"Sat, ",
	};
	/* Write week day */
	p = u_stpcpy_len(p, days[tm->tm_wday], S_LEN("Day, "));
	/* Write month day */
	p = u_utoa_le2_p((unsigned int)tm->tm_mday, p);
	*p++ = ' ';
	const char *mons[] = {
		"Jan ",
		"Feb ",
		"Mar ",
		"Apr ",
		"May ",
		"Jun ",
		"Jul ",
		"Aug ",
		"Sep ",
		"Oct ",
		"Nov ",
		"Dec "
	};
	/* Write month */
	p = u_stpcpy_len(p, mons[tm->tm_mon], S_LEN("Mon "));
	/* Write year */
	p = u_utoa_p((unsigned int)tm->tm_year + 1900, p);
	/* Set next update for when the day changes. Max is 86399 s which
	 * exceeds unsigned short; clamp instead of wrapping (a wrapped
	 * value made the date re-run every ~5.8 h). */
	const unsigned int secs = ((23 - tm->tm_hour) * 3600) + ((59 - tm->tm_min) * 60) + (59 - tm->tm_sec);
	*interval = (unsigned short)MIN(secs, (unsigned int)(unsigned short)-1);
	(void)dst_size;
	(void)unused;
	return p;
}
