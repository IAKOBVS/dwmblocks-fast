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
#include <fcntl.h>
#include <unistd.h>

#include "../../include/macros.h"
#include "../../include/utils.h"
#include "../../include/config.h"

/* ../../include/blocks/webcam.h */

#ifdef HAVE_PROCFS

char *
b_write_webcam_on(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	int fd = open("/proc/modules", O_RDONLY);
	if (unlikely(fd == -1))
		DIE(return dst);
	char buf[4096];
	ssize_t read_sz;
	read_sz = read(fd, buf, sizeof(buf));
	if (unlikely(close(fd) == -1))
		DIE(return dst);
	if (unlikely(read_sz == -1))
		DIE(return dst);
	buf[read_sz] = '\0';
	if (u_strstr_len(buf, (size_t)read_sz, S_LITERAL("uvcvideo")))
		dst = u_stpcpy_len(dst, S_LITERAL(ICON_WEBCAM_ON));
	return dst;
	(void)dst_len;
	(void)interval;
	(void)unused;
}

#endif
