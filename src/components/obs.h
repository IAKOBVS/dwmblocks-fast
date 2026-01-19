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
#	define C_OBS_H 1

#	include "procfs.h"


#	define OBS_OPEN_ICON       "🎥 OBS"
#	define OBS_OPEN_INTERVAL   4
#	define OBS_RECORD_ICON     "🔴 Recording"
#	define OBS_RECORD_INTERVAL 2

static unsigned int c_obs_recording_pid;
static unsigned int c_obs_open_pid;

static char *
c_write_obs(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval, const char *proc_name, unsigned int proc_name_len, unsigned int proc_interval, const char *proc_icon, unsigned int *pid_cache)
{
	/* Need to search /proc/[pid] for proc. */
	if (*pid_cache == 0) {
		/* Cache the pid to avoid searching for next calls. */
		*pid_cache = c_read_proc_exists(proc_name, proc_name_len);
		if (*pid_cache == 0) {
			/* OBS is not recording, but still on. Keep checking. */
			if (pid_cache == &c_obs_recording_pid && c_obs_open_pid)
				*interval = proc_interval;
			/* OBS is closed. Stop checking. */
			else
				*interval = 0;
			return dst;
		}
	} else {
		/* Construct path: /proc/[pid]/status. */
		char fname[S_LEN(PROC) + sizeof(unsigned int) * 8 + S_LEN(STATUS) + 1];
		char *fnamep = fname;
		/* /proc/ */
		fnamep = xstpcpy_len(fnamep, S_LITERAL(PROC));
		/* /proc/[pid] */
		fnamep = utoa_p(*pid_cache, fnamep);
		/* /proc/[pid]/status */
		fnamep = xstpcpy_len(fnamep, S_LITERAL(STATUS));
		(void)fnamep;
		if (!c_read_proc_exists_at(proc_name, proc_name_len, fname)) {
			*pid_cache = 0;
			*interval = 0;
			return dst;
		}
	}
	dst = xstpcpy(dst, proc_icon);
	*interval = proc_interval;
	return dst;
	(void)unused;
	(void)dst_len;
}

static char *
c_write_obs_on(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return c_write_obs(dst, dst_len, unused, interval, S_LITERAL("obs"), OBS_OPEN_INTERVAL, OBS_OPEN_ICON, &c_obs_open_pid);
	(void)unused;
	(void)dst_len;
}

static char *
c_write_obs_recording(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return c_write_obs(dst, dst_len, unused, interval, S_LITERAL("obs-ffmpeg-mux"), OBS_RECORD_INTERVAL, OBS_RECORD_ICON, &c_obs_recording_pid);
	(void)unused;
	(void)dst_len;
}

#endif /* C_OBS_H */
