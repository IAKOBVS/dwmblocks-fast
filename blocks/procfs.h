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

#ifndef B_PROCFS_H
#	define B_PROCFS_H 1

/* ../src/blocks/procfs.c */

#define B_PAGE_SIZE 4096

int
b_proc_exist_at(const char *proc_name, unsigned int proc_name_len, const char *pid_status_path);
unsigned int
b_proc_exist(const char *proc_name, unsigned int proc_name_len);
unsigned int
b_proc_read_procfs(char *dst, unsigned int dst_size, const char *filename);
char *
b_proc_value_get(const char *procfs_buf, unsigned int procfs_buf_len, const char *key, unsigned int key_len);
unsigned long long
b_proc_value_getull(const char *procfs_buf, unsigned int procfs_buf_len, const char *key, unsigned int key_len, int delimiter);

#endif /* B_PROCFS_H */
