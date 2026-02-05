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
#include <dirent.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "../macros.h"
#include "../utils.h"

int
b_proc_name_match(const char *proc_buf, unsigned int proc_buf_sz, const char *proc_name, unsigned int proc_name_len)
{
	const char *p = u_strstr_len(proc_buf, proc_buf_sz, S_LITERAL("Name:\t"));
	if (p) {
		p += S_LEN("Name:\t");
		proc_buf_sz -= S_LEN("Name:\t");
		if (proc_buf_sz > proc_name_len && !memcmp(p, proc_name, proc_name_len) && *(p + proc_name_len) == '\n')
			return 1;
	}
	return 0;
}

int
b_read_proc_exist_at(const char *proc_name, unsigned int proc_name_len, const char *pid_status_path)
{
	const int fd = open(pid_status_path, O_RDONLY);
	if (fd == -1) {
		if (unlikely(errno == ENOMEM))
			DIE(return -1);
		return 0;
	}
#if !defined PATH_MAX
	enum {
		PATH_MAX = 4096,
	};
#endif
	char buf[PATH_MAX];
	/* Read /proc/[pid]/status */
	const ssize_t read_sz = read(fd, buf, PATH_MAX);
	if (unlikely(close(fd) == -1))
		DIE(return -1);
	if (unlikely(read_sz == -1))
		DIE(return -1);
	buf[read_sz] = '\0';
	return b_proc_name_match(buf, read_sz, proc_name, proc_name_len);
}

unsigned int
b_read_proc_exist(const char *proc_name, unsigned int proc_name_len)
{
	char fname[S_LEN("/proc/") + sizeof(unsigned int) * 8 + S_LEN("/status") + 1] = "/proc/";
	char *fnamep = fname + S_LEN("/proc/");
	/* Open /proc/ */
	DIR *dp = opendir("/proc/");
	if (unlikely(dp == NULL))
		DIE(return (unsigned int)-1);
	struct dirent *ep;
	errno = 0;
	while ((ep = readdir(dp))) {
		/* Enter /proc/[pid] */
		if (*(ep->d_name) == '.' || !u_isdigit(*(ep->d_name)))
			continue;
		char *fname_e = fnamep;
		fname_e = u_stpcpy(fname_e, ep->d_name);
		fname_e = u_stpcpy_len(fname_e, S_LITERAL("/status"));
		if (b_read_proc_exist_at(proc_name, proc_name_len, fname))
			return (unsigned int)atoi(ep->d_name);
	}
	if (unlikely(errno == EBADF))
		DIE(return (unsigned int)-1);
	if (unlikely(closedir(dp) == -1))
		DIE(return (unsigned int)-1);
	return 0;
}
