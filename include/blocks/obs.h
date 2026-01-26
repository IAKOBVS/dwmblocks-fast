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

#ifndef C_OBS_H
#define C_OBS_H 1

/* ../../src/blocks/obs.c */

char *
b_write_obs(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval, const char *proc_name, unsigned int proc_name_len, unsigned int proc_interval, const char *proc_icon, unsigned int *pid_cache);
char *
b_write_obs_on(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);
char *
b_write_obs_recording(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);

#endif /* C_OBS_H */
