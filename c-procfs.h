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

#ifndef C_PROCFS_H
#	define C_PROCFS_H 1

#	include <fcntl.h>
#	include <dirent.h>
#	include <stdlib.h>
#	include <assert.h>
#	include <unistd.h>
#	include <errno.h>

#	include "macros.h"
#	include "utils.h"

#	define PROC        "/proc/"
#	define STATUS      "/status"
#	define PAGESZ      4096
#	define STATUS_NAME "Name:\t"

static int
c_proc_name_match(const char *proc_buf, unsigned int proc_buf_sz, const char *proc_name, unsigned int proc_name_len)
{
	const char *p = xstrstr_len(proc_buf, proc_buf_sz, S_LITERAL(STATUS_NAME));
	if (p) {
		p += S_LEN(STATUS_NAME);
		proc_buf_sz -= S_LEN(STATUS_NAME);
		if (proc_buf_sz > proc_name_len && !memcmp(p, proc_name, proc_name_len) && *(p + proc_name_len) == '\n')
			return 1;
	}
	return 0;
}

static int
c_read_proc_exists_at(const char *proc_name, unsigned int proc_name_len, const char *pid_status_path)
{
	int fd = open(pid_status_path, O_RDONLY);
	if (fd == -1) {
		if (errno == ENOMEM)
			ERR(return 0);
		return 0;
	}
	char buf[PAGESZ];
	/* Read /proc/[pid]/status */
	ssize_t read_sz = read(fd, buf, PAGESZ);
	if (close(fd) == -1)
		ERR(return 0);
	if (read_sz < 0)
		ERR(return 0);
	buf[read_sz] = '\0';
	return c_proc_name_match(buf, read_sz, proc_name, proc_name_len);
}

static unsigned int
c_read_proc_exists(const char *proc_name, unsigned int proc_name_len)
{
	char fname[S_LEN(PROC) + sizeof(unsigned int) * 8 + S_LEN(STATUS) + 1] = PROC;
	char *fnamep = fname + S_LEN(PROC);
	/* Open /proc/ */
	DIR *dp = opendir(PROC);
	if (dp == NULL)
		ERR(return 0);
	struct dirent *ep;
	while ((ep = readdir(dp))) {
		/* Enter /proc/[pid] */
		if (*(ep->d_name) == '.' || !xisdigit(*(ep->d_name)))
			continue;
		char *fname_e = fnamep;
		fname_e = xstpcpy(fname_e, ep->d_name);
		fname_e = xstpcpy_len(fname_e, S_LITERAL(STATUS));
		if (c_read_proc_exists_at(proc_name, proc_name_len, fname))
			return (unsigned int)atoi(ep->d_name);
	}
	if (closedir(dp) == -1)
		ERR();
	return 0;
}

#endif /* C_PROCFS_H */
